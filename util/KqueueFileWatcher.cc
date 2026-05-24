// birch - IRC daemon, built with Bazel
//
// Copyright (C) 2026 Benjamin Bader
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "util/KqueueFileWatcher.h"

#include <fcntl.h>
#include <sys/event.h>
#include <unistd.h>

#include <chrono>
#include <functional>
#include <latch>
#include <optional> 

#include "absl/container/flat_hash_set.h"
#include "absl/log/log.h"

namespace fs = std::filesystem;

namespace birch::util {

namespace {

constexpr std::uint32_t kFileFlags =
    NOTE_DELETE | NOTE_RENAME | NOTE_REVOKE |
    NOTE_WRITE  | NOTE_EXTEND | NOTE_ATTRIB;

constexpr std::uint32_t kDirFlags  =
    NOTE_WRITE  | NOTE_DELETE | NOTE_RENAME;

constexpr std::uintptr_t kWakeIdent = 1;
constexpr auto kCoalesceWindow = std::chrono::milliseconds(50);

void* WatchIdToPtr(KqueueFileWatcher::WatchId id)
{
    static_assert(sizeof(KqueueFileWatcher::WatchId) <= sizeof(void*));
    return reinterpret_cast<void*>(static_cast<std::uintptr_t>(id));
}

KqueueFileWatcher::WatchId PtrToWatchId(void* ptr)
{
    static_assert(sizeof(KqueueFileWatcher::WatchId) <= sizeof(void*));
    return static_cast<KqueueFileWatcher::WatchId>(reinterpret_cast<std::uintptr_t>(ptr));
}

} // namespace

KqueueFileWatcher::WatchHandle::WatchHandle(WatchId id, std::weak_ptr<KqueueFileWatcher>&& watcher)
    : id(id)
    , watcher(std::move(watcher))
{
}

KqueueFileWatcher::WatchHandle::~WatchHandle()
{
    Unwatch();
}

void KqueueFileWatcher::WatchHandle::Unwatch()
{
    if (auto w = watcher.lock())
    {
        w->CloseWatch(id);
        watcher.reset();
    }
}

KqueueFileWatcher::Watch::Watch(WatchId id, const std::filesystem::path& path, const std::filesystem::path& parentPath, int fileFd, int dirFd, IFileWatcher::Callback&& callback)
    : id(id)
    , path(path)
    , parentPath(parentPath)
    , fileFd(fileFd)
    , dirFd(dirFd)
    , callback(std::move(callback))
{
}

absl::StatusOr<std::shared_ptr<IFileWatcher>> IFileWatcher::Create()
{
    return KqueueFileWatcher::Create();
}

absl::StatusOr<std::shared_ptr<KqueueFileWatcher>> KqueueFileWatcher::Create()
{
    int kq = ::kqueue();
    if (kq == -1)
    {
        return absl::ErrnoToStatus(errno, "kqueue() failed");
    }
    else
    {
        // Set the kqueue to close on exec.
        int kqFlags = ::fcntl(kq, F_GETFD);
        if (kqFlags != -1)
        {
            if (::fcntl(kq, F_SETFD, kqFlags | FD_CLOEXEC) == -1)
            {
                PLOG(WARNING) << "fcntl() failed to set FD_CLOEXEC on kqueue";
            }
        }
    }

    struct kevent wake;
    EV_SET(&wake, kWakeIdent, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    if (::kevent(kq, &wake, 1, nullptr, 0, nullptr) == -1)
    {
        int e = errno;
        ::close(kq);
        return absl::ErrnoToStatus(e, "kevent() failed");
    }

    auto fw = std::shared_ptr<KqueueFileWatcher>(new KqueueFileWatcher(kq));

    std::latch ready(1);
    fw->m_thread = std::thread([&ready, raw = fw.get()]()
    {
        ready.count_down();
        try
        {
            raw->Run();
        }
        catch (const std::exception& e)
        {
            LOG(ERROR) << "KqueueFileWatcher thread caught exception: " << e.what();
        }
        catch (...)
        {
            LOG(ERROR) << "KqueueFileWatcher thread caught unknown exception";
        }
    });

    // _try_ to ensure that the thread runs before anyone has the chance to destroy the object
    ready.wait();

    return fw;
}

absl::StatusOr<std::unique_ptr<IFileWatcher::IWatchHandle>> KqueueFileWatcher::WatchFile(const std::filesystem::path& path, IFileWatcher::Callback callback)
{
    auto canon = fs::absolute(path).lexically_normal();
    auto dir = canon.parent_path();
    if (dir.empty())
    {
        dir = ".";
    }
    
    int dirFd = ::open(dir.c_str(), O_RDONLY | O_EVTONLY | O_CLOEXEC);
    if (dirFd == -1)
    {
        int e = errno;
        return absl::ErrnoToStatus(e, "open() failed");
    }
    
    auto id = ++m_nextWatchId;
    {
        std::lock_guard lock(m_mutex);
        auto [it, _] = m_watches.emplace(id, Watch(id, canon, dir, -1, dirFd, std::move(callback)));
        Watch& watch = it->second;

        if (!ArmDir(watch))
        {
            int e = errno;

            ::close(watch.dirFd);
            watch.dirFd = -1;

            m_watches.erase(id);
            return absl::ErrnoToStatus(e, "ArmDir() failed");
        }

        // it's okay if the file isn't found yet, we'll arm it when it's created
        TryOpenAndArmFile(watch);
    }

    return std::make_unique<WatchHandle>(id, weak_from_this());
}

KqueueFileWatcher::KqueueFileWatcher(int kqueue)
    : m_kqueue(kqueue)
{
}

KqueueFileWatcher::~KqueueFileWatcher()
{
    m_shutdown.store(true, std::memory_order_relaxed);
    Wake();
    m_thread.join();

    for (auto& [_, watch] : m_watches)
    {
        DisarmWatch(watch);
    }
    m_watches.clear();

    ::close(m_kqueue);
}

bool KqueueFileWatcher::ArmFile(Watch& watch)
{
    if (watch.fileFd == -1)
    {
        return false;
    }

    struct kevent event;
    EV_SET(&event, watch.fileFd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, kFileFlags, 0, WatchIdToPtr(watch.id));

    if (::kevent(m_kqueue, &event, 1, nullptr, 0, nullptr) < 0)
    {
        PLOG(ERROR) << "kevent() failed";
        return false;
    }

    return true;
}

bool KqueueFileWatcher::ArmDir(Watch& watch)
{
    if (watch.dirFd == -1)
    {
        return false;
    }
    
    struct kevent event;
    EV_SET(&event, watch.dirFd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_CLEAR, kDirFlags, 0, WatchIdToPtr(watch.id));

    if (::kevent(m_kqueue, &event, 1, nullptr, 0, nullptr) < 0)
    {
        PLOG(ERROR) << "kevent() failed";
        return false;
    }

    return true;
}

void KqueueFileWatcher::DisarmFile(Watch& watch)
{
    if (watch.fileFd == -1)
    {
        return;
    }

    struct kevent event;
    EV_SET(&event, watch.fileFd, EVFILT_VNODE, EV_DELETE, 0, 0, nullptr);

    if (::kevent(m_kqueue, &event, 1, nullptr, 0, nullptr) < 0)
    {
        PLOG(ERROR) << "kevent() failed";
    }

    ::close(watch.fileFd);
    watch.fileFd = -1;
}

void KqueueFileWatcher::DisarmDir(Watch& watch)
{
    if (watch.dirFd == -1)
    {
        return;
    }

    struct kevent event;
    EV_SET(&event, watch.dirFd, EVFILT_VNODE, EV_DELETE, 0, 0, nullptr);

    if (::kevent(m_kqueue, &event, 1, nullptr, 0, nullptr) < 0)
    {
        PLOG(ERROR) << "kevent() failed";
    }

    ::close(watch.dirFd);
    watch.dirFd = -1;
}

bool KqueueFileWatcher::TryOpenAndArmFile(Watch& watch)
{
    if (watch.fileFd != -1)
    {
        return true;
    }

    int fd = ::open(watch.path.c_str(), O_EVTONLY | O_CLOEXEC);
    if (fd == -1)
    {
        return false;
    }

    watch.fileFd = fd;
    if (!ArmFile(watch))
    {
        ::close(fd);
        watch.fileFd = -1;
        return false;
    }

    return true;
}

void KqueueFileWatcher::Run()
{
    constexpr int kBatch = 16;
    struct kevent events[kBatch];

    absl::flat_hash_set<WatchId> dirty;
    std::optional<std::chrono::steady_clock::time_point> deadline;

    while (!m_shutdown.load(std::memory_order_relaxed))
    {
        // First, handle any pending unwatches.
        {
            std::lock_guard lock(m_mutex);
            for (auto id : m_pendingUnwatches)
            {
                auto watch = m_watches.find(id);
                if (watch != m_watches.end())
                {
                    DisarmWatch(watch->second);
                    m_watches.erase(watch);
                }
            }
            m_pendingUnwatches.clear();
        }

        // Next, wait for events.
        timespec ts_storage{};
        timespec* ts = nullptr;
        if (deadline)
        {
            auto remaining = *deadline - std::chrono::steady_clock::now();
            if (remaining.count() < 0)
            {
                remaining = std::chrono::steady_clock::duration::zero();
            }
            ts_storage.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(remaining).count();
            ts_storage.tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(remaining - std::chrono::seconds(ts_storage.tv_sec)).count();
            ts = &ts_storage;
        }

        int n = ::kevent(m_kqueue, nullptr, 0, events, kBatch, ts);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            PLOG(ERROR) << "kevent() failed";
            continue;
        }

        for (int i = 0; i < n; ++i)
        {
            struct kevent& event = events[i];
            if (event.filter == EVFILT_USER && event.ident == kWakeIdent)
            {
                continue;
            }

            WatchId id = PtrToWatchId(event.udata);
            bool changed = false;
            {
                std::lock_guard lock(m_mutex);
                auto it = m_watches.find(id);
                if (it == m_watches.end())
                {
                    LOG(INFO) << "Watch not found: " << id;
                    continue;
                }

                Watch& watch = it->second;
                if (static_cast<int>(event.ident) == watch.fileFd)
                {
                    changed = HandleFileEvent(watch, event.fflags);
                }
                else if (static_cast<int>(event.ident) == watch.dirFd)
                {
                    changed = HandleDirEvent(watch);
                }
                else
                {
                    VLOG(1) << "Ignoring stale event for an fd we no longer own: " << event.ident;
                    continue;
                }
            }
            
            if (changed)
            {
                dirty.insert(id);
                if (!deadline)
                {
                    deadline = std::chrono::steady_clock::now() + kCoalesceWindow;
                }
            }
        }

        if (deadline && std::chrono::steady_clock::now() >= *deadline)
        {
            FireDirty(dirty);
            dirty.clear();
            deadline.reset();
        }
    }
}

void KqueueFileWatcher::Wake()
{
    struct kevent event;
    EV_SET(&event, kWakeIdent, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);

    if (::kevent(m_kqueue, &event, 1, nullptr, 0, nullptr) < 0)
    {
        PLOG(ERROR) << "kevent() failed";
    }
}

void KqueueFileWatcher::CloseWatch(WatchId id)
{
    {
        std::lock_guard lock(m_mutex);
        m_pendingUnwatches.push_back(id);
    }
    Wake();
}

void KqueueFileWatcher::DisarmWatch(Watch& watch)
{
    DisarmFile(watch);
    DisarmDir(watch);
}

bool KqueueFileWatcher::HandleFileEvent(Watch& watch, std::uint32_t fflags)
{
    if (fflags & (NOTE_DELETE | NOTE_RENAME | NOTE_REVOKE))
    {
        // The inode we had open is unreachable through `w.path` now.
        // This is the first half of an atomic replace, OR a plain delete.
        // Drop the stale fd. Do NOT fire — the new file may not be in place.
        DisarmFile(watch);

        // Eagerly try once; sometimes rename(2) has already completed by
        // the time we're handling the kevent. If not, the directory event
        // will recover us.
        return TryOpenAndArmFile(watch);
    }
    
    return (fflags & (NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB)) != 0;    
}

bool KqueueFileWatcher::HandleDirEvent(Watch& watch)
{
    if (watch.fileFd == -1)
    {
        // The directory changed - maybe our file exists now?
        return TryOpenAndArmFile(watch);
    }

    // We've got a live fd on the current inode.  This notification was
    // for some other entry, or for the rename that produced this inode
    // (and which we'll learn about from the file FD).
    return false;
}

void KqueueFileWatcher::FireDirty(const absl::flat_hash_set<WatchId>& dirty)
{
    std::vector<std::function<void()>> callbacks;
    {
        std::lock_guard lock(m_mutex);
        for (auto id : dirty)
        {
            auto watch = m_watches.find(id);
            if (watch != m_watches.end())
            {
                Watch& w = watch->second;
                callbacks.emplace_back([cb = w.callback, path = w.path]() { cb(path); });
            }
        }
    }
    
    for (auto& callback : callbacks)
    {
        callback();
    }
}

} // namespace birch::util

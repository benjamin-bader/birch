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

#include "util/WinFileWatcher.h"

#include <chrono>
#include <cstring>
#include <latch>
#include <optional>
#include <string_view>
#include <utility>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"

namespace fs = std::filesystem;

namespace birch::util {

namespace {

constexpr DWORD kNotifyFilter =
    FILE_NOTIFY_CHANGE_FILE_NAME |
    FILE_NOTIFY_CHANGE_DIR_NAME  |
    FILE_NOTIFY_CHANGE_ATTRIBUTES |
    FILE_NOTIFY_CHANGE_SIZE |
    FILE_NOTIFY_CHANGE_LAST_WRITE |
    FILE_NOTIFY_CHANGE_CREATION;

constexpr auto kCoalesceWindow = std::chrono::milliseconds(50);

// Completion key sentinels for the IOCP.
constexpr ULONG_PTR kKeyIo = 0;
constexpr ULONG_PTR kKeyWake = 1;

absl::Status WinErrorToStatus(DWORD error, std::string_view context)
{
    return absl::InternalError(absl::StrCat(context, ": GetLastError=", error));
}

// In-place case fold via the Win32 case-folding routine. Used both for the
// stored map keys (filename and parent directory) and for the incoming
// filenames in FILE_NOTIFY_INFORMATION records. NTFS is case-insensitive but
// case-preserving, and RDCW may report either the long or 8.3 form, so we
// can't rely on byte equality.
std::wstring ToLowerKey(std::wstring s)
{
    if (!s.empty())
    {
        ::CharLowerBuffW(s.data(), static_cast<DWORD>(s.size()));
    }
    return s;
}

} // namespace

// ---------------------------------------------------------------------------
// WinFileWatcher::WatchHandle
// ---------------------------------------------------------------------------

class WinFileWatcher::WatchHandle : public IFileWatcher::IWatchHandle
{
    WatchId m_id;
    std::weak_ptr<WinFileWatcher> m_watcher;

public:
    WatchHandle(WatchId id, std::weak_ptr<WinFileWatcher> watcher)
        : m_id(id)
        , m_watcher(std::move(watcher))
    {
    }

    ~WatchHandle() override
    {
        Unwatch();
    }

    void Unwatch() override
    {
        if (auto w = m_watcher.lock())
        {
            w->CloseWatch(m_id);
            m_watcher.reset();
        }
    }
};

// ---------------------------------------------------------------------------
// WinDirWatcher
// ---------------------------------------------------------------------------

absl::StatusOr<std::shared_ptr<WinDirWatcher>>
WinDirWatcher::Open(const fs::path& dir, HANDLE iocp)
{
    HANDLE h = ::CreateFileW(
        dir.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);
    if (h == INVALID_HANDLE_VALUE)
    {
        return WinErrorToStatus(::GetLastError(), "CreateFileW(dir)");
    }

    if (::CreateIoCompletionPort(h, iocp, kKeyIo, 0) == nullptr)
    {
        DWORD e = ::GetLastError();
        ::CloseHandle(h);
        return WinErrorToStatus(e, "CreateIoCompletionPort(dir)");
    }

    auto dw = std::shared_ptr<WinDirWatcher>(new WinDirWatcher(dir, h));
    dw->m_io.owner = dw.get();
    return dw;
}

WinDirWatcher::WinDirWatcher(const fs::path& dir, HANDLE handle)
    : m_dirHandle(handle)
    , m_dir(dir)
{
}

WinDirWatcher::~WinDirWatcher()
{
    if (m_dirHandle != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_dirHandle);
        m_dirHandle = INVALID_HANDLE_VALUE;
    }
}

bool WinDirWatcher::IssueRead()
{
    if (m_dirHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    std::memset(&m_io.overlapped, 0, sizeof(m_io.overlapped));

    BOOL ok = ::ReadDirectoryChangesW(
        m_dirHandle,
        m_buffer,
        static_cast<DWORD>(sizeof(m_buffer)),
        FALSE, // bWatchSubtree
        kNotifyFilter,
        nullptr, // lpBytesReturned (unused for async)
        &m_io.overlapped,
        nullptr); // lpCompletionRoutine

    if (!ok)
    {
        LOG(ERROR) << "ReadDirectoryChangesW failed: GetLastError=" << ::GetLastError();
        return false;
    }

    return true;
}

void WinDirWatcher::Cancel()
{
    if (m_dirHandle != INVALID_HANDLE_VALUE)
    {
        ::CancelIoEx(m_dirHandle, &m_io.overlapped);
    }
}

void WinDirWatcher::Add(std::wstring fileNameKey, Entry entry)
{
    m_watches[std::move(fileNameKey)].push_back(std::move(entry));
}

bool WinDirWatcher::Remove(WatchId id, const std::wstring& fileNameKey)
{
    auto it = m_watches.find(fileNameKey);
    if (it == m_watches.end())
    {
        return false;
    }

    auto removed = std::erase_if(it->second, [&](const Entry& e) { return e.id == id; });
    if (it->second.empty())
    {
        m_watches.erase(it);
    }
    return removed > 0;
}

void WinDirWatcher::CollectDirty(absl::flat_hash_set<WatchId>& dirty, DWORD bytesTransferred) const
{
    if (bytesTransferred == 0)
    {
        // Buffer overflow indicator from RDCW: the change queue overran our
        // buffer and the OS dropped records. Treat every watch in this dir
        // as potentially affected.
        CollectAllDirty(dirty);
        return;
    }

    const BYTE* p = m_buffer;
    const BYTE* end = m_buffer + bytesTransferred;

    while (p + sizeof(FILE_NOTIFY_INFORMATION) <= end)
    {
        const auto* fni = reinterpret_cast<const FILE_NOTIFY_INFORMATION*>(p);
        std::wstring_view name(fni->FileName, fni->FileNameLength / sizeof(WCHAR));
        std::wstring key = ToLowerKey(std::wstring(name));

        if (auto it = m_watches.find(key); it != m_watches.end())
        {
            for (const auto& e : it->second)
            {
                dirty.insert(e.id);
            }
        }

        if (fni->NextEntryOffset == 0)
        {
            break;
        }
        p += fni->NextEntryOffset;
    }
}

void WinDirWatcher::CollectAllDirty(absl::flat_hash_set<WatchId>& dirty) const
{
    for (const auto& [_, vec] : m_watches)
    {
        for (const auto& e : vec)
        {
            dirty.insert(e.id);
        }
    }
}

void WinDirWatcher::ForEachEntry(WatchId id, const EntryConsumer& fn) const
{
    for (const auto& [_, vec] : m_watches)
    {
        for (const auto& e : vec)
        {
            if (e.id == id)
            {
                fn(e);
                return;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// WinFileWatcher
// ---------------------------------------------------------------------------

absl::StatusOr<std::shared_ptr<WinFileWatcher>> WinFileWatcher::Create()
{
    HANDLE iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1);
    if (iocp == nullptr)
    {
        return WinErrorToStatus(::GetLastError(), "CreateIoCompletionPort");
    }

    auto fw = std::shared_ptr<WinFileWatcher>(new WinFileWatcher(iocp));

    std::latch ready(1);
    fw->m_worker = std::thread([&ready, raw = fw.get()]()
    {
        ready.count_down();
        try
        {
            raw->Run();
        }
        catch (const std::exception& e)
        {
            LOG(ERROR) << "WinFileWatcher thread caught exception: " << e.what();
        }
        catch (...)
        {
            LOG(ERROR) << "WinFileWatcher thread caught unknown exception";
        }
    });
    ready.wait();

    return fw;
}

WinFileWatcher::WinFileWatcher(HANDLE iocp)
    : m_iocp(iocp)
{
}

WinFileWatcher::~WinFileWatcher()
{
    m_shutdown.store(true, std::memory_order_relaxed);
    Wake();

    if (m_worker.joinable())
    {
        m_worker.join();
    }

    if (m_iocp != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_iocp);
        m_iocp = INVALID_HANDLE_VALUE;
    }
}

void WinFileWatcher::Wake()
{
    ::PostQueuedCompletionStatus(m_iocp, 0, kKeyWake, nullptr);
}

absl::StatusOr<std::unique_ptr<IFileWatcher::IWatchHandle>>
WinFileWatcher::WatchFile(const fs::path& path, Callback callback)
{
    auto canon = fs::absolute(path).lexically_normal();
    auto dir = canon.parent_path();
    auto file = canon.filename();
    if (file.empty())
    {
        return absl::InvalidArgumentError("WatchFile: empty filename");
    }
    if (dir.empty())
    {
        dir = fs::current_path();
    }

    auto dirKey = ToLowerKey(dir.wstring());
    auto fileNameKey = ToLowerKey(file.wstring());
    auto id = m_nextWatchId.fetch_add(1, std::memory_order_relaxed);

    // Held through the (possibly newly-issued) RDCW call so a second WatchFile
    // for the same directory can't be visible to the worker before the read
    // is armed, and so a failure here cleans up only our own state.
    std::lock_guard lock(m_mutex);

    std::shared_ptr<WinDirWatcher> dw;
    bool newlyOpened = false;

    if (auto it = m_dirWatchers.find(dirKey); it != m_dirWatchers.end())
    {
        dw = it->second;
    }
    else
    {
        auto opened = WinDirWatcher::Open(dir, m_iocp);
        if (!opened.ok())
        {
            return opened.status();
        }
        dw = *opened;
        m_dirWatchers.emplace(dirKey, dw);
        newlyOpened = true;
    }

    dw->Add(fileNameKey, WinDirWatcher::Entry{id, canon, std::move(callback)});
    m_watchLocations.emplace(id, WatchLocation{dirKey, fileNameKey});

    if (newlyOpened && !dw->IssueRead())
    {
        dw->Remove(id, fileNameKey);
        m_watchLocations.erase(id);
        m_dirWatchers.erase(dirKey);
        return absl::InternalError("ReadDirectoryChangesW initial issue failed");
    }

    return std::make_unique<WatchHandle>(id, weak_from_this());
}

void WinFileWatcher::CloseWatch(WatchId id)
{
    {
        std::lock_guard lock(m_mutex);
        m_pendingUnwatches.push_back(id);
    }
    Wake();
}

void WinFileWatcher::DrainPendingUnwatches()
{
    std::vector<std::shared_ptr<WinDirWatcher>> toCancel;
    {
        std::lock_guard lock(m_mutex);
        for (auto id : m_pendingUnwatches)
        {
            auto locIt = m_watchLocations.find(id);
            if (locIt == m_watchLocations.end())
            {
                continue;
            }

            auto dirKey = locIt->second.dirKey;
            auto fileNameKey = locIt->second.fileNameKey;
            m_watchLocations.erase(locIt);

            auto dirIt = m_dirWatchers.find(dirKey);
            if (dirIt == m_dirWatchers.end())
            {
                continue;
            }

            dirIt->second->Remove(id, fileNameKey);

            if (dirIt->second->Empty())
            {
                auto dw = dirIt->second;
                m_dirWatchers.erase(dirIt);
                m_draining.emplace(dw.get(), dw);
                toCancel.push_back(std::move(dw));
            }
        }
        m_pendingUnwatches.clear();
    }

    for (auto& dw : toCancel)
    {
        dw->Cancel();
    }
}

void WinFileWatcher::FireDirty(const absl::flat_hash_set<WatchId>& dirty)
{
    std::vector<std::function<void()>> callbacks;
    {
        std::lock_guard lock(m_mutex);
        for (auto id : dirty)
        {
            auto locIt = m_watchLocations.find(id);
            if (locIt == m_watchLocations.end())
            {
                continue;
            }
            auto dirIt = m_dirWatchers.find(locIt->second.dirKey);
            if (dirIt == m_dirWatchers.end())
            {
                continue;
            }

            dirIt->second->ForEachEntry(id, [&](const WinDirWatcher::Entry& e)
            {
                callbacks.emplace_back([cb = e.callback, p = e.reportPath]() { cb(p); });
            });
        }
    }

    for (auto& cb : callbacks)
    {
        cb();
    }
}

void WinFileWatcher::Run()
{
    absl::flat_hash_set<WatchId> dirty;
    std::optional<std::chrono::steady_clock::time_point> deadline;
    bool shuttingDown = false;

    while (true)
    {
        DrainPendingUnwatches();

        if (!shuttingDown && m_shutdown.load(std::memory_order_relaxed))
        {
            shuttingDown = true;

            std::vector<std::shared_ptr<WinDirWatcher>> toCancel;
            {
                std::lock_guard lock(m_mutex);
                for (auto& [_, dw] : m_dirWatchers)
                {
                    m_draining.emplace(dw.get(), dw);
                    toCancel.push_back(dw);
                }
                m_dirWatchers.clear();
                m_watchLocations.clear();
            }
            for (auto& dw : toCancel)
            {
                dw->Cancel();
            }

            // Any pending dirty callbacks are dropped on shutdown; we hand
            // back to whoever destroyed us as quickly as we can.
            dirty.clear();
            deadline.reset();
        }

        if (shuttingDown)
        {
            std::lock_guard lock(m_mutex);
            if (m_draining.empty())
            {
                break;
            }
        }

        DWORD timeoutMs = INFINITE;
        if (deadline)
        {
            auto now = std::chrono::steady_clock::now();
            if (now >= *deadline)
            {
                timeoutMs = 0;
            }
            else
            {
                timeoutMs = static_cast<DWORD>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(*deadline - now).count());
            }
        }

        DWORD bytes = 0;
        ULONG_PTR key = 0;
        OVERLAPPED* ov = nullptr;
        BOOL ok = ::GetQueuedCompletionStatus(m_iocp, &bytes, &key, &ov, timeoutMs);
        DWORD error = ok ? ERROR_SUCCESS : ::GetLastError();

        if (!ok && ov == nullptr)
        {
            if (error != WAIT_TIMEOUT)
            {
                LOG(WARNING) << "GetQueuedCompletionStatus failed: GetLastError=" << error;
            }
            // Either a timeout (drop into FireDirty below) or a non-fatal
            // error; loop back regardless.
        }
        else if (key == kKeyWake && ov == nullptr)
        {
            // Just a wake from CloseWatch or the destructor; the next top-of-
            // loop pass picks up pending unwatches and the shutdown flag.
        }
        else if (ov != nullptr)
        {
            auto* io = reinterpret_cast<WinDirIo*>(ov);
            WinDirWatcher* dir = io->owner;

            // Hold the dir watcher alive for the duration of this iteration.
            std::shared_ptr<WinDirWatcher> live;
            bool draining = false;
            {
                std::lock_guard lock(m_mutex);
                if (auto it = m_draining.find(dir); it != m_draining.end())
                {
                    live = it->second;
                    m_draining.erase(it);
                    draining = true;
                }
                else
                {
                    // Search the active set. Linear, but the count of watched
                    // directories is expected to be small.
                    for (auto& [_, dw] : m_dirWatchers)
                    {
                        if (dw.get() == dir)
                        {
                            live = dw;
                            break;
                        }
                    }
                }
            }

            if (!live)
            {
                continue;
            }

            if (draining)
            {
                // Any completion (real or ERROR_OPERATION_ABORTED) is the
                // final one — we never re-issue while draining. Dropping
                // `live` here triggers ~WinDirWatcher and closes the handle.
                continue;
            }

            if (!ok)
            {
                if (error == ERROR_OPERATION_ABORTED)
                {
                    // Shouldn't happen for an active watcher, but be defensive.
                    continue;
                }
                LOG(ERROR) << "RDCW completion error: GetLastError=" << error;
                continue;
            }

            {
                std::lock_guard lock(m_mutex);
                live->CollectDirty(dirty, bytes);
            }

            if (!live->IssueRead())
            {
                // Re-issue failed: take the watcher out of active rotation.
                std::lock_guard lock(m_mutex);
                for (auto it = m_dirWatchers.begin(); it != m_dirWatchers.end(); ++it)
                {
                    if (it->second.get() == live.get())
                    {
                        m_dirWatchers.erase(it);
                        break;
                    }
                }
            }

            if (!dirty.empty() && !deadline)
            {
                deadline = std::chrono::steady_clock::now() + kCoalesceWindow;
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

} // namespace birch::util

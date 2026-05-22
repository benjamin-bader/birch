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

#ifndef BIRCH_UTIL_KQUEUEFILEWATCHER_H
#define BIRCH_UTIL_KQUEUEFILEWATCHER_H

#include "IFileWatcher.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"

namespace birch::util {

class KqueueFileWatcher : public IFileWatcher, public std::enable_shared_from_this<KqueueFileWatcher>
{
public:
    using WatchId = std::uint64_t;

private:
    class WatchHandle : public IFileWatcher::IWatchHandle
    {
        WatchId id;
        std::weak_ptr<KqueueFileWatcher> watcher;

    public:
        WatchHandle(WatchId id, std::weak_ptr<KqueueFileWatcher>&& watcher);

        ~WatchHandle() override;

        void Unwatch() override;
    };

    struct Watch
    {
        WatchId id;
        std::filesystem::path path;
        std::filesystem::path parentPath;
        int fileFd;
        int dirFd;
        Callback callback;

        Watch() = default;
        Watch(const Watch&) = default;
        Watch(Watch&&) = default;
        Watch& operator=(const Watch&) = default;
        Watch& operator=(Watch&&) = default;

        Watch(WatchId id, const std::filesystem::path& path, const std::filesystem::path& parentPath, int fileFd, int dirFd, Callback&& callback);
    };

    int m_kqueue;
    std::thread m_thread;
    std::atomic<bool> m_shutdown{false};
    std::atomic<WatchId> m_nextWatchId{1};
    std::mutex m_mutex;
    absl::flat_hash_map<WatchId, Watch> m_watches;
    std::vector<WatchId> m_pendingUnwatches;

public:
    static absl::StatusOr<std::shared_ptr<KqueueFileWatcher>> Create();

    ~KqueueFileWatcher() override;

    absl::StatusOr<std::unique_ptr<IWatchHandle>> WatchFile(const std::filesystem::path& path, Callback callback) override;

private:
    explicit KqueueFileWatcher(int kqueue);

    void Run();
    void Wake();
    void CloseWatch(WatchId id);
    void DisarmWatch(Watch& watch);

    bool ArmFile(Watch& watch);
    bool ArmDir(Watch& watch);
    void DisarmFile(Watch& watch);
    void DisarmDir(Watch& watch);
    bool TryOpenAndArmFile(Watch& watch);

    bool HandleFileEvent(Watch& watch, std::uint32_t fflags);
    bool HandleDirEvent(Watch& watch);

    void FireDirty(const absl::flat_hash_set<WatchId>& dirty);
};

} // namespace birch::util

#endif // BIRCH_UTIL_KQUEUEFILEWATCHER_H

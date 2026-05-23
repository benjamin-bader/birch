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

#ifndef BIRCH_UTIL_WINFILEWATCHER_H
#define BIRCH_UTIL_WINFILEWATCHER_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/status/statusor.h"

#include "util/IFileWatcher.h"

namespace birch::util {

class WinDirWatcher;

// Standard-layout wrapper we hand to ReadDirectoryChangesW. OVERLAPPED must be
// the first member so we can reinterpret_cast back from the OVERLAPPED* that
// GetQueuedCompletionStatus returns.
struct WinDirIo
{
    OVERLAPPED overlapped;
    WinDirWatcher* owner;
};

class WinFileWatcher
    : public IFileWatcher
    , public std::enable_shared_from_this<WinFileWatcher>
{
public:
    using WatchId = std::uint64_t;

    static absl::StatusOr<std::shared_ptr<WinFileWatcher>> Create();

    ~WinFileWatcher() override;

    absl::StatusOr<std::unique_ptr<IWatchHandle>>
        WatchFile(const std::filesystem::path& path, Callback callback) override;

private:
    class WatchHandle;
    friend class WinDirWatcher;

    struct WatchLocation
    {
        std::wstring dirKey;
        std::wstring fileNameKey;
    };

    explicit WinFileWatcher(HANDLE iocp);

    void Run();
    void Wake();

    void CloseWatch(WatchId id);
    void DrainPendingUnwatches();
    void FireDirty(const absl::flat_hash_set<WatchId>& dirty);

    HANDLE m_iocp{INVALID_HANDLE_VALUE};
    std::thread m_worker;
    std::atomic<bool> m_shutdown{false};
    std::atomic<WatchId> m_nextWatchId{1};

    std::mutex m_mutex;
    absl::flat_hash_map<std::wstring, std::shared_ptr<WinDirWatcher>> m_dirWatchers;
    absl::flat_hash_map<WatchId, WatchLocation> m_watchLocations;
    std::vector<WatchId> m_pendingUnwatches;

    // Dir watchers whose IO has been canceled and which are waiting for the
    // final (likely ERROR_OPERATION_ABORTED) completion before they can be
    // destroyed. Keeping the shared_ptr here keeps the buffer and handle alive
    // until the OS is finished with them.
    absl::flat_hash_map<WinDirWatcher*, std::shared_ptr<WinDirWatcher>> m_draining;
};

class WinDirWatcher
{
public:
    using WatchId = WinFileWatcher::WatchId;

    struct Entry
    {
        WatchId id;
        std::filesystem::path reportPath;
        IFileWatcher::Callback callback;
    };

    static absl::StatusOr<std::shared_ptr<WinDirWatcher>>
        Open(const std::filesystem::path& dir, HANDLE iocp);

    ~WinDirWatcher();

    WinDirWatcher(const WinDirWatcher&) = delete;
    WinDirWatcher& operator=(const WinDirWatcher&) = delete;

    bool IssueRead();
    void Cancel();

    void Add(std::wstring fileNameKey, Entry entry);
    bool Remove(WatchId id, const std::wstring& fileNameKey);
    bool Empty() const { return m_watches.empty(); }

    // Called by the IOCP worker when bytes were transferred (events parsed
    // from the buffer) or when the buffer overflowed (bytes == 0).
    void CollectDirty(absl::flat_hash_set<WatchId>& dirty, DWORD bytesTransferred) const;
    void CollectAllDirty(absl::flat_hash_set<WatchId>& dirty) const;

    using EntryConsumer = std::function<void(const Entry&)>;
    void ForEachEntry(WatchId id, const EntryConsumer& fn) const;

private:
    WinDirWatcher(const std::filesystem::path& dir, HANDLE handle);

    static constexpr std::size_t kBufferBytes = 32 * 1024;

    WinDirIo m_io{};
    HANDLE m_dirHandle{INVALID_HANDLE_VALUE};
    std::filesystem::path m_dir;
    alignas(DWORD) BYTE m_buffer[kBufferBytes]{};

    // Lowercased filename -> entries.
    absl::flat_hash_map<std::wstring, std::vector<Entry>> m_watches;
};

} // namespace birch::util

#endif // BIRCH_UTIL_WINFILEWATCHER_H

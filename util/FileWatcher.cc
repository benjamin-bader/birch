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

#include <functional>
#include <memory>
#include <mutex>
#include <utility>

#include "util/IFileWatcher.h"
#include "util/NoopFileWatcher.h"

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace birch::util {

namespace {

std::once_flag g_onceFlag;
std::shared_ptr<IFileWatcher> g_fileWatcher{nullptr};
absl::Status g_status;

} // namespace

absl::Status InitializeGlobalFileWatcher()
{
    std::call_once(g_onceFlag, [&]() {
        auto maybeFileWatcher = IFileWatcher::Create();
        if (maybeFileWatcher.ok())
        {
            g_fileWatcher = std::move(*maybeFileWatcher);
        }
        else
        {
            g_status = maybeFileWatcher.status();
            g_fileWatcher = std::make_shared<NoopFileWatcher>();
            LOG(ERROR) << "Failed to initialize file-watcher, file change detection disabled!  " << g_status;
        }
    });

    return g_status;
}

absl::Status InitializeGlobalFileWatcherForTesting()
{
    std::call_once(g_onceFlag, [&]() {
        g_fileWatcher = std::make_shared<NoopFileWatcher>();
        g_status = absl::OkStatus();
    });
    return g_status;
}

IFileWatcher* GetGlobalFileWatcher()
{
    return g_fileWatcher.get();
}

} // namespace birch::util

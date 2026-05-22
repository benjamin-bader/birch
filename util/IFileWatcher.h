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

#ifndef BIRCH_UTIL_IFILEWATCHER_H
#define BIRCH_UTIL_IFILEWATCHER_H

#include <filesystem>
#include <functional>
#include <memory>

#include "absl/status/statusor.h"

namespace birch::util {

class IFileWatcher
{
public:
    using Callback = std::function<void(const std::filesystem::path&)>;

    class IWatchHandle
    {
    public:
        virtual ~IWatchHandle() = default;

        virtual void Unwatch() = 0;
    };

    static absl::StatusOr<std::shared_ptr<IFileWatcher>> Create();

    virtual ~IFileWatcher() = default;

    virtual absl::StatusOr<std::unique_ptr<IWatchHandle>> WatchFile(
        const std::filesystem::path& path,
        Callback callback
    ) = 0;
};

} // namespace birch::util

#endif // BIRCH_UTIL_IFILEWATCHER_H

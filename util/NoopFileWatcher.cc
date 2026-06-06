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

#include "NoopFileWatcher.h"

#include <filesystem>
#include <memory>

#include "absl/status/statusor.h"

namespace birch::util
{

namespace
{

class WatchHandle : public IFileWatcher::IWatchHandle
{
public:
    ~WatchHandle() override = default;
    void Unwatch() override {}
};

} // namespace

absl::StatusOr<std::unique_ptr<IFileWatcher::IWatchHandle>> NoopFileWatcher::WatchFile(
    const std::filesystem::path& path,
    IFileWatcher::Callback callback
)
{
    return std::make_unique<WatchHandle>();
}

} // namespace birch::util

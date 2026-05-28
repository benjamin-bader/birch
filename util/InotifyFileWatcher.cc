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

#include "util/InotifyFileWatcher.h"

#include <cstdlib>

#include <sys/inotify.h>
#include <unistd.h>

namespace birch::util {

absl::StatusOr<std::shared_ptr<IFileWatcher>> IFileWatcher::Create()
{
    return InotifyFileWatcher::Create();
}

absl::StatusOr<std::shared_ptr<InotifyFileWatcher>> InotifyFileWatcher::Create()
{
    int inotify = ::inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (inotify == -1)
    {
        return absl::ErrnoToStatus(errno, "inotify_init1() failed");
    }

    std::shared_ptr<InotifyFileWatcher> fw(new InotifyFileWatcher(inotify));

    return fw;
}

absl::StatusOr<std::unique_ptr<IFileWatcher::IWatchHandle>> InotifyFileWatcher::WatchFile(const std::filesystem::path& path, Callback callback)
{
    return absl::UnimplementedError("WatchFile not implemented for InotifyFileWatcher");
}

InotifyFileWatcher::InotifyFileWatcher(int inotify)
    : m_inotify(inotify)
{
}

InotifyFileWatcher::~InotifyFileWatcher()
{
    if (m_inotify != -1)
    {
        ::close(m_inotify);
        m_inotify = -1;
    }
}

} // namespace birch::util

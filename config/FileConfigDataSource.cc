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

#include "FileConfigDataSource.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <string>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"

namespace birch::config {

absl::StatusOr<std::unique_ptr<IConfigDataSource>> FileConfigDataSource::CreateFromFile(const std::filesystem::path& path)
{
    // TODO:
    // - Validate that the file exists and is suitable for a config source
    // - Create a watcher for the file

    return std::make_unique<FileConfigDataSource>(path);
}

FileConfigDataSource::FileConfigDataSource(const std::filesystem::path& path)
    : m_path(path)
{
}

absl::StatusOr<std::string> FileConfigDataSource::Read() const
{
    std::ifstream file(m_path);
    if (!file.is_open())
    {
        return absl::Status(
            absl::StatusCode::kNotFound, 
            absl::StrFormat("Failed to open file %s: %s", m_path.string(), std::strerror(errno)));
    }

    std::string contents;
    contents.assign(std::istreambuf_iterator<char>(file), {});
    return contents;
}

void FileConfigDataSource::Subscribe(Callback&& callback)
{
    m_watchers.push_back(std::move(callback));
}

void FileConfigDataSource::OnFileChanged()
{
    NotifyWatchers();
}

void FileConfigDataSource::NotifyWatchers()
{
    for (const auto& watcher : m_watchers)
    {
        watcher();
    }
}

} // namespace birch

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
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"

#include "util/IFileWatcher.h"

namespace birch::config {

absl::StatusOr<std::unique_ptr<IConfigDataSource>> FileConfigDataSource::CreateFromFile(const std::filesystem::path& path)
{
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        if (ec) {
            return absl::ErrnoToStatus(ec.value(), "is_regular_file() failed");
        }
        return absl::InvalidArgumentError(absl::StrFormat("File %s is not a regular file", path.string()));
    }

    auto source = std::make_unique<FileConfigDataSource>(path);
    auto status = source->InitializeFileWatch();
    if (!status.ok()) {
        return status;
    }

    return std::move(source);
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
    std::lock_guard lock(m_mutex);
    m_callbacks.push_back(std::move(callback));
}

absl::Status FileConfigDataSource::InitializeFileWatch()
{
    auto handle = util::GetGlobalFileWatcher()->WatchFile(m_path, [this](const std::filesystem::path& path) {
        this->OnFileChanged();
    });

    if (!handle.ok()) {
        return handle.status();
    }

    m_watchHandle = std::move(*handle);

    return absl::OkStatus();
}

void FileConfigDataSource::OnFileChanged()
{
    NotifyWatchers();
}

void FileConfigDataSource::NotifyWatchers()
{
    std::vector<Callback> callbacks;
    {
        std::lock_guard lock(m_mutex);
        callbacks = m_callbacks; // copy that floppy
    }

    for (const auto& cb : callbacks)
    {
        cb();
    }
}

} // namespace birch

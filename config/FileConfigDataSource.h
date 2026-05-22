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

#ifndef BIRCH_CONFIG_FILECONFIGDATASOURCE_H
#define BIRCH_CONFIG_FILECONFIGDATASOURCE_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "absl/status/statusor.h"

#include "IConfigSource.h"

namespace birch::config {

class FileConfigDataSource : public IConfigDataSource
{
    std::filesystem::path m_path;
    std::vector<Callback> m_watchers;

public:
    static absl::StatusOr<std::unique_ptr<IConfigDataSource>> CreateFromFile(const std::filesystem::path& path);

    FileConfigDataSource(const std::filesystem::path& path);

    absl::StatusOr<std::string> Read() const override;

    void Subscribe(Callback&& callback) override;

private:
    void OnFileChanged();

    void NotifyWatchers();
};

} // namespace birch

#endif // BIRCH_CONFIG_FILECONFIGDATASOURCE_H
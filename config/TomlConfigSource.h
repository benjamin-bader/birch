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

#ifndef BIRCH_CONFIG_CONFIGSOURCE_H
#define BIRCH_CONFIG_CONFIGSOURCE_H

#include <filesystem>
#include <memory>
#include <string>

#include "IConfigSource.h"

namespace birch {

class TomlConfigSource : public IConfigSource
{
    std::filesystem::path m_path;

public:
    TomlConfigSource(const std::filesystem::path& path);

    static absl::StatusOr<std::unique_ptr<IConfigSource>> CreateFromTomlFile(const std::filesystem::path& path);

    absl::StatusOr<KeyValuePairs> LoadConfigValues() const override;
};

} // namespace birch

#endif // BIRCH_CONFIG_CONFIGSOURCE_H
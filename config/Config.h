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

#ifndef BIRCH_CONFIG_CONFIG_H
#define BIRCH_CONFIG_CONFIG_H

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "IConfigSource.h"
#include "IValueSource.h"

namespace birch {

class IConfig
{
public:
    static std::shared_ptr<IConfig> CreateFromSource(std::unique_ptr<IConfigSource>&& source);

    virtual ~IConfig() = default;

    /**
     * Reloads the configuration from the source.
     */
    virtual void Reload() = 0;

    virtual std::shared_ptr<IValueSource<std::int64_t>> GetInt64(const std::string& key) = 0;
    virtual std::shared_ptr<IValueSource<std::string>> GetString(const std::string& key) = 0;
    virtual std::shared_ptr<IValueSource<bool>> GetBool(const std::string& key) = 0;
    virtual std::shared_ptr<IValueSource<double>> GetDouble(const std::string& key) = 0;

    virtual std::shared_ptr<IConfig> GetSection(const std::string& key) = 0;
};

}

#endif // BIRCH_CONFIG_CONFIG_H
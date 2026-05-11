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

#ifndef BIRCH_CONFIG_ICONFIGSOURCE_H
#define BIRCH_CONFIG_ICONFIGSOURCE_H

#include <string>
#include <utility>
#include <vector>

#include "absl/status/statusor.h"

namespace birch {

/**
 * An IConfigSource is an object that can load configuration values.
 *
 * Configuration values are key-value string pairs.  Keys are not guaranteed to be
 * unique.  Keys can express a hierarchy by separating components with a period,
 * for example, "server.port" and "server.tls_port" would be two distinct keys residing
 * under the same parent key "server".
 *
 * No guarantees are made about the underlying format or storage mechanism of the configuration
 * values.  Keys are guaranteed to be valid UTF-8; values have no such guarantee at all.
 */
class IConfigSource
{
public:
    using KeyValuePair = std::pair<std::string, std::string>;
    using KeyValuePairs = std::vector<KeyValuePair>;

    virtual ~IConfigSource() = default;

    /**
     * Loads the configuration values from the source.
     *
     * @return A vector of key-value pairs, or an error if the configuration values cannot be loaded.
     */
    virtual absl::StatusOr<KeyValuePairs> LoadConfigValues() const = 0;
};

}

#endif // BIRCH_CONFIG_CONFIGSOURCE_H

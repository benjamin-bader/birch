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

#include <functional>
#include <string>

#include "absl/status/statusor.h"

namespace birch::config {

/**
 * An IConfigDataSource is an object that can read configuration values from a source.
 *
 * The source is typically a file, but doesn't have to be!
 *
 * The source is read as a string, and the string is then used to load the configuration values.
 *
 * The source can be subscribed to changes, and the callback will be called whenever the source changes.
 *
 * Example usage:
 *
 *   // Create a data source
 *   std::unique_ptr<IConfigDataSource> source = ...;
 *
 *   auto contents = source->Read();
 *   if (!contents.ok()) {
 *       LOG(ERROR) << "Failed to read configuration from source: " << contents.status().message();
 *       return;
 *   }
 *   std::string config_contents = *contents;
 *
 *   // Subscribe to changes
 *   source->Subscribe([]() {
 *       // Handle configuration source changes here
 *   });
 */
class IConfigDataSource
{
public:
    using Callback = std::function<void()>;

    virtual ~IConfigDataSource() = default;

    virtual absl::StatusOr<std::string> Read() const = 0;

    /**
     * Subscribe to changes to the configuration source.  When the source changes, the callback
     * will be invoked, and at that point the source will contain up-to-date data.
     *
     * NOTE: This method is still in flux and is probably not implemented yet.
     */
    virtual void Subscribe(Callback&& callback) = 0;
};

}

#endif // BIRCH_CONFIG_CONFIGSOURCE_H

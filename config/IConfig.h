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

#ifndef BIRCH_CONFIG_ICONFIG_H
#define BIRCH_CONFIG_ICONFIG_H

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "ConfigPath.h"
#include "IConfigSource.h"

namespace birch::config {

class IValue;

class IValueList
{
public:
    virtual ~IValueList() = default;

    virtual std::size_t Size() const = 0;
    virtual std::shared_ptr<IValue> Get(std::size_t index) const = 0;
};

class IValueMap
{
public:
    virtual ~IValueMap() = default;

    virtual std::size_t Size() const = 0;
    virtual std::shared_ptr<IValue> Get(const std::string& key) const = 0;
};

/**
 * An IValue is a value in the configuration.  It can be a boolean, an integer, a double, a string,
 * a list, or a map.
 */
class IValue
{
public:
    virtual ~IValue() = default;

    virtual std::optional<bool> AsBool() const = 0;
    virtual std::optional<std::int64_t> AsInt64() const = 0;
    virtual std::optional<double> AsDouble() const = 0;
    virtual std::optional<std::string> AsString() const = 0;
    virtual std::shared_ptr<IValueList> AsArray() const = 0;
    virtual std::shared_ptr<IValueMap> AsMap() const = 0;
};

/**
 * An IConfigNode is a node in the configuration tree, possibly bearing a value.  It is a path to a value in the configuration,
 * and it can be subscribed to for changes.
 *
 * ## Subscriptions
 * 
 * Subscriptions are used to be notified when the value of a node changes.  The callback will be called with the path to the node and the new value.
 * The callback will be called asynchronously, and the caller is responsible for ensuring that the callback is thread-safe.
 *
 * Subscriptions are not supported for lists.  If a node is a list, the callback will not be called when the list changes.
 * Instead, subscribe to the parent node that contains the list.
 *
 * Notifications will always be sent if the value of the node changes, and might be sent even if it doesn't; callbacks should not assume that the value
 * is new.  If, after a reload, a node's path is no longer found, the callback will be invoked with a null value.
 */
class IConfigNode
{
public:
    virtual ~IConfigNode() = default;

    // GetPath returns the logical path to the node in the configuration tree.
    virtual const ConfigPath& GetPath() const = 0;

    // GetValue returns the value of the node, if any.
    virtual IValue* GetValue() = 0;

    // Subscribe registers a callback to be called when the value of the node changes.
    // The callback will be called with the path to the node and the new value.
    virtual void Subscribe(std::function<void(const ConfigPath&, const IValue*)>&& callback) = 0;
};

/**
 * An IConfig is the root of the configuration tree.  It can be reloaded, and it can be queried for nodes.
 */
class IConfig
{
public:
    virtual ~IConfig() = default;

    // Reload reloads the configuration from the source.
    virtual void Reload() = 0;

    // GetNode returns a node in the configuration tree.
    //
    // The path is a dot-separated string of keys, e.g. "server.port".  If the path is empty, the root node is returned.
    // If the given path is not found, nullptr is returned.
    virtual std::shared_ptr<IConfigNode> GetNode(const ConfigPath& path) = 0;
};

} // namespace birch::config

#endif // BIRCH_CONFIG_ICONFIG_H

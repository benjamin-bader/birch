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

#include "TomlConfig.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "absl/log/log.h"
#include "toml.hpp"

namespace birch::config {

namespace {

std::optional<TomlValue::Variant> NodeToVariant(const toml::node& node)
{
    if (node.is_array())
    {
        return TomlList::Create(*node.as_array());
    }
    else if (node.is_table())
    {
        return TomlMap::Create(*node.as_table());
    }
    else if (node.is_string())
    {
        return node.as_string()->get();
    }
    else if (node.is_integer())
    {
        return node.as_integer()->get();
    }
    else if (node.is_floating_point())
    {
        return node.as_floating_point()->get();
    }
    else if (node.is_boolean())
    {
        return node.as_boolean()->get();
    }
    else
    {
        LOG(ERROR) << "Unhandled TOML node type: " << absl::StrFormat("%v", absl::FormatStreamed(node.type()));
        return std::nullopt;
    }
}

std::shared_ptr<TomlValue> Dig(const std::shared_ptr<TomlValue>& value, const ConfigPath& path)
{
    if (value == nullptr)
    {
        return nullptr;
    }

    auto cur = value;
    for (const auto& segment : path)
    {
        if (cur == nullptr)
        {
            break;
        }

        auto map = cur->AsMap();
        if (map == nullptr)
        {
            return nullptr;
        }

        auto next = map->Get(segment);
        if (next == nullptr)
        {
            return nullptr;
        }

        auto tomlValue = std::static_pointer_cast<TomlValue>(next);
        if (tomlValue == nullptr)
        {
            return nullptr;
        }

        cur = tomlValue;
    }

    return cur;
}

} // namespace


// TomlList

std::shared_ptr<TomlList> TomlList::Create(const toml::array& array)
{
    std::vector<std::shared_ptr<TomlValue>> values;

    for (const auto& element : *array.as_array())
    {
        auto value = TomlValue::Create(element);
        if (value)
        {
            values.emplace_back(std::move(value));
        }
    }
    return std::make_shared<TomlList>(std::move(values));
}

TomlList::TomlList(std::vector<std::shared_ptr<TomlValue>>&& values)
    : m_values(std::move(values))
{
}

std::size_t TomlList::Size() const
{
    return m_values.size();
}

std::shared_ptr<IValue> TomlList::Get(std::size_t index) const
{
    return m_values.at(index);
}

// TomlMap

std::shared_ptr<TomlMap> TomlMap::Create(const toml::table& table)
{
    absl::flat_hash_map<std::string, std::shared_ptr<TomlValue>> values;
    table.for_each([&](const toml::key& key, const toml::node& value)
    {
        auto maybeValue = TomlValue::Create(value);
        if (maybeValue)
        {
            values.emplace(key.str(), std::move(maybeValue));
        }
    });
    return std::make_shared<TomlMap>(std::move(values));
}

TomlMap::TomlMap(absl::flat_hash_map<std::string, std::shared_ptr<TomlValue>>&& values)
    : m_values(std::move(values))
{
}

std::size_t TomlMap::Size() const
{
    return m_values.size();
}

std::shared_ptr<IValue> TomlMap::Get(const std::string& key) const
{
    auto it = m_values.find(key);
    if (it == m_values.end())
    {
        return nullptr;
    }
    return it->second;
}

// TomlValue


std::shared_ptr<TomlValue> TomlValue::Create(const toml::node& node)
{
    auto maybeValue = NodeToVariant(node);
    if (!maybeValue)
    {
        return nullptr;
    }
    return std::make_shared<TomlValue>(std::move(*maybeValue));
}

TomlValue::TomlValue()
    : m_value()
{
}   


TomlValue::TomlValue(const Variant& value)
    : m_value(value)
{
}

TomlValue::TomlValue(Variant&& value)
    : m_value(std::move(value))
{
}

std::optional<bool> TomlValue::AsBool() const
{
    if (std::holds_alternative<bool>(m_value))
    {
        return std::get<bool>(m_value);
    }
    return std::nullopt;
}

std::optional<std::int64_t> TomlValue::AsInt64() const
{
    if (std::holds_alternative<std::int64_t>(m_value))
    {
        return std::get<std::int64_t>(m_value);
    }
    return std::nullopt;
}

std::optional<double> TomlValue::AsDouble() const
{
    if (std::holds_alternative<double>(m_value))
    {
        return std::get<double>(m_value);
    }
    return std::nullopt;
}

std::optional<std::string> TomlValue::AsString() const
{
    if (std::holds_alternative<std::string>(m_value))
    {
        return std::get<std::string>(m_value);
    }
    return std::nullopt;
}

std::shared_ptr<IValueList> TomlValue::AsArray() const
{
    if (std::holds_alternative<std::shared_ptr<TomlList>>(m_value))
    {
        return std::get<std::shared_ptr<TomlList>>(m_value);
    }
    return nullptr;
}

std::shared_ptr<IValueMap> TomlValue::AsMap() const
{
    if (std::holds_alternative<std::shared_ptr<TomlMap>>(m_value))
    {
        return std::get<std::shared_ptr<TomlMap>>(m_value);
    }
    return nullptr;
}

void TomlValue::Reset(const toml::node& node)
{
    auto maybeValue = NodeToVariant(node);
    if (!maybeValue)
    {
        return;
    }

    m_value = std::move(*maybeValue);
}


// TomlNode

TomlNode::TomlNode(const ConfigPath& path, std::shared_ptr<TomlValue>&& value)
    : m_path(path)
    , m_value(std::move(value))
{
}

const ConfigPath& TomlNode::GetPath() const
{
    return m_path;
}

TomlValue* TomlNode::GetValue()
{
    return m_value.get();
}

void TomlNode::Subscribe(std::function<void(const ConfigPath&, const IValue*)>&& callback)
{
    // NOTE: We don't allow subscribing to lists!
    //
    // This is because lists are not _addressable_ in an easy-to-reason-about way across config
    // reloads.  It's possible that list entries are reoredered, or new entries are added, or entries
    // are removed, or entries are changed.  Without a stable way to identify an entry, it's quite
    // difficult to figure out what the nature of the change might have been.
    //
    // I'm sure it's possible, but I don't want to figure it out for this use-case.
    // Just subscribe to the thing that holds the list instead.
    //
    // BUG: What if, on reload, a map becomes a list?  Or vice versa?  How do we handle that?  Pathological.
    if (m_value->AsArray())
    {
        LOG(INFO) << "Skipping subscription to list node at " << m_path;
        return;
    }

    m_callbacks.push_back(std::move(callback));
}

void TomlNode::Notify()
{
    if (m_value->AsArray())
    {
        // See note in Subscribe() about why we don't allow subscribing to lists.
        LOG(INFO) << "Skipping notification for list node at " << m_path;
        return;
    }

    for (const auto& callback : m_callbacks)
    {
        callback(m_path, m_value.get());
    }
}

// TomlConfig

std::shared_ptr<TomlConfig> TomlConfig::Create(std::unique_ptr<IConfigDataSource>&& source)
{
    auto config = std::make_shared<TomlConfig>(std::move(source));
    config->Reload();
    return config;
}

TomlConfig::TomlConfig(std::unique_ptr<IConfigDataSource>&& source)
    : m_source{std::move(source)}
    , m_root{}
    , m_nodes{}
{
}

void TomlConfig::Reload()
{
    auto contents = m_source->Read();
    if (!contents.ok())
    {
        LOG(ERROR) << "Failed to read TOML config: " << contents.status();
        return;
    }

    toml::table table;
    try
    {
        table = toml::parse(*contents);
    }
    catch (const toml::parse_error& e) {
        LOG(ERROR) << "Failed to parse TOML: " << e.what();
        return;
    }

    m_root = TomlValue::Create(table);
}

std::shared_ptr<IConfigNode> TomlConfig::GetRootNode()
{
    ConfigPath root;
    return GetNode(root);
}

std::shared_ptr<IConfigNode> TomlConfig::GetNode(const ConfigPath& path)
{
    auto it = m_nodes.find(path);
    if (it != m_nodes.end())
    {
        if (std::shared_ptr<TomlNode> node = it->second.lock())
        {
            return node;
        }
        else
        {
            m_nodes.erase(it);
        }
    }

    auto result = Dig(m_root, path);
    if (result == nullptr)
    {
        return nullptr;
    }

    auto node = std::make_shared<TomlNode>(path, std::move(result));
    m_nodes[path] = node;
    return node;
}

} // namespace birch

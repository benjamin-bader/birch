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

#ifndef BIRCH_CONFIG_TOMLCONFIG_H
#define BIRCH_CONFIG_TOMLCONFIG_H

#include <filesystem>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

#include "ConfigPath.h"
#include "IConfig.h"
#include "IConfigSource.h"

#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include "toml.hpp"

namespace birch::config {

class TomlValue;

class TomlList : public IValueList
{
    std::vector<std::shared_ptr<TomlValue>> m_values;

public:
    static std::shared_ptr<TomlList> Create(const toml::array& array);

    TomlList() = default;
    TomlList(const TomlList& other) = default;
    TomlList(TomlList&& other) = default;
    TomlList(std::vector<std::shared_ptr<TomlValue>>&& values);

    TomlList& operator=(const TomlList& other) = default;
    TomlList& operator=(TomlList&& other) = default;

    std::size_t Size() const override;
    std::shared_ptr<IValue> Get(std::size_t index) const override;
};

class TomlMap : public IValueMap
{
    absl::flat_hash_map<std::string, std::shared_ptr<TomlValue>> m_values;

public:
    static std::shared_ptr<TomlMap> Create(const toml::table& table);

    TomlMap() = default;
    TomlMap(const TomlMap& other) = default;
    TomlMap(TomlMap&& other) = default;
    TomlMap(absl::flat_hash_map<std::string, std::shared_ptr<TomlValue>>&& values);

    TomlMap& operator=(const TomlMap& other) = default;
    TomlMap& operator=(TomlMap&& other) = default;

    std::size_t Size() const override;
    std::shared_ptr<IValue> Get(const std::string& key) const override;
};

class TomlValue : public IValue {
public:
    using Variant = std::variant<
        bool,
        std::int64_t,
        double,
        std::string,
        std::shared_ptr<TomlList>,
        std::shared_ptr<TomlMap>
    >;

    static std::shared_ptr<TomlValue> Create(const toml::node& node);

    TomlValue();
    TomlValue(const TomlValue& other) = default;
    TomlValue(TomlValue&& other) = default;
    TomlValue& operator=(const TomlValue& other) = default;
    TomlValue& operator=(TomlValue&& other) = default;
    TomlValue(const Variant& value);
    TomlValue(Variant&& value);

    virtual ~TomlValue() = default;

    std::optional<bool> AsBool() const override;
    std::optional<std::int64_t> AsInt64() const override;
    std::optional<double> AsDouble() const override;
    std::optional<std::string> AsString() const override;
    std::shared_ptr<IValueList> AsArray() const override;
    std::shared_ptr<IValueMap> AsMap() const override;

    void Reset(const toml::node& node);

private:
    Variant m_value;
};


class TomlNode : public IConfigNode
{
    ConfigPath m_path;
    std::shared_ptr<TomlValue> m_value;
    std::vector<std::function<void(const ConfigPath&, const IValue*)>> m_callbacks;

public:
    TomlNode(const ConfigPath& path, std::shared_ptr<TomlValue>&& value);
    ~TomlNode() override = default;

    const ConfigPath& GetPath() const override;
    TomlValue* GetValue() override;
    void Subscribe(std::function<void(const ConfigPath&, const IValue*)>&& callback) override;
    void Notify();
};

/**
 * A TomlConfig is an IConfig implementation that is backed by TOML data.
 */
class TomlConfig : public IConfig, public std::enable_shared_from_this<TomlConfig>
{
    std::unique_ptr<IConfigDataSource> m_source;
    std::shared_ptr<TomlValue> m_root;
    absl::btree_map<ConfigPath, std::weak_ptr<TomlNode>> m_nodes;

public:
    static std::shared_ptr<TomlConfig> Create(std::unique_ptr<IConfigDataSource>&& source);

    TomlConfig(std::unique_ptr<IConfigDataSource>&& source);

    void Reload() override;

    std::shared_ptr<IConfigNode> GetRootNode();

    std::shared_ptr<IConfigNode> GetNode(const ConfigPath& path) override;
};

} // namespace birch

#endif // BIRCH_CONFIG_TOMLCONFIG_H

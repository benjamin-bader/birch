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

#include "Config.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "absl/strings/str_cat.h"

#include "ValueSources.h"

namespace birch {

class Config : public IConfig, public std::enable_shared_from_this<Config>
{
    std::unique_ptr<IConfigSource> m_source;
    std::map<std::string, std::string> m_values;

public:
    Config(std::unique_ptr<IConfigSource>&& source);

    void Reload() override;

    std::shared_ptr<IValueSource<std::int64_t>> GetInt64(const std::string& key) override;
    std::shared_ptr<IValueSource<std::string>> GetString(const std::string& key) override;
    std::shared_ptr<IValueSource<bool>> GetBool(const std::string& key) override;
    std::shared_ptr<IValueSource<double>> GetDouble(const std::string& key) override;

    std::shared_ptr<IConfig> GetSection(const std::string& key) override;
};

class ConfigView : public IConfig
{
    std::shared_ptr<Config> m_config;
    std::string m_key;

public:
    ConfigView(const std::shared_ptr<Config>& config, const std::string& key);

    void Reload() override;

    std::shared_ptr<IValueSource<std::int64_t>> GetInt64(const std::string& key) override;
    std::shared_ptr<IValueSource<std::string>> GetString(const std::string& key) override;
    std::shared_ptr<IValueSource<bool>> GetBool(const std::string& key) override;
    std::shared_ptr<IValueSource<double>> GetDouble(const std::string& key) override;

    std::shared_ptr<IConfig> GetSection(const std::string& key) override;
};

// Config impl

Config::Config(std::unique_ptr<IConfigSource>&& source)
    : m_source(std::move(source))
    , m_values()
{
}

void Config::Reload()
{
    auto values = m_source->LoadConfigValues();

    if (values.ok())
    {
        for (const auto& [key, value] : values.value())
        {
            m_values[key] = value;
        }
    }
    else
    {
        LOG(ERROR) << "Failed to load configuration values: " << values.status();
    }
}

std::shared_ptr<IValueSource<std::int64_t>> Config::GetInt64(const std::string& key)
{
    auto value = m_values.find(key);
    if (value == m_values.end())
    {
        return std::make_shared<Int64ValueSource>(key, std::nullopt);
    }

    return std::make_shared<Int64ValueSource>(key, value->second);
}

std::shared_ptr<IValueSource<std::string>> Config::GetString(const std::string& key)
{
    auto value = m_values.find(key);
    if (value == m_values.end())
    {
        return std::make_shared<StringValueSource>(key, std::nullopt);
    }

    return std::make_shared<StringValueSource>(key, value->second);
}

std::shared_ptr<IValueSource<bool>> Config::GetBool(const std::string& key)
{
    auto value = m_values.find(key);
    if (value == m_values.end())
    {
        return std::make_shared<BoolValueSource>(key, std::nullopt);
    }

    return std::make_shared<BoolValueSource>(key, value->second);
}

std::shared_ptr<IValueSource<double>> Config::GetDouble(const std::string& key)
{
    auto value = m_values.find(key);
    if (value == m_values.end())
    {
        return std::make_shared<DoubleValueSource>(key, std::nullopt);
    }

    return std::make_shared<DoubleValueSource>(key, value->second);
}

std::shared_ptr<IConfig> Config::GetSection(const std::string& key)
{
    return std::make_shared<ConfigView>(shared_from_this(), key);
}

// ConfigView impl

ConfigView::ConfigView(const std::shared_ptr<Config>& config, const std::string& key)
    : m_config(config)
    , m_key(key)
{
}

void ConfigView::Reload()
{
    m_config->Reload();
}

std::shared_ptr<IValueSource<std::int64_t>> ConfigView::GetInt64(const std::string& key)
{
    return m_config->GetInt64(absl::StrCat(m_key, ".", key));
}

std::shared_ptr<IValueSource<std::string>> ConfigView::GetString(const std::string& key)
{
    return m_config->GetString(absl::StrCat(m_key, ".", key));
}

std::shared_ptr<IValueSource<bool>> ConfigView::GetBool(const std::string& key)
{
    return m_config->GetBool(absl::StrCat(m_key, ".", key));
}

std::shared_ptr<IValueSource<double>> ConfigView::GetDouble(const std::string& key)
{
    return m_config->GetDouble(absl::StrCat(m_key, ".", key));
}

std::shared_ptr<IConfig> ConfigView::GetSection(const std::string& key)
{
    return m_config->GetSection(absl::StrCat(m_key, ".", key));
}

// IConfig impl

std::shared_ptr<IConfig> IConfig::CreateFromSource(std::unique_ptr<IConfigSource>&& source)
{
    auto config = std::make_shared<Config>(std::move(source));
    config->Reload();
    return config;
}

} // namespace birch

#include "TomlConfigSource.h"

#include <memory>
#include <string>

#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "toml.hpp"

namespace birch {

namespace {

std::string FormatTomlScalar(const toml::node& node)
{
    switch (node.type())
    {
    case toml::node_type::string:
        return std::string(static_cast<const std::string&>(*node.as<std::string>()));
    case toml::node_type::integer:
        return std::to_string(
            static_cast<std::int64_t>(static_cast<const std::int64_t&>(*node.as<std::int64_t>())));
    case toml::node_type::floating_point:
        return std::to_string(static_cast<double>(static_cast<const double&>(*node.as<double>())));
    case toml::node_type::boolean:
        return static_cast<bool>(static_cast<const bool&>(*node.as<bool>())) ? "true" : "false";
    case toml::node_type::date:
        return absl::StrFormat("%v", absl::FormatStreamed(*node.as_date()));
    case toml::node_type::time:
        return absl::StrFormat("%v", absl::FormatStreamed(*node.as_time()));
    case toml::node_type::date_time:
        return absl::StrFormat("%v", absl::FormatStreamed(*node.as_date_time()));
    case toml::node_type::none:
    case toml::node_type::table:
    case toml::node_type::array:
    default:
        LOG(INFO) << "Unhandled TOML node type: " << absl::StrFormat("%v", absl::FormatStreamed(node.type()));
        return {};
    }
}

void AccumulateKeyValuePairs(IConfigSource::KeyValuePairs& pairs, const std::string& prefix, const toml::node& node)
{
    if (node.is_table())
    {
        node.as_table()->for_each([&](const toml::key& key, const toml::node& value)
        {
            const std::string configKey =
                prefix.empty() ? std::string(key.str()) : absl::StrCat(prefix, ".", key.str());
            AccumulateKeyValuePairs(pairs, configKey, value);
        });
    }
    else if (node.is_array())
    {
        size_t index = 0;
        for (const auto& element : *node.as_array())
        {
            const std::string configKey =
                prefix.empty() ? std::to_string(index) : absl::StrCat(prefix, ".", index);
            AccumulateKeyValuePairs(pairs, configKey, element);
            ++index;
        }
    }
    else
    {
        pairs.emplace_back(prefix, FormatTomlScalar(node));
    }
}

} // namespace

absl::StatusOr<std::unique_ptr<IConfigSource>> TomlConfigSource::CreateFromTomlFile(const std::filesystem::path& path)
{
    return std::make_unique<TomlConfigSource>(path);
}

TomlConfigSource::TomlConfigSource(const std::filesystem::path& path)
    : m_path(path)
{
}

absl::StatusOr<IConfigSource::KeyValuePairs> TomlConfigSource::LoadConfigValues() const
{
    toml::table table;
    try
    {
        table = toml::parse_file(m_path.string());
    }
    catch (const toml::parse_error& e)
    {
        return absl::InvalidArgumentError(e.what());
    }

    IConfigSource::KeyValuePairs pairs;
    
    AccumulateKeyValuePairs(pairs, "", table);

    return pairs;
}

} // namespace birch
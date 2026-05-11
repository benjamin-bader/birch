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

#include "TomlConfigSource.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "absl/status/status.h"
#include "gtest/gtest.h"

namespace birch {

namespace {

std::string WriteTempTomlFile(std::string_view contents)
{
    static int seq = 0;
    const char* tmpdir = std::getenv("TEST_TMPDIR");
    const std::filesystem::path dir =
        tmpdir ? std::filesystem::path(tmpdir) : std::filesystem::temp_directory_path();
    const auto path =
        dir / std::filesystem::path("toml_config_source_test_" + std::to_string(++seq) + ".toml");
    std::ofstream out(path);
    out << contents;
    return path.string();
}

void SortPairs(IConfigSource::KeyValuePairs& pairs)
{
    std::sort(pairs.begin(), pairs.end());
}

const std::string* FindValue(const IConfigSource::KeyValuePairs& pairs, std::string_view key)
{
    for (const auto& p : pairs)
    {
        if (p.first == key)
        {
            return &p.second;
        }
    }
    return nullptr;
}

} // namespace

TEST(TomlConfigSourceTest, CreateFromTomlFileReturnsUsableSource)
{
    const std::string path = WriteTempTomlFile(R"(
title = "birch"
enabled = true
)");
    auto created = TomlConfigSource::CreateFromTomlFile(path);
    ASSERT_TRUE(created.ok());
    ASSERT_NE(created.value(), nullptr);

    auto loaded = created.value()->LoadConfigValues();
    ASSERT_TRUE(loaded.ok());

    IConfigSource::KeyValuePairs expected = {
        { "enabled", "true" },
        { "title", "birch" },
    };
    SortPairs(loaded.value());
    SortPairs(expected);
    EXPECT_EQ(loaded.value(), expected);
}

TEST(TomlConfigSourceTest, NestedTablesBecomeDottedKeys)
{
    const std::string path = WriteTempTomlFile(R"(
[server]
port = 6697
ssl = false

[server.tls]
min_version = "1.2"
)");
    TomlConfigSource source(path);
    auto loaded = source.LoadConfigValues();
    ASSERT_TRUE(loaded.ok());

    IConfigSource::KeyValuePairs expected = {
        { "server.port", "6697" },
        { "server.ssl", "false" },
        { "server.tls.min_version", "1.2" },
    };
    SortPairs(loaded.value());
    SortPairs(expected);
    EXPECT_EQ(loaded.value(), expected);
}

TEST(TomlConfigSourceTest, LocalDateSerializes)
{
    const std::string path = WriteTempTomlFile("birthday = 1979-05-27\n");
    TomlConfigSource source(path);
    auto loaded = source.LoadConfigValues();
    ASSERT_TRUE(loaded.ok());

    const std::string* value = FindValue(loaded.value(), "birthday");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "1979-05-27");
}

TEST(TomlConfigSourceTest, UtcDateTimeSerializes)
{
    const std::string path = WriteTempTomlFile("stamp = 1979-05-27T07:32:00Z\n");
    TomlConfigSource source(path);
    auto loaded = source.LoadConfigValues();
    ASSERT_TRUE(loaded.ok());

    const std::string* value = FindValue(loaded.value(), "stamp");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "1979-05-27T07:32:00Z");
}

TEST(TomlConfigSourceTest, LocalDateTimeWithoutOffsetSerializes)
{
    const std::string path = WriteTempTomlFile("starts = 1979-05-27T07:32:00\n");
    TomlConfigSource source(path);
    auto loaded = source.LoadConfigValues();
    ASSERT_TRUE(loaded.ok());

    const std::string* value = FindValue(loaded.value(), "starts");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, "1979-05-27T07:32:00");
}

TEST(TomlConfigSourceTest, InvalidTomlReturnsInvalidArgument)
{
    // Bare keys cannot contain spaces; this must fail parsing without tripping
    // toml++ internal assertions (e.g. malformed table headers like "[[[").
    const std::string path = WriteTempTomlFile("hello world = 1\n");
    TomlConfigSource source(path);
    auto loaded = source.LoadConfigValues();
    ASSERT_FALSE(loaded.ok());
    EXPECT_EQ(loaded.status().code(), absl::StatusCode::kInvalidArgument);
}

TEST(TomlConfigSourceTest, MissingFileReturnsError)
{
    const std::string path = WriteTempTomlFile("");
    std::filesystem::remove(path);

    TomlConfigSource source(path);
    auto loaded = source.LoadConfigValues();
    ASSERT_FALSE(loaded.ok());
}

} // namespace birch

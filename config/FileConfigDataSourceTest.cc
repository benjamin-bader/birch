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

#include "FileConfigDataSource.h"

#include <filesystem>
#include <fstream>

#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "gtest/gtest.h"

#include "util/IFileWatcher.h"

namespace birch::config {

class FileConfigDataSourceTest : public testing::Test
{
protected:
    std::filesystem::path m_path;

    void SetUp() override
    {
        auto status = birch::util::InitializeGlobalFileWatcherForTesting();
        ABSL_ASSERT_OK(status);
        m_path = std::filesystem::path(testing::TempDir()) / "config.toml";

        std::ofstream os(m_path);
        os << "server_name = \"irc.example.com\"" << std::endl;
    }

    void TearDown() override
    {
        std::filesystem::remove(m_path);
    }
};

TEST_F(FileConfigDataSourceTest, CreateFromFileAcceptsAbsolutePath)
{
    auto source = FileConfigDataSource::CreateFromFile(std::filesystem::path(m_path));
    ABSL_EXPECT_OK(source);
    EXPECT_NE(*source, nullptr);
}

TEST_F(FileConfigDataSourceTest, ReadReturnsFileContents)
{   
    FileConfigDataSource source(m_path);
    auto contents = source.Read();

    ABSL_EXPECT_OK(contents);
    EXPECT_EQ(*contents, "server_name = \"irc.example.com\"\n");
}

TEST_F(FileConfigDataSourceTest, ReadReturnsNotFoundForMissingFile)
{
    FileConfigDataSource source("some/missing/config.toml");

    auto contents = source.Read();

    EXPECT_FALSE(contents.ok());
    EXPECT_EQ(contents.status().code(), absl::StatusCode::kNotFound);
}

TEST_F(FileConfigDataSourceTest, SubscribeAcceptsCallback)
{
    FileConfigDataSource source(m_path);

    EXPECT_NO_THROW(source.Subscribe([]() {}));
}

TEST_F(FileConfigDataSourceTest, SubscribeAcceptsMultipleCallbacks)
{
    FileConfigDataSource source(m_path);

    EXPECT_NO_THROW({
        source.Subscribe([]() {});
        source.Subscribe([]() {});
        source.Subscribe([]() {});
    });
}

} // namespace birch::config

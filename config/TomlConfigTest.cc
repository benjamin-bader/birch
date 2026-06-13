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

#include <filesystem>

#include "absl/status/status_matchers.h"
#include "gtest/gtest.h"

#include "FileConfigDataSource.h"

namespace birch::config {

class TomlConfigTest : public testing::Test
{
protected:
    std::filesystem::path m_path;

    void SetUp() override
    {
        auto status = birch::util::InitializeGlobalFileWatcherForTesting();
        ABSL_ASSERT_OK(status);
        m_path = std::filesystem::path(testing::TempDir()) / "test.toml";
    }

    void TearDown() override
    {
        std::filesystem::remove(m_path);
    }
};

TEST_F(TomlConfigTest, CanCreateFromDataSource)
{
    {
        std::ofstream os(m_path);
        os << "server_name = \"irc.example.com\"";
    }

    auto source = FileConfigDataSource::CreateFromFile(m_path);
    ABSL_ASSERT_OK(source);
    TomlConfig config(std::move(*source));
}

} // namespace birch

// birch - IRC daemon, built with Bazel
//
// Copyright (C) 2018 Benjamin Bader
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

#include <filesystem>
#include <string>
#include <vector>

#include "config/ConfigPath.h"
#include "config/FileConfigDataSource.h"
#include "config/TomlConfig.h"
#include "config/IConfig.h"
#include "server/Server.h"

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "absl/log/log.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_cat.h"

int main(int argc, char** argv)
{
  absl::FlagsUsageConfig usageConfig;
  usageConfig.version_string = []() -> std::string
  {
    return absl::StrCat("birch ", VERSION); // this will always look like a compiler error in the IDE; bazel provides this as a define.
  };
  
  absl::SetProgramUsageMessage("TODO: Make an IRC server");
  std::vector<char*> positionalArgs = absl::ParseCommandLine(argc, argv);

  absl::InitializeLog();

  auto configSource = birch::config::FileConfigDataSource::CreateFromFile("config.toml");
  if (!configSource.ok()) {
    LOG(ERROR) << "Failed to create config source: " << configSource.status();
    return 1;
  }

  auto config = birch::config::TomlConfig::Create(std::move(*configSource));
  if (config == nullptr) {
    LOG(ERROR) << "Failed to create config";
    return 1;
  }

  return 0;
}

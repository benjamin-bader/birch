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

#include <exception>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "config/ConfigPath.h"
#include "config/FileConfigDataSource.h"
#include "config/TomlConfig.h"
#include "config/IConfig.h"
#include "server/Server.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "absl/log/log.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_cat.h"

namespace std::filesystem {

bool AbslParseFlag(std::string_view text, std::filesystem::path* out, std::string* err)
{
    *out = std::filesystem::path{text};
    return true;
}

std::string AbslUnparseFlag(std::filesystem::path p)
{
    return p.string();
}

} // namespace std::filesystem

ABSL_FLAG(std::filesystem::path, configFile, "./config.toml", "The path to the config file");

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

  auto status = birch::util::InitializeGlobalFileWatcher();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to initialize global file watcher: " << status;
    return 1;
  }

  auto configFilePath = absl::GetFlag(FLAGS_configFile);
  auto configSource = birch::config::FileConfigDataSource::CreateFromFile(configFilePath);
  if (!configSource.ok()) {
    LOG(ERROR) << "Failed to create config source: " << configSource.status();
    return 1;
  }

  auto config = birch::config::TomlConfig::Create(std::move(*configSource));
  if (config == nullptr) {
    LOG(ERROR) << "Failed to create config";
    return 1;
  }

  auto serverConfig = birch::server::IServerConfig::Create(config);
  if (!serverConfig.ok())
  {
      LOG(ERROR) << "Failed to create server config: " << serverConfig.status();
      return 1;
  }

  auto server = birch::server::IServer::Create(*serverConfig);
  if (server == nullptr)
  {
      LOG(ERROR) << "Failed to create server instance";
      return 1;
  }

  try
  {
      LOG(INFO) << "Running server...";
      server->ServeForever();
  }
  catch (std::exception& e)
  {
      LOG(ERROR) << "Caught an unhandled exception: " << e.what();
  }

  return 0;
}

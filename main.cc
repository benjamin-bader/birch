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

#include "server/Server.h"

#include <iostream>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "tclap/CmdLine.h"

int main(int argc, char** argv)
{
  auto console = spdlog::stdout_color_mt("console");
  try
  {
    TCLAP::CmdLine cmd("TODO: Make an IRC server", ' ', "0.1.0");
    cmd.parse(argc, argv);

    console->info("Hello, you!");
  }
  catch (const TCLAP::ArgException& e)
  {
    std::cerr << "oh no\n" << e.error() << std::endl;
    return 1;
  }

  return 0;
}

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

#include "absl/debugging/failure_signal_handler.h"
#include "absl/debugging/symbolize.h"
#include "absl/log/log.h"
#include "absl/log/initialize.h"

#include "gtest/gtest.h"

int main(int argc, char** argv)
{
    absl::InitializeSymbolizer(argv[0]);

    absl::FailureSignalHandlerOptions options;
    absl::InstallFailureSignalHandler(options);

    absl::InitializeLog();

    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}

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

#ifndef BIRCH_SERVER_MESSAGESERIALIZER_H
#define BIRCH_SERVER_MESSAGESERIALIZER_H

#include <cstddef>
#include <span>

#include "server/Message.h"

namespace birch::server {

class MessageSerializer
{
public:
    std::size_t ComputeRequiredSpace(const Message& message);
    std::size_t WriteToBuffer(std::span<char> buffer, const Message& message);
};

} // namespace birch::server

#endif

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

#ifndef BIRCH_IRC_USER_H
#define BIRCH_IRC_USER_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "absl/time/time.h"

namespace birch::irc {

using UserId = std::uint64_t;

class User
{
    UserId m_id;
    std::string m_nick;
    std::string m_origin;
    int m_mode{0};

public:
    User();
    User(const User&) = default;
    User(User&&) = default;

    virtual ~User() = default;

    User& operator=(const User&) = default;
    User& operator=(User&&) = default;

    UserId GetId() const;
    const std::string& GetNick() const;
    const std::string& GetRealname() const;

    int GetMode() const;
    bool IsOp() const;
    bool IsInvisible() const;

};

} // namespace birch::irc

#endif

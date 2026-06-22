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

#ifndef BIRCH_IRC_IRC_STATE_H
#define BIRCH_IRC_IRC_STATE_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <asio.hpp>
#include <asio/strand.hpp>
#include <asio/thread_pool.hpp>

#include "absl/container/flat_hash_map.h"
#include "absl/status/statusor.h"

#include "irc/Channel.h"
#include "irc/User.h"


namespace birch::irc {

class IrcState
{
    using executor_t = asio::any_io_executor;

    absl::flat_hash_map<UserId, std::shared_ptr<User>> m_users;
    absl::flat_hash_map<std::string, std::shared_ptr<Channel>> m_channelsByName;
    absl::flat_hash_map<std::uint64_t, std::string> m_foo;

    asio::strand<executor_t> m_strand;

public:
    IrcState();

    absl::StatusOr<std::shared_ptr<Channel>> CreateChannel(const std::string& name);
    std::shared_ptr<Channel> FindChannel(const std::string& name) const;


};

} // namespace birch::irc

#endif

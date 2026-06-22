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

#ifndef BIRCH_IRC_CHANNEL_H
#define BIRCH_IRC_CHANNEL_H

#include <memory>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

#include "irc/User.h"

namespace birch::irc {

class Channel
{
    std::string m_name;
    absl::flat_hash_map<UserId, std::shared_ptr<User>> m_members;

public:
    static absl::StatusOr<std::shared_ptr<Channel>> Create(const std::string& name);

    Channel(const std::string& name);
    ~Channel() = default;

    const std::string& GetName() const;

    std::vector<std::shared_ptr<User>> ListMembers() const;

    absl::Status Join(const std::shared_ptr<User>& user);
    absl::Status Depart(const std::shared_ptr<User>& user);

    void AdministrativelyAddUser(const std::shared_ptr<User>& user);
    void AdministrativelyRemoveUser(const std::shared_ptr<User>& user);

};

}

#endif

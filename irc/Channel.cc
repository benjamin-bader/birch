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

#include "Channel.h"

#include <memory>
#include <string>

#include "absl/log/check.h"
#include "absl/status/status.h"

namespace birch::irc {

absl::StatusOr<std::shared_ptr<Channel>> Channel::Create(const std::string& name)
{
    // TODO: Validate the channel name
    return std::make_shared<Channel>(name);
}

Channel::Channel(const std::string& name)
    : m_name(name)
    , m_members()
{}

const std::string& Channel::GetName() const
{
    return m_name;
}

std::vector<std::shared_ptr<User>> Channel::ListMembers() const
{
    std::vector<std::shared_ptr<User>> result{m_members.size()};

    for (const auto& [_, user] : m_members)
    {
        result.push_back(user);
    }

    return result;
}

absl::Status Channel::Join(const std::shared_ptr<User>& user)
{
    // TODO: Enforce some rules here
    CHECK(user != nullptr);
    return absl::UnimplementedError("not yet implemented");
}

absl::Status Channel::Depart(const std::shared_ptr<User>& user)
{
    CHECK(user != nullptr);
    return absl::UnimplementedError("not yet implemented");
}

void Channel::AdministrativelyAddUser(const std::shared_ptr<User>& user)
{
    CHECK(user != nullptr);
    m_members.emplace(user->GetId(), user);
}

void Channel::AdministrativelyRemoveUser(const std::shared_ptr<User>& user)
{
    CHECK(user != nullptr);
    m_members.erase(user->GetId());
}

} // namespace birch::irc

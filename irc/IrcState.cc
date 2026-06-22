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

#include "IrcState.h"

#include "absl/strings/str_cat.h"

namespace birch::irc {

IrcState::IrcState()
{

}

absl::StatusOr<std::shared_ptr<Channel>> IrcState::CreateChannel(const std::string& name)
{
    auto it = m_channelsByName.find(name);
    if (it != m_channelsByName.end())
    {
        return absl::AlreadyExistsError(absl::StrCat("Channel ", name, " already exists"));
    }

    auto statusOrChannel = Channel::Create(name);
    if (!statusOrChannel.ok())
    {
        return statusOrChannel.status();
    }

    m_channelsByName.emplace(name, *statusOrChannel);

    return *statusOrChannel;
}

} // namespace birch::irc

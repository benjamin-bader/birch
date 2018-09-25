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

#ifndef BIRCH_CHANNEL_H
#define BIRCH_CHANNEL_H

#include <string>

namespace birch {

bool IsValidChannelName(const std::string& name);

enum class ChannelKind
{
    Normal,
    ServerLocal,
};

enum class ChannelModes
{
    ChannelCreatorStatus,
    GrantOperatorStatus,
    GrantVoiceStatus,

    Anonymous,
    InviteOnly,
    Moderated,
    NoMessagesFromOutside,
    QuietMode,
    SecretMode,
    ServerReOp,

    ChannelKey,
    UserLimit,

};

class IChannel
{
public:
    virtual ~IChannel() = default;

    virtual const std::string& GetTopic() const noexcept = 0;
    virtual void SetTopic(const std::string& topic) = 0;

    /**
     * Checks if the user identified by the given label is "privileged"
     * within this channel.
     */
    virtual bool IsUserPrivileged(const std::string& userLabel) const = 0;

    /**
     * Returns @c true if the channel is "safe".
     * 
     * Safe channels continue to exist even after the last
     * user departs.  In contrast, non-safe channels expire
     * when they have no members.
     */
    virtual bool IsSafeChannel() const noexcept = 0;

    virtual bool HasMode(ChannelModes mode) const noexcept = 0;
};

}

#endif
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

#include "Message.h"

#include <sstream>
#include <string>

#include "gtest/gtest.h"

#include "absl/strings/str_format.h"

namespace birch::irc {
namespace {

std::string ToString(const Message& message)
{
    std::ostringstream os;
    os << message;
    return os.str();
}

TEST(MessageToString, EmptyMessage)
{
    Message message = Message::Builder().Build();
    EXPECT_EQ("Message(prefix=, command=, params=[], tags={})", ToString(message));
}

TEST(MessageToString, PrefixOnly)
{
    Message message = Message::Builder()
        .WithPrefix("irc.example.com")
        .Build();
    EXPECT_EQ("Message(prefix=irc.example.com, command=, params=[], tags={})", ToString(message));
}

TEST(MessageToString, CommandOnly)
{
    Message message = Message::Builder()
        .WithCommand("PING")
        .Build();
    EXPECT_EQ("Message(prefix=, command=PING, params=[], tags={})", ToString(message));
}

TEST(MessageToString, SingleParam)
{
    Message message = Message::Builder()
        .WithCommand("JOIN")
        .WithParam("#birch")
        .Build();
    EXPECT_EQ("Message(prefix=, command=JOIN, params=[#birch], tags={})", ToString(message));
}

TEST(MessageToString, MultipleParams)
{
    Message message = Message::Builder()
        .WithCommand("PRIVMSG")
        .WithParam("#birch")
        .WithParam("hello world")
        .Build();
    EXPECT_EQ("Message(prefix=, command=PRIVMSG, params=[#birch, hello world], tags={})", ToString(message));
}

TEST(MessageToString, FullMessage)
{
    Message message = Message::Builder()
        .WithPrefix("nick!user@host")
        .WithCommand("PRIVMSG")
        .WithParam("#birch")
        .WithParam("hi there")
        .Build();
    EXPECT_EQ(
        "Message(prefix=nick!user@host, command=PRIVMSG, params=[#birch, hi there], tags={})",
        ToString(message));
}

TEST(MessageToString, SingleTag)
{
    Message message = Message::Builder()
        .WithCommand("PING")
        .WithTag("time", "2018-01-01T00:00:00.000Z")
        .Build();
    EXPECT_EQ(
        "Message(prefix=, command=PING, params=[], tags={time=2018-01-01T00:00:00.000Z})",
        ToString(message));
}

TEST(MessageToString, MultipleTagsPreserveInsertionOrder)
{
    Message message = Message::Builder()
        .WithCommand("PRIVMSG")
        .WithTag("account", "alice")
        .WithTag("time", "2018-01-01T00:00:00.000Z")
        .Build();
    EXPECT_EQ(
        "Message(prefix=, command=PRIVMSG, params=[], tags={account=alice, time=2018-01-01T00:00:00.000Z})",
        ToString(message));
}

} // namespace
} // namespace birch::server

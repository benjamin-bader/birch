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

#include "MessageParser.h"

#include <functional>
#include <iostream>
#include <string>
#include <tuple>

#include "gtest/gtest.h"

namespace birch::server {

class ParserTest : public testing::Test
{
protected:
    Message message;
    MessageParser parser;
    ParseState state;

    ParseState Parse(const std::string& text)
    {
        auto begin = std::begin(text);
        auto end = std::end(text);
        message = {};
        parser.Reset();
        state = parser.Parse(message, begin, end);
        return state;
    }
};

TEST_F(ParserTest, AllowsSimpleCommandWithNoParams)
{
    EXPECT_EQ(ParseState::Complete, Parse("LIST\r\n"));
    EXPECT_EQ("LIST", message.GetCommand());
}

TEST_F(ParserTest, MessagesAreTerminatedWithCrLf)
{
    EXPECT_EQ(ParseState::Incomplete, Parse("LIST"));
    EXPECT_EQ(ParseState::Incomplete, Parse("LIST\r"));
    EXPECT_EQ(ParseState::Complete, Parse("LIST\r\n"));

    EXPECT_EQ(ParseState::Invalid, Parse("LIST\n"));
}

TEST_F(ParserTest, MessagesCanHavePrefixes)
{
    EXPECT_EQ(ParseState::Complete, Parse(":notben!ben@bendb.com LIST\r\n"));
    EXPECT_EQ("notben!ben@bendb.com", message.GetPrefix());
}

TEST_F(ParserTest, PrefixCanBeHostname)
{
    EXPECT_EQ(ParseState::Complete, Parse(":birch.bendb.com LIST\r\n"));
    EXPECT_EQ("birch.bendb.com", message.GetPrefix());
}

TEST_F(ParserTest, PrefixCanBeBareNickname)
{
    EXPECT_EQ(ParseState::Complete, Parse(":ben LIST\r\n"));
    EXPECT_EQ(ParseState::Complete, Parse(":[]{}|`_\\^@foo LIST\r\n")); // special chars are legal
}

TEST_F(ParserTest, ParseNickAtHostname)
{
    EXPECT_EQ(ParseState::Complete, Parse(":[]{}|`_\\^@bendb.com LIST\r\n")); // special characters are legal
    EXPECT_EQ(ParseState::Invalid, Parse(":ben@-bendb.com LIST\r\n"));
    EXPECT_EQ(ParseState::Invalid, Parse(":ben@bendb..com LIST\r\n"));
    EXPECT_EQ(ParseState::Invalid, Parse(":ben@bendb.com. LIST\r\n"));

    EXPECT_EQ(ParseState::Invalid, Parse(":abcdefghij@bendb.com LIST\r\n")); // max length of 9
    EXPECT_EQ(ParseState::Invalid, Parse(":0ben@bendb.com LIST\r\n")); // can't start with number
    EXPECT_EQ(ParseState::Invalid, Parse(":-ben@bendb.com LIST\r\n")); // can't start with hyphen

    EXPECT_EQ(ParseState::Complete, Parse(":ben@127.0.0.1 LIST\r\n"));
    EXPECT_EQ(ParseState::Complete, Parse(":ben@::1 LIST\r\n"));
}

TEST_F(ParserTest, ParseNickAtIPAddress)
{
    EXPECT_EQ(ParseState::Complete, Parse(":ben@127.0.0.1 LIST\r\n"));
    EXPECT_EQ(ParseState::Complete, Parse(":ben@::1 LIST\r\n"));
    EXPECT_EQ(ParseState::Complete, Parse(":ben@2001:0db8:85a3:0000:0000:8a2e:0370:7334 LIST\r\n"));
}

TEST_F(ParserTest, ParseNickAndUserAtHostname)
{
    EXPECT_EQ(ParseState::Complete, Parse(":ben!notben@bendb.com LIST\r\n"));

    // The following user segments each have one of the five forbidden
    // characters.
    EXPECT_EQ(ParseState::Invalid, Parse(":ben!not\rben@bendb.com LIST\r\n"));
    EXPECT_EQ(ParseState::Invalid, Parse(":ben!not\nben@bendb.com LIST\r\n"));
    //EXPECT_EQ(ParseState::Invalid, Parse(":ben!not\0ben@bendb.com LIST\r\n"));
    EXPECT_EQ(ParseState::Invalid, Parse(":ben!not ben@bendb.com LIST\r\n"));
    EXPECT_EQ(ParseState::Invalid, Parse(":ben!not@ben@bendb.com LIST\r\n"));

    // A nick is still required when a userame is given
    EXPECT_EQ(ParseState::Invalid, Parse(":!notben@bendb.com LIST\r\n"));

    // A host is mandatory if a username is given
    EXPECT_EQ(ParseState::Invalid, Parse(":ben!notben LIST\r\n"));
}

TEST_F(ParserTest, PrefixCanBeIPv4)
{
    EXPECT_EQ(ParseState::Complete, Parse(":127.0.0.1 LIST\r\n"));
    EXPECT_EQ("127.0.0.1", message.GetPrefix());
}

TEST_F(ParserTest, CommandWithSpaceAfterName)
{
    EXPECT_EQ(ParseState::Complete, Parse("LIST \r\n"));
    EXPECT_EQ("LIST", message.GetCommand());
    EXPECT_EQ(0, message.GetParams().size());
}

TEST_F(ParserTest, CommandWithSpaceAfterNameAndParam)
{
    EXPECT_EQ(ParseState::Complete, Parse("LIST foo \r\n"));
    EXPECT_EQ("LIST", message.GetCommand());
    EXPECT_EQ(1, message.GetParams().size());
    EXPECT_EQ("foo", message.GetParams()[0]);
}

TEST_F(ParserTest, CommandWithSeveralSpacesAfterName)
{
    EXPECT_EQ(ParseState::Complete, Parse("LIST foo bar    \r\n"));
    EXPECT_EQ("LIST", message.GetCommand());
    EXPECT_EQ(2, message.GetParams().size());
    EXPECT_EQ("foo", message.GetParams()[0]);
    EXPECT_EQ("bar", message.GetParams()[1]);
}

TEST_F(ParserTest, CommandWithTags)
{
    EXPECT_EQ(ParseState::Complete, Parse("@foo=bar;baz=qux LIST\r\n"));
    EXPECT_EQ("LIST", message.GetCommand());
    EXPECT_EQ(0, message.GetParams().size());
    EXPECT_EQ(2, message.GetTags().size());

    auto tags = message.GetTags();
    EXPECT_EQ("bar", tags["foo"]);
    EXPECT_EQ("qux", tags["baz"]);
}

TEST_F(ParserTest, CommandWithEmptyTagValue)
{
    EXPECT_EQ(ParseState::Complete, Parse("@foo=;baz=qux LIST\r\n"));
    EXPECT_EQ("LIST", message.GetCommand());
    EXPECT_EQ(0, message.GetParams().size());
    EXPECT_EQ(2, message.GetTags().size());
    auto tags = message.GetTags();
    EXPECT_EQ("", tags["foo"]);
    EXPECT_EQ("qux", tags["baz"]);
}

TEST_F(ParserTest, CommandWithClientOnlyTag)
{
    EXPECT_EQ(ParseState::Complete, Parse("@+foo=bar LIST\r\n"));
    EXPECT_EQ("LIST", message.GetCommand());
    EXPECT_EQ(0, message.GetParams().size());
    EXPECT_EQ(1, message.GetTags().size());
    auto tags = message.GetTags();
    EXPECT_EQ("bar", tags["+foo"]);
}

} // namespace birch::server

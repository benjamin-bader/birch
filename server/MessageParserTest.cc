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

namespace birch {

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

}
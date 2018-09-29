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

#ifndef BIRCH_MESSAGEPARSER_H
#define BIRCH_MESSAGEPARSER_H

#include <array>
#include <string>

#include "Message.h"

namespace birch {

enum class ParseState
{
    Incomplete,
    Complete,
    Invalid
};

class MessageParser
{
public:
    template <typename InputIter>
    ParseState Parse(Message& message, InputIter& begin, InputIter end);

    void Reset();

private:

    ParseState Consume(Message& message, char input);

    // Checks m_buffer to see if it contains a valid message prefix.
    bool IsValidPrefix() const noexcept;

private:
    enum {
        parseStateStart = 0,

        parseStatePrefix, // We don't attempt to validate the prefix during incremental parsing

        parseStateCommandNameOrNumber,
        parseStateCommandName,
        parseStateCommandNumber,

        parseStateParamStart,
        parseStateParam,

        parseStateTrailing,

        parseStateCr,
        parseStateComplete
    } m_state {parseStateStart};

    std::string m_buffer {};
};

template <typename InputIter>
ParseState MessageParser::Parse(Message& message, InputIter& iter, InputIter end)
{
    ParseState state = ParseState::Incomplete;
    while (iter != end)
    {
        state = Consume(message, *iter++);
        if (state != ParseState::Incomplete)
        {
            break;
        }
    }
    return state;
}

}

#endif
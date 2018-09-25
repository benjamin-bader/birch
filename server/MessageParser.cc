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

#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace birch {

namespace {

// TODO:
// It's possible that we could collapse most of the following conditionals
// into a lookup table of 256 bytes, prepopulated with a flags enum
// like so:
//   uint8_t ccLetter = 1 << 0;
//   uint8_t ccDigit = 1 << 1;
//   uint8_t ccHex = 1 << 2;
//   uint8_t ccUsername = 1 << 3;
//   uint8_t ccKey = 1 << 4;
//   //etc
//
// constexpr std::array<byte, 256> InitLookupTable()
// {
//    std::array<byte, 256> table = ...;
//    for (int i = 'a'; i <= 'z'; ++i) table[i] |= ccLetter;
//    // etc
//    return table;
// }
//
// Checks like IsUserChar reduce to 'table[c] & ccUsername'.
//
// In the interest of debuggability, and of avoiding premature optimization,
// let's defer this until we think it's needed.  Such a table would take more
// cache lines than the current code, which may outweigh any branch-predictor
// gains.

inline constexpr bool IsUserChar(char c)
{
    // RFC 2812 2.3:
    // User names can be any octet except:
    //   NUL, CR, LF, " ", and "@"
    return c != '\0' && c != '\r' && c != '\n' && c != ' ' && c != '@';
}

inline constexpr bool IsKeyChar(char c)
{
    return (c >= 0x21 && c <= 0x7F)
        || (c >= 0x0E && c <= 0x1F)
        || (c >= 0x01 && c <= 0x05)
        || c == 0x00
        || c == 0x08
        || c == 0x0C;
}

inline constexpr bool IsLetter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline constexpr bool IsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

inline constexpr bool IsHex(char c)
{
    return IsDigit(c) || (c >= 'A' && c <= 'F');
}

inline constexpr bool IsSpecial(char c)
{
    // RFC 2812 2.3:
    // "special" chars are: '[', ']', '\', '`', '_', '^', '{', '|', '}'
    // equivalent to the ASCII ranges x5B-x60, x7B-x7D inclusive.
    return (c >= 0x5B && c <= 0x60) || (c >= 0x7B && c <= 0x7D);
}

inline constexpr bool IsWild(char c)
{
    return c == '*' || c == '?';
}

inline constexpr bool IsPrefixChar(char c)
{
    return IsLetter(c)
        || IsDigit(c)
        || c == '!'
        || c == '.'
        || c == '@';
}

inline constexpr bool IsParamStartChar(char c)
{
    return (c >= 0x3B && c <= 0xFF)
        || (c >= 0x21 && c <= 0x39)
        || (c >= 0x0E && c <= 0x1F)
        || (c >= 0x0B && c <= 0x0C)
        || (c >= 0x01 && c <= 0x09);
}

} // anonymous namespace

void MessageParser::Reset()
{
    m_state = parseStateStart;
    m_buffer.clear();
    m_numParams = 0;
}

ParseState MessageParser::Consume(Message& message, char input)
{
    #define NEXT(s) do { \
      std::cout << "-> " << #s << std::endl; \
      m_state = (s); \
    } while (0);

    switch (m_state)
    {
        case parseStateStart:
            if (input == ':')
            {
                NEXT(parseStatePrefix);
                // don't include the colon in the prefix buffer
                return ParseState::Incomplete;
            }

            if (IsLetter(input))
            {
                NEXT(parseStateCommandName);
                m_buffer.push_back(input);
                return ParseState::Incomplete;
            }

            if (IsDigit(input))
            {
                NEXT(parseStateCommandNumber);
                m_buffer.push_back(input);
                return ParseState::Incomplete;
            }

            return ParseState::Invalid;

        case parseStatePrefix:
            if (IsPrefixChar(input))
            {
                m_buffer.push_back(input);
                return ParseState::Incomplete;
            }

            if (input == ' ')
            {
                NEXT(parseStateCommandNameOrNumber);
                message.SetPrefix(std::move(m_buffer));
                m_buffer.clear();
                return ParseState::Incomplete;
            }

            return ParseState::Invalid;

        case parseStateCommandNameOrNumber:
            if (IsLetter(input))
            {
                NEXT(parseStateCommandName);
                m_buffer.push_back(input);
                return ParseState::Incomplete;
            }

            if (IsDigit(input))
            {
                NEXT(parseStateCommandNumber);
                m_buffer.push_back(input);
                return ParseState::Incomplete;
            }

            return ParseState::Invalid;

        case parseStateCommandName:
            if (IsLetter(input))
            {
                m_buffer.push_back(input);
                return ParseState::Incomplete;
            }

            if (input == ' ')
            {
                NEXT(parseStateParamStart);
                message.SetCommand(std::move(m_buffer));
                m_buffer.clear();
                return ParseState::Incomplete;
            }

            if (input == '\r')
            {
                NEXT(parseStateCr);
                message.SetCommand(std::move(m_buffer));
                m_buffer.clear();
                return ParseState::Incomplete;
            }

            return ParseState::Invalid;

        case parseStateCommandNumber:
            if (input == ' ' || input == '\r')
            {
                if (m_buffer.size() != 3)
                {
                    // TODO: Trace
                    return ParseState::Invalid;
                }

                m_state = input == ' ' ? parseStateParamStart : parseStateCr;
                message.SetCommand(std::move(m_buffer));
                m_buffer.clear();
                return ParseState::Incomplete;
            }
        
            if (IsDigit(input) && m_buffer.size() < 3)
            {
                m_buffer.push_back(input);
                return ParseState::Incomplete;
            }

            return ParseState::Invalid;

        case parseStateParamStart:
            if (input == ':')
            {
                // Don't buffer the trailer-start char
                NEXT(parseStateTrailing);
                return ParseState::Incomplete;
            }

            if (IsParamStartChar(input))
            {
                NEXT(parseStateParam);
                m_buffer.push_back(input);
                return ParseState::Incomplete;
            }

            return ParseState::Incomplete;

        case parseStateParam:
            if (input == ' ' || input == '\r')
            {
                m_state = input == ' ' ? parseStateParamStart : parseStateCr;
                message.AddParam(std::move(m_buffer));
                m_buffer.clear();
                return ParseState::Incomplete;
            }

            if (IsParamStartChar(input) || input == ':')
            {
                m_buffer.push_back(input);
                return ParseState::Incomplete;
            }

            return ParseState::Invalid;

        case parseStateTrailing:
            if (input == '\r')
            {
                NEXT(parseStateCr);
                message.AddParam(std::move(m_buffer));
                return ParseState::Incomplete;
            }

            if (IsParamStartChar(input) || input == ':' || input == ' ')
            {
                m_buffer.push_back(input);
                return ParseState::Incomplete;
            }

            return ParseState::Invalid;

        case parseStateCr:
            if (input == '\n')
            {
                NEXT(parseStateComplete);
                return ParseState::Complete;
            }

            return ParseState::Invalid;

        case parseStateComplete:
            return ParseState::Complete;

        default:
            std::stringstream ss;
            ss << "Invalid parse state: " << m_state;
            throw std::logic_error(ss.str());
    }

    return ParseState::Invalid;

    #undef NEXT
}

} // birch
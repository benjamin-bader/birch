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

#include "asio.hpp"

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
        || IsSpecial(c)
        || c == '!'
        || c == '.'
        || c == '@'
        || c == '-'  // hostnames can contain dashes
        || c == ':'; // IPv6 addresses have colons
}

inline constexpr bool IsParamStartChar(char c)
{
    int i = static_cast<int>(c);
    return (i >= 0x3B && i <= 0xFF)
        || (i >= 0x21 && i <= 0x39)
        || (i >= 0x0E && i <= 0x1F)
        || (i >= 0x0B && i <= 0x0C)
        || (i >= 0x01 && i <= 0x09);
}

bool IsValidHostname(const char* begin, const char* end) noexcept
{
    bool shortnameStart = true;

    if (begin == nullptr || end == nullptr || begin >= end)
    {
        return false;
    }

    const char* cur = begin;
    while (cur != end)
    {
        char c = *cur++;

        if (c == '!' || c == '@')
        {
            return false; // not a servername!
        }

        if (IsLetter(c) || IsDigit(c) || (!shortnameStart && c == '-'))
        {
            shortnameStart = false;
            continue;
        }
        else if (c == '.' && !shortnameStart)
        {
            shortnameStart = true;
            continue;
        }
        else
        {
            return false;
        }
    }

    return !shortnameStart;
}

bool IsInternetAddress(const char* c) noexcept
{
    // NOTE: Assumes that c is a null-terminated string!
    // this function is NOT SAFE for general use!
    asio::error_code ec;
    asio::ip::make_address(c, ec);
    return !ec;
}

} // anonymous namespace

void MessageParser::Reset()
{
    m_state = parseStateStart;
    m_buffer.clear();
}

ParseState MessageParser::Consume(Message& message, char input)
{
    #define NEXT(s) do { \
      std::cout << "-> " << #s << std::endl; \
      m_state = (s); \
    } while (0);

    if (input == '\0')
    {
        // the null character is never permitted
        return ParseState::Invalid;
    }

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
                if (!IsValidPrefix())
                {
                    return ParseState::Invalid;
                }

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

bool MessageParser::IsValidPrefix() const noexcept
{
    // could be either servername or nick [ [! user] @ host].
    // try servername first.

    const char* begin = &m_buffer[0];
    const char* end = begin + m_buffer.size();

    // servername must be a hostname; if the whole buffer is a valid
    // hostname, then we're fine
    if (IsValidHostname(begin, end))
    {
        return true;
    }

    // not a servername; validate nickname/user/host.

    enum {
        vsNickname,
        vsUser,
        vsHost
    } state = vsNickname;

    size_t nickLen = 0;
    size_t userLen = 0;
    size_t hostLen = 0;

    const char* userStart = end;
    const char* hostStart = end;

    auto cur = begin;
    while (cur != end)
    {
        char c = *cur++;

        switch (state)
        {
            case vsNickname:
                if (IsLetter(c) || IsSpecial(c))
                {
                    ++nickLen;
                }
                else if (IsDigit(c) || c == '-')
                {
                    if (nickLen == 0)
                    {
                        return false;
                    }
                    ++nickLen;
                }
                else if (c == '!')
                {
                    userStart = cur;
                    state = vsUser;
                }
                else if (c == '@')
                {
                    hostStart = cur;
                    state = vsHost;
                }
                else
                {
                    return false;
                }
                break;

            case vsUser:
                if (IsUserChar(c))
                {
                    ++userLen;
                }
                else if (c == '@')
                {
                    hostStart = cur;
                    state = vsHost;
                }
                else
                {
                    return false;
                }

                break;

            case vsHost:
                // Could be a hostname, IPv4 addr, or IPv6 addr.
                // Accept all the chars, and validate later.
                if (IsLetter(c) || IsHex(c) || c == '.' || c == ':' || c == '-')
                {
                    ++hostLen;
                }
                else
                {
                    return false;
                }

                break;
        }
    }

    // Nicknames are always required.  Hosts are optional,
    // unless there is also a username in which case the
    // host is required, too.
    if (nickLen < 1 || nickLen > 9)
    {
        return false;
    }

    if (hostStart == end)
    {
        return userStart == end;
    }

    if (userStart != end && userLen == 0)
    {
        return false;
    }

    return IsValidHostname(hostStart, end) || IsInternetAddress(hostStart);
}

} // birch
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

#include "Channel.h"

#include <algorithm>

namespace birch {

bool IsValidChannelName(const std::string& name)
{
    if (name.size() <= 1)
    {
        return false;
    }

    if (name[0] != '&' && name[0] != '#' && name[0] != '+' && name[0] != '!')
    {
        return false;
    }

    if (name.size() > 50)
    {
        return false;
    }

    // RFC 2811 2.1: channel names shall not have a comma, space, or ^G.
    // ':' is a "channel mask delimiter", which I think means it also is illegal.
    constexpr const char* c_InvalidChars = ",: \x07";
    if (name.find_first_of(c_InvalidChars) != std::string::npos)
    {
        return false;
    }

    return true;
}

}
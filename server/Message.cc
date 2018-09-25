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

#include "Message.h"

#include <utility>

namespace birch {

void Message::SetPrefix(const std::string& prefix)
{
    m_prefix = prefix;
}

void Message::SetPrefix(std::string&& prefix)
{
    m_prefix = std::move(prefix);
}

void Message::SetCommand(const std::string& command)
{
    m_command = command;
}

void Message::SetCommand(std::string&& command)
{
    m_command = std::move(command);
}

void Message::AddParam(const std::string& param)
{
    m_params.push_back(param);
}

void Message::AddParam(std::string&& param)
{
    m_params.emplace_back(std::move(param));
}

const std::string& Message::GetPrefix() const noexcept
{
    return m_prefix;
}

const std::string& Message::GetCommand() const noexcept
{
    return m_command;
}

const std::vector<std::string>& Message::GetParams() const noexcept
{
    return m_params;
}

}
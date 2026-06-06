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

#include <iostream>
#include <utility>

namespace birch::server {

MessageBuilder Message::Builder()
{
    return MessageBuilder();
}

void Message::AddTag(const std::string& key, const std::string& value)
{
    m_tags[key] = value;
}

void Message::AddTag(std::string&& key, std::string&& value)
{
    m_tags.emplace(std::move(key), std::move(value));
}

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

const absl::linked_hash_map<std::string, std::string>& Message::GetTags() const noexcept
{
    return m_tags;
}

std::ostream& operator<<(std::ostream& os, const Message& message)
{
    os << "Message(";
    os << "prefix=" << message.GetPrefix() << ", ";
    os << "command=" << message.GetCommand() << ", ";
    os << "params=[";
    for (const auto& param : message.GetParams())
    {
        os << param << ", ";
    }
    os << "])";
    return os;
}

MessageBuilder& MessageBuilder::WithPrefix(const std::string& prefix)
{
    m_message.SetPrefix(prefix);
    return *this;
}

MessageBuilder& MessageBuilder::WithPrefix(std::string&& prefix)
{
    m_message.SetPrefix(std::move(prefix));
    return *this;
}

MessageBuilder& MessageBuilder::WithCommand(const std::string& command)
{
    m_message.SetCommand(command);
    return *this;
}

MessageBuilder& MessageBuilder::WithCommand(std::string&& command)
{
    m_message.SetCommand(std::move(command));
    return *this;
}

MessageBuilder& MessageBuilder::WithParam(const std::string& param)
{
    m_message.AddParam(param);
    return *this;
}

MessageBuilder& MessageBuilder::WithParam(std::string&& param)
{
    m_message.AddParam(std::move(param));
    return *this;
}

MessageBuilder& MessageBuilder::WithTag(const std::string& key, const std::string& value)
{
    m_message.AddTag(key, value);
    return *this;
}

MessageBuilder& MessageBuilder::WithTag(std::string&& key, std::string&& value)
{
    m_message.AddTag(std::move(key), std::move(value));
    return *this;
}

Message MessageBuilder::Build() const &
{
    return m_message;
}

Message MessageBuilder::Build() &&
{
    return std::move(m_message);
}

} // namespace birch::server

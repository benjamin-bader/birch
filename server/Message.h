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

#ifndef BIRCH_SERVER_MESSAGE_H
#define BIRCH_SERVER_MESSAGE_H

#include <iosfwd>
#include <string>
#include <vector>

#include "absl/container/linked_hash_map.h"

namespace birch::server {

class MessageBuilder;

class Message
{
    std::string m_prefix;
    std::string m_command;
    std::vector<std::string> m_params;
    absl::linked_hash_map<std::string, std::string> m_tags;

public:
    static MessageBuilder Builder();

    Message() = default;
    Message(const Message&) = default;
    Message(Message&&) = default;

    Message& operator=(const Message&) = default;
    Message& operator=(Message&&) = default;

    void AddTag(const std::string& key, const std::string& value);
    void AddTag(std::string&& key, std::string&& value);

    void SetPrefix(const std::string& prefix);
    void SetPrefix(std::string&& prefix);

    void SetCommand(const std::string& command);
    void SetCommand(std::string&& command);

    void AddParam(const std::string& param);
    void AddParam(std::string&& param);

    const std::string& GetPrefix() const noexcept;
    const std::string& GetCommand() const noexcept;
    const std::vector<std::string>& GetParams() const noexcept;
    const absl::linked_hash_map<std::string, std::string>& GetTags() const noexcept;
};

class MessageBuilder
{
    Message m_message;

public:
    MessageBuilder() = default;

    MessageBuilder& WithPrefix(const std::string& prefix);
    MessageBuilder& WithPrefix(std::string&& prefix);

    MessageBuilder& WithCommand(const std::string& command);
    MessageBuilder& WithCommand(std::string&& command);

    MessageBuilder& WithParam(const std::string& param);
    MessageBuilder& WithParam(std::string&& param);

    MessageBuilder& WithTag(const std::string& key, const std::string& value);
    MessageBuilder& WithTag(std::string&& key, std::string&& value);

    Message Build() const &;
    Message Build() &&;
};

std::ostream& operator<<(std::ostream& os, const Message& message);

} // namespace birch::server

#endif

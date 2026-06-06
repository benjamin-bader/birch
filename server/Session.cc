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

#include "Session.h"

namespace birch::server {

Session::Session(std::weak_ptr<IConnection> connection)
    : m_connection{std::move(connection)}
{
}

void Session::SendResponse(const Message& message)
{
    if (auto connection = m_connection.lock())
    {
        connection->DeliverResponse(message);
    }
}

void Session::HandlePing(const Message& message)
{
    std::string token;
    if (message.GetParams().size() > 0)
    {
        token = *message.GetParams().rbegin();
    }

    Message response = Message::Builder()
        .WithCommand("PONG")
        .WithParam("TODO(server info here)")
        .WithParam(token)
        .Build();
    SendResponse(response);
}

} // namespace birch::server

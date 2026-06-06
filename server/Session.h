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

#ifndef BIRCH_SERVER_SESSION_H
#define BIRCH_SERVER_SESSION_H

#include <memory>
#include <string>

#include "server/Connection.h"

namespace birch::server {

/**
 * A Session represents a session of a user with the server.
 * It is responsible for mediating between the user's connection to the server,
 * and the server's internal state.
 */
class Session
    : public std::enable_shared_from_this<Session>
    , public IConnectionObserver
{
    std::weak_ptr<IConnection> m_connection;

    std::string m_nick;

public:
    Session(std::weak_ptr<IConnection> connection);
    virtual ~Session() = default;

public: // IConnectionObserver
    void OnMessage(IConnection::ConnId connId, const Message& message) override;
    void OnError(IConnection::ConnId connId, const absl::Status& status) override;
    void OnDisconnect(IConnection::ConnId connId) override;

private:
    void SendResponse(const Message& message);

    void HandlePing(const Message& message);
};

} // namespace birch::server

#endif // BIRCH_SERVER_SESSION_H

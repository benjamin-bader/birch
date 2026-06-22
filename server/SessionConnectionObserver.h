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

#ifndef BIRCH_SERVER_SESSION_CONNECTION_OBSERVER_H
#define BIRCH_SERVER_SESSION_CONNECTION_OBSERVER_H

#include <memory>

#include "absl/status/status.h"

#include "irc/Message.h"
#include "irc/Session.h"
#include "server/Connection.h"

namespace birch::server {

/**
 * A SessionConnectionObserver adapts a birch::server::IConnection to a Session,
 * driving the session with the messages and other events produced by the connection.
 */
class SessionConnectionObserver
    : public IConnectionObserver
    , public irc::ISessionObserver
{
    std::shared_ptr<irc::Session> m_session;
    std::shared_ptr<IConnection> m_connection;

public:
    SessionConnectionObserver(const std::shared_ptr<irc::Session>& session, const std::shared_ptr<IConnection>& connection);

public: // IConnectionObserver
    void OnMessage(IConnection::ConnId connId, const irc::Message& message) override;
    void OnError(IConnection::ConnId connId, const absl::Status& status) override;
    void OnDisconnect(IConnection::ConnId connId) override;

public: // ISessionObserver
    void OnMessageEmitted(const irc::Message& message) override;
};

} // namespace birch::server

#endif // BIRCH_SERVER_SESSION_CONNECTION_OBSERVER_H

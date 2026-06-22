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

#include "SessionConnectionObserver.h"

#include "absl/log/log.h"
#include "absl/status/status.h"

namespace birch::server {

SessionConnectionObserver::SessionConnectionObserver(
    const std::shared_ptr<irc::Session>& session,
    const std::shared_ptr<IConnection>& connection
)
    : m_session(session)
    , m_connection(connection)
{}

void SessionConnectionObserver::OnMessage(IConnection::ConnId connId, const irc::Message& message)
{
    if (m_session != nullptr)
    {
        m_session->AcceptMessage(message);
    }
}

void SessionConnectionObserver::OnError(IConnection::ConnId connId, const absl::Status& status)
{
    // This IConnectionObserver callback happens in response to a fatal transport-layer
    // problem; clean up the session and treat it as a disconnect.
    if (m_session != nullptr)
    {
        m_session->Close(status);
    }

    m_session = nullptr;
    m_connection = nullptr;
}

void SessionConnectionObserver::OnDisconnect(IConnection::ConnId connId)
{
    if (m_session != nullptr)
    {
        m_session->Close(absl::OkStatus());
    }

    m_session = nullptr;
    m_connection = nullptr;
}

// ISessionObserver

void SessionConnectionObserver::OnMessageEmitted(const irc::Message& message)
{
    if (m_connection != nullptr)
    {
        auto result = m_connection->DeliverResponse(message);
        if (result != WriteResult::Sent)
        {
            // TODO: Implement something intelligent related to backpressur
            LOG(WARNING) << "Failed to deliver message to conn " << m_connection->GetConnId() << "; writeResult=" << result;
        }
    }
}

} // namespace birch::irc

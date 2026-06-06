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

#ifndef BIRCH_SERVER_CONNECTION_H
#define BIRCH_SERVER_CONNECTION_H

#include <chrono>
#include <cstdint>

#include "absl/status/status.h"

#include "server/Message.h"

namespace birch::server {

class IUser;
class IChannel;

enum class ClientType
{
    User,
    Service
};

class IConnectionStats
{
public:
    virtual ~IConnectionStats() = default;

    virtual std::chrono::system_clock::time_point ConnectedAt() const = 0;
    virtual uint64_t NumMessagesSent() const noexcept = 0;
    virtual uint64_t NumMessagesReceived() const noexcept = 0;

    virtual uint64_t NumBytesSent() const noexcept = 0;
    virtual uint64_t NumBytesReceived() const noexcept = 0;
};

class IConnectionObserver;

enum class WriteResult
{
    Sent,
    Backpressure,
    Closed,
};

class IConnection
{
public:
    using ConnId = std::uint64_t;

    virtual ~IConnection() = default;

    virtual ConnId GetConnId() const = 0;

    // Starts the connection lifecycle
    virtual void OnConnected() = 0;

    virtual WriteResult DeliverResponse(const Message& message) = 0;

    virtual void AddObserver(const std::shared_ptr<IConnectionObserver>& observer) = 0;
    virtual void RemoveObserver(const std::shared_ptr<IConnectionObserver>& observer) = 0;
};

/**
 * An IConnectionObserver is an object that can observe events originating from an IConnection.
 *
 * Events:
 * - Message: A message was received from the connection.
 * - Error: An error occurred on the connection.
 * - Disconnect: The connection was disconnected.
 */
class IConnectionObserver
{
public:
    virtual ~IConnectionObserver() = default;

    virtual void OnMessage(IConnection::ConnId connId, const Message& message) = 0;
    virtual void OnError(IConnection::ConnId connId, const absl::Status& status) = 0;
    virtual void OnDisconnect(IConnection::ConnId connId) = 0;
};

// An IClientConnection is a connection from a client to this server.
class IClientConnection
{
public:
    virtual ~IClientConnection() = default;
};

// An IServerConnection is a connection from this server to another server.
class IServerConnection
{
public:
    virtual ~IServerConnection() = default;
};

} // namespace birch::server

#endif // BIRCH_SERVER_CONNECTION_H

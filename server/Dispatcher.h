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

#ifndef BIRCH_SERVER_DISPATCHER_H
#define BIRCH_SERVER_DISPATCHER_H

#include <memory>

#include "irc/Message.h"
#include "server/Connection.h"

namespace birch::server {

/**
 * An IDispatcher mediates between connections and the server.
 * It is responsible for receiving messages from connections, relaying
 * responses from the server, and managing backpressure between the two.
 */
class IDispatcher
{
public:
    using Ptr = std::unique_ptr<IDispatcher>;

    virtual ~IDispatcher() = default;

    virtual void OnMessage(const IClientConnection& connection, const irc::Message& message) = 0;
    virtual void OnMessage(const IServerConnection& connection, const irc::Message& message) = 0;
};

} // namespace birch::server

#endif // BIRCH_SERVER_DISPATCHER_H

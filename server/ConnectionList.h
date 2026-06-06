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

#ifndef BIRCH_SERVER_CONNECTIONLIST_H
#define BIRCH_SERVER_CONNECTIONLIST_H

#include "Connection.h"

namespace birch::server {

/**
 * An IConnectionList is a collection of connections.
 * It is responsible for managing the connections and
 * dispatching messages to the appropriate connections.
 */
class IConnectionList
{
public:
    virtual ~IConnectionList() = default;

    virtual void AddConnection(const IClientConnection& connection) = 0;
    virtual void RemoveConnection(const IClientConnection& connection) = 0;
    virtual void AddConnection(const IServerConnection& connection) = 0;
    virtual void RemoveConnection(const IServerConnection& connection) = 0;
    virtual void GetConnections() const = 0;
};

std::unique_ptr<IConnectionList> MakeConnectionList();

} // namespace birch::server

#endif

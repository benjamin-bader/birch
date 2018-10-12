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

#ifndef BIRCH_CONNECTION_H
#define BIRCH_CONNECTION_H

#include <chrono>
#include <cstdint>
#include <memory>

#include "asio.hpp"

namespace birch {

class IUser;
class IChannel;
class IConnection;

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

class IConnectionCallbacks
{
    virtual ~IConnectionCallbacks() = default;

    virtual void OnDataAvailable(const std::shared_ptr<IConnection>& conn) = 0;
};

class IConnection
{
public:
    virtual ~IConnection() = default;

    virtual void AddCallback(const std::shared_ptr<IConnectionCallbacks>& callback) = 0;
};

class IConnectionFactory
{
public:
    virtual ~IConnectionFactory() = default;

    virtual std::shared_ptr<IConnection> MakeConnection(asio::ip::tcp::socket&& socket) = 0;
};

}

#endif // BIRCH_CONNECTION_H
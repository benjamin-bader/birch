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

#include "Connection.h"

#include <array>
#include <utility>

namespace birch {

class Connection : public IConnection, std::enable_shared_from_this<Connection>
{
public:
    Connection(asio::ip::tcp::socket&& socket);
    virtual ~Connection();

    virtual void AddCallback(const std::shared_ptr<IConnectionCallbacks>& callback) override;

private:
    std::array<char, 512> m_buffer;
    asio::ip::tcp::socket m_socket;
};

class ConnectionFactory : public IConnectionFactory
{
public:
    ConnectionFactory() = default;
    virtual ~ConnectionFactory() = default;

    virtual std::shared_ptr<IConnection> MakeConnection(asio::ip::tcp::socket&& socket) override;
};

Connection::Connection(asio::ip::tcp::socket&& socket)
    : m_buffer()
    , m_socket(std::move(socket))
{}

Connection::~Connection()
{
    m_socket.close();
}

void Connection::AddCallback(const std::shared_ptr<IConnectionCallbacks>& callback)
{

}

std::shared_ptr<IConnection> ConnectionFactory::MakeConnection(asio::ip::tcp::socket&& socket)
{
    return std::make_shared<Connection>(std::forward<asio::ip::tcp::socket>(socket));
}

}
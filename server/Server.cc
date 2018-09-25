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

#include "Server.h"

#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include <openssl/ssl.h>
#include "asio/ssl.hpp"
#include "asio.hpp"

namespace birch {

using tcp = asio::ip::tcp;

class Server : public IServer
{
public:
    Server(uint16_t port);
    virtual ~Server();

    virtual void ServeForever() override;

private:
    asio::io_service m_io;
    asio::signal_set m_signals;
    tcp::acceptor m_acceptor;
    tcp::socket m_socket;
    std::vector<std::thread> m_workers;
};

Server::Server(uint16_t port)
    : m_io{}
    , m_signals{m_io}
    , m_acceptor{m_io}
    , m_socket{m_io}
    , m_workers{}
{
    m_signals.add(SIGHUP);
    m_signals.add(SIGTERM);
    m_signals.add(SIGKILL);
    #if defined(SIGQUIT)
    m_signals.add(SIGQUIT);
    #endif

    m_signals.async_wait([this](asio::error_code ec, int sig)
    {
        switch (sig)
        {

        }
    });

    unsigned int numCores = std::thread::hardware_concurrency();
    for (auto i = 0; i < numCores; ++i)
    {

    }
}

Server::~Server()
{

}

void Server::ServeForever()
{

}

std::unique_ptr<IServer> MakeServer(const IServerConfig& config)
{
    return std::make_unique<Server>(config.GetPort());
}

}
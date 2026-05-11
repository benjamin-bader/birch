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

#include <signal.h>

#include <memory>
#include <thread>
#include <vector>

#include "absl/log/log.h"

#include <asio/ssl.hpp>
#include <asio.hpp>

namespace birch {

using tcp = asio::ip::tcp;

class Server : public IServer, public std::enable_shared_from_this<Server>
{
    asio::io_context m_io;
    asio::signal_set m_signals;
    tcp::acceptor m_acceptor;
    tcp::socket m_socket;
    std::vector<std::thread> m_workers;

public:
    Server(uint16_t port);
    virtual ~Server();

    virtual void ServeForever() override;

private:
    void HandleSignal(int signal);
};

Server::Server(uint16_t port)
    : m_io{}
    , m_signals{m_io}
    , m_acceptor{m_io}
    , m_socket{m_io}
    , m_workers{}
{
}

Server::~Server()
{

}

void Server::ServeForever()
{
    #if defined(SIGHUP)
    m_signals.add(SIGHUP);
    #endif

    #if defined(SIGTERM)
    m_signals.add(SIGTERM);
    #endif

    #if defined(SIGKILL)
    m_signals.add(SIGKILL);
    #endif

    #if defined(SIGQUIT)
    m_signals.add(SIGQUIT);
    #endif

    auto self = shared_from_this();
    m_signals.async_wait([self](asio::error_code /*ec*/, int sig)
    {
        if (!self)
        {
            return;
        }

        self->HandleSignal(sig);
    });

    unsigned int numCores = std::thread::hardware_concurrency();
    if (numCores == 0)
    {
        numCores = 1;
    }

    for (auto i = 0; i < numCores; ++i)
    {
        m_workers.emplace_back([self]()
        {
            if (!self)
            {
                return;
            }

            self->m_io.run();
        });
    }

    for (auto& worker : m_workers)
    {
        worker.join();
    }
}

std::unique_ptr<IServer> MakeServer(const IServerConfig& config)
{
    return std::make_unique<Server>(config.GetPort());
}

void Server::HandleSignal(int signal)
{
    LOG(INFO) << "Received signal " << signal;

    switch (signal)
    {
    case SIGHUP:
        // TODO: Reload configuration rather than stopping
        m_acceptor.close();
        break;
    case SIGTERM:
        m_acceptor.close();
        break;
    case SIGKILL:
        m_acceptor.close();
        break;
    case SIGQUIT:
        m_acceptor.close();
        break;
    default:
        LOG(WARNING) << "Received unknown signal " << signal;
        break;
    }
}

} // namespace birch
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

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/log.h"

#include <asio/ssl.hpp>
#include <asio.hpp>

#include "server/AsioConnection.h"

namespace birch::server {

using tcp = asio::ip::tcp;

class Server
    : public IServer
    , public std::enable_shared_from_this<Server>
    , public IConnectionObserver
{
    using ExecType = asio::any_io_executor;
    using StrandType = asio::strand<ExecType>;

    asio::io_context m_io;
    StrandType m_strand;
    asio::signal_set m_signals;
    tcp::acceptor m_acceptor;
    std::vector<std::thread> m_workers;
    std::uint16_t m_listenPort;

    std::atomic_bool m_draining{false};
    std::atomic<IConnection::ConnId> m_nextConnId{1};
    absl::flat_hash_map<IConnection::ConnId, std::shared_ptr<AsioConnection>> m_connections;

public:
    Server(uint16_t port);
    ~Server() override;

public: // IServer
    void ServeForever() override;

public: // IConnectionObserver
    void OnMessage(IConnection::ConnId connId, const Message& message) override;
    void OnError(IConnection::ConnId connId, const absl::Status& status) override;
    void OnDisconnect(IConnection::ConnId connId) override;

private:
    asio::awaitable<void> AcceptConnections();

    void HandleSignal(int signal);
};

Server::Server(std::uint16_t port)
    : m_io{}
    , m_strand{m_io.get_executor()}
    , m_signals{m_io}
    , m_acceptor{m_io}
    , m_workers{}
    , m_listenPort{port}
    , m_connections{}
{
}

Server::~Server()
{
    asio::error_code ec;
    ec = m_signals.clear(ec);
    ec = m_acceptor.close(ec);
    m_io.stop();
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

    m_signals.async_wait([weakSelf = weak_from_this()](asio::error_code /*ec*/, int sig)
    {
        if (auto self = weakSelf.lock())
        {
            self->HandleSignal(sig);
        }
    });

    tcp::endpoint ep(tcp::v4(), static_cast<int>(m_listenPort));
    m_acceptor.open(ep.protocol());
    m_acceptor.set_option(asio::socket_base::reuse_address(true));
    m_acceptor.bind(ep);
    m_acceptor.listen();

    asio::co_spawn(m_strand, [self = shared_from_this()]() -> asio::awaitable<void> {
        co_await self->AcceptConnections();
    }, asio::detached);

    unsigned int numCores = std::thread::hardware_concurrency();
    if (numCores == 0)
    {
        numCores = 1;
    }

    for (auto i = 0; i < numCores; ++i)
    {
        m_workers.emplace_back([self = shared_from_this()]()
        {
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

asio::awaitable<void> Server::AcceptConnections()
{
    while (!m_draining.load(std::memory_order_relaxed))
    {
        try
        {
            auto socket = co_await m_acceptor.async_accept(asio::use_awaitable);

            if (m_draining.load(std::memory_order_relaxed))
            {
                LOG(WARNING) << "Server is draining, discarding incoming connection";
                co_return;
            }

            auto connId = m_nextConnId.fetch_add(1);
            auto conn = std::make_shared<AsioConnection>(connId, std::move(socket));
            m_connections.emplace(connId, conn);

            conn->AddObserver(shared_from_this());
            conn->OnConnected();
        }
        catch (asio::system_error& e)
        {
            LOG(ERROR) << "Error accepting connection: " << e.code().message();
            co_return;
        }
    }
}

// Server::IConnectionObserver implementation

void Server::OnMessage(IConnection::ConnId connId, const Message& message)
{
}

void Server::OnError(IConnection::ConnId connId, const absl::Status& status)
{
}

void Server::OnDisconnect(IConnection::ConnId connId)
{
    asio::dispatch(m_strand, [self = shared_from_this(), connId]()
    {
        self->m_connections.erase(connId);
    });
}

} // namespace birch::server

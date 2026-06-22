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

#include <algorithm>
#include <array>
#include <atomic>
#include <csignal>
#include <memory>
#include <limits>
#include <optional>
#include <set>
#include <thread>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"

#include <asio/awaitable.hpp>
#include <asio.hpp>

#include "server/AsioConnection.h"

namespace birch::server {

namespace {

constexpr const auto kTerminalSignals = std::array {
    SIGINT, SIGTERM,
};

constexpr const auto kReloadSignals = std::array {
#if defined(SIGBREAK)
    SIGBREAK,
#endif

#if defined(SIGHUP)
    SIGHUP,
#endif
};

struct ServerConfigData
{
    unsigned int NumWorkers;
    std::uint16_t Port;
    std::uint16_t TlsPort;
};

std::optional<ServerConfigData> ParseServerConfigFromValue(const config::IValue* value)
{
    if (value == nullptr)
    {

        return std::nullopt;
    }

    auto map = value->AsMap();
    if (map == nullptr)
    {
        return std::nullopt;
    }

    auto numWorkersValue = map->Get("num_workers");
    auto portValue = map->Get("port");
    auto tlsPortValue = map->Get("tls_port");

    ServerConfigData data{};

    if (numWorkersValue != nullptr)
    {
        // TODO: make this cast safe
        data.NumWorkers = numWorkersValue->AsInt64().value_or(0);
    }

    if (portValue != nullptr)
    {
        // TODO: make this cast safe
        data.Port = static_cast<std::uint16_t>(portValue->AsInt64().value_or(0));
    }

    if (tlsPortValue != nullptr)
    {
        // TODO: make this cast safe
        data.TlsPort = static_cast<std::uint16_t>(tlsPortValue->AsInt64().value_or(0));
    }

    return data;
}

class ServerConfig : public IServerConfig, public std::enable_shared_from_this<ServerConfig>
{
    std::shared_ptr<config::IConfigNode> m_serverConfigNode;
    ServerConfigData m_data;

public:
    ServerConfig(std::shared_ptr<config::IConfigNode>&& node, ServerConfigData&& data)
        : m_serverConfigNode(std::move(node))
        , m_data(std::move(data))
    {
        m_serverConfigNode->Subscribe([weakSelf = weak_from_this()](const config::ConfigPath& path, const config::IValue* value)
        {
            if (auto self = weakSelf.lock())
            {
                self->OnConfigChanged(path, value);
            }
        });
    }

    unsigned int GetNumWorkers() const override
    {
        return m_data.NumWorkers;
    }

    uint16_t GetPort() const override
    {
        return m_data.Port;
    }

    uint16_t GetTlsPort() const override
    {
        return m_data.TlsPort;
    }

    void OnConfigChanged(const config::ConfigPath& path, const config::IValue* value)
    {
        if (value == nullptr)
        {
            LOG(WARNING) << "Config updated with null value for " << path << "; ignoring update";
            return;
        }

        auto maybeConfigData = ParseServerConfigFromValue(value);
        if (!maybeConfigData)
        {
            LOG(WARNING) << "Config updated with malformed server config at " << path << "; ignoring update";
            return;
        }

        if (maybeConfigData->NumWorkers > 0)
        {
            m_data.NumWorkers = maybeConfigData->NumWorkers;
        }

        m_data.Port = maybeConfigData->Port;
        m_data.TlsPort = maybeConfigData->TlsPort;
    }
};

} // namespace

absl::StatusOr<std::shared_ptr<IServerConfig>> IServerConfig::Create(const std::shared_ptr<config::IConfig> &config)
{
    if (config == nullptr)
    {
        return absl::InvalidArgumentError("config node missing");
    }

    auto serverSection = config->GetNode(config::ConfigPath("server"));
    if (serverSection == nullptr)
    {
        return absl::InvalidArgumentError("config missing server section");
    }

    auto value = serverSection->GetValue();
    if (value == nullptr)
    {
        return absl::InvalidArgumentError("config server section has no value");
    }

    auto configData = ParseServerConfigFromValue(value);
    if (!configData)
    {
        return absl::InvalidArgumentError("config server section missing required data");
    }

    return std::make_shared<ServerConfig>(std::move(serverSection), std::move(*configData));
}

using tcp = asio::ip::tcp;

class Server
    : public IServer
    , public std::enable_shared_from_this<Server>
    , public IConnectionObserver
{
    using ExecType = asio::any_io_executor;
    using StrandType = asio::strand<ExecType>;

    std::shared_ptr<IServerConfig> m_config;

    asio::io_context m_io;
    StrandType m_strand;
    asio::signal_set m_signals;
    tcp::acceptor m_acceptor;
    std::vector<std::thread> m_workers;

    std::atomic_bool m_draining{false};
    std::atomic<IConnection::ConnId> m_nextConnId{1};
    absl::flat_hash_map<IConnection::ConnId, std::shared_ptr<AsioConnection>> m_connections;

public:
    Server(const std::shared_ptr<IServerConfig>&);
    ~Server() override;

public: // IServer
    void ServeForever() override;

public: // IConnectionObserver
    void OnMessage(IConnection::ConnId connId, const irc::Message& message) override;
    void OnError(IConnection::ConnId connId, const absl::Status& status) override;
    void OnDisconnect(IConnection::ConnId connId) override;

private:
    asio::awaitable<void> AcceptConnections();
    asio::awaitable<void> HandleSignals();
};

std::shared_ptr<IServer> IServer::Create(const std::shared_ptr<IServerConfig>& config)
{
    return std::make_shared<Server>(config);
}

Server::Server(const std::shared_ptr<IServerConfig>& config)
    : m_config{config}
    , m_io{}
    , m_strand{m_io.get_executor()}
    , m_signals{m_io}
    , m_acceptor{m_io}
    , m_workers{}
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
    for (const auto sig : kReloadSignals)
    {
        m_signals.add(sig);
    }

    for (const auto sig : kTerminalSignals)
    {
        m_signals.add(sig);
    };

    std::uint16_t listenPort = m_config->GetPort();

    tcp::endpoint ep(tcp::v4(), static_cast<int>(listenPort));
    m_acceptor.open(ep.protocol());
    m_acceptor.set_option(asio::socket_base::reuse_address(true));
    m_acceptor.bind(ep);
    m_acceptor.listen();

    LOG(INFO) << "listening on " << listenPort;

    asio::co_spawn(m_strand, [self = shared_from_this()]() -> asio::awaitable<void> {
        co_await self->HandleSignals();
    }, asio::detached);;

    asio::co_spawn(m_strand, [self = shared_from_this()]() -> asio::awaitable<void> {
        co_await self->AcceptConnections();
    }, asio::detached);

    for (auto i = 0; i < m_config->GetNumWorkers(); ++i)
    {
        m_workers.emplace_back([this]() { m_io.run(); });
    }

    for (auto& worker : m_workers)
    {
        worker.join();
    }
}

asio::awaitable<void> Server::HandleSignals()
{
    CHECK(m_strand.running_in_this_thread()) << "HandleSignals called off-strand";

    try
    {
        while (m_draining.load(std::memory_order_relaxed) == false)
        {
            int sig = co_await m_signals.async_wait(asio::use_awaitable);
            LOG(INFO) << "Received signal number " << sig;

            if (std::find(std::begin(kReloadSignals), std::end(kReloadSignals), sig) != kReloadSignals.end())
            {
                // Reload
                LOG(ERROR) << "OMG IMPLEMENT ME ALREADY";
                continue;
            }

            if (std::find(std::begin(kTerminalSignals), std::end(kTerminalSignals), sig) != std::end(kTerminalSignals))
            {
                LOG(INFO) << "Received terminal signal, shutting down server";
                m_draining.store(true, std::memory_order_relaxed);
                m_acceptor.close();

                // Actively close every live connection. We must Close() rather
                // than just clear the map: each connection's read/write loops
                // hold a shared_ptr to themselves, so erasing the map entry
                // alone won't run their destructors or cancel their parked
                // async ops. Close() cancels those ops, unwinding the coroutines
                // so the io_context can drain and run() can return.
                for (auto& [connId, conn] : m_connections)
                {
                    conn->Close();
                }
                break;
            }

            LOG(WARNING) << "Unexpected signal received: " << sig;
        }
    }
    catch (asio::system_error& e)
    {
        if (e.code() == asio::error::operation_aborted)
        {
            // Normal termination
            co_return;
        }

        LOG(ERROR) << "Error while waiting for signal: " << e.code();
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Error while waiting for signal: " << e.what();
    }
}

asio::awaitable<void> Server::AcceptConnections()
{
    CHECK(m_strand.running_in_this_thread()) << "AcceptConnections called off-strand";

    while (!m_draining.load(std::memory_order_relaxed))
    {
        auto [ec, socket] = co_await m_acceptor.async_accept(asio::as_tuple);
        if (ec == asio::error::operation_aborted)
        {
            // time to die - we've been shut down.
            break;
        }
        else if (ec)
        {
            LOG(ERROR) << "Error accepting incoming connections: " << ec.message();
            break;
        }
        else if (m_draining.load(std::memory_order_relaxed))
        {
            // Draining - we don't want new connections.
            break;
        }

        auto connId = m_nextConnId.fetch_add(1);
        auto conn = std::make_shared<AsioConnection>(connId, std::move(socket));
        m_connections.emplace(connId, conn);

        conn->AddObserver(shared_from_this());
        conn->OnConnected();
    }
}

// Server::IConnectionObserver implementation

void Server::OnMessage(IConnection::ConnId connId, const irc::Message& message)
{
}

void Server::OnError(IConnection::ConnId connId, const absl::Status& status)
{
}

void Server::OnDisconnect(IConnection::ConnId connId)
{
    LOG(INFO) << "ConnId " << connId << " disconnected; tearing it down.";

    asio::dispatch(m_strand, [self = shared_from_this(), connId]()
    {
        self->m_connections.erase(connId);
    });
}

} // namespace birch::server

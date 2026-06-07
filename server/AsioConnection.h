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

#ifndef BIRCH_SERVER_ASIOCONNECTION_H
#define BIRCH_SERVER_ASIOCONNECTION_H

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include "absl/status/status.h"

#include <asio/awaitable.hpp>
#include <asio/experimental/concurrent_channel.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/strand.hpp>

#include "server/Connection.h"
#include "server/Message.h"
#include "server/MessageParser.h"
#include "server/MessageSerializer.h"

namespace birch::server {

class AsioConnection : public IConnection, public std::enable_shared_from_this<AsioConnection>
{
    using tcp = asio::ip::tcp;

    ConnId m_connId;
    tcp::socket m_socket;
    asio::strand<asio::any_io_executor> m_strand;
    asio::experimental::concurrent_channel<void(asio::error_code, Message)> m_writeQueue;

    Message m_message;
    MessageParser m_parser;
    std::array<char, 4096> m_buffer;

    MessageSerializer m_serializer;
    std::array<char, 1024*12> m_writeBuffer;

    std::vector<std::weak_ptr<IConnectionObserver>> m_observers;

public:
    AsioConnection(ConnId connId, tcp::socket&& socket);
    ~AsioConnection() override;

    ConnId GetConnId() const override;

    void OnConnected() override;
    WriteResult DeliverResponse(const Message& message) override;
    void Close() override;

    void AddObserver(const std::shared_ptr<IConnectionObserver>& observer) override;
    void RemoveObserver(const std::shared_ptr<IConnectionObserver>& observer) override;

private:
    asio::awaitable<void> RunReadLoop();
    asio::awaitable<void> RunWriteLoop();

    void CloseUnderLock();

    void NotifyMessage(const Message& message);
    void NotifyError(const absl::Status& status);
    void NotifyDisconnect();
};

} // namespace birch

#endif

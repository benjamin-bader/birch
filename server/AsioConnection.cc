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

#include "server/AsioConnection.h"

#include <future>
#include <utility>

#include "absl/log/check.h"
#include "absl/log/log.h"

#include <asio.hpp>
#include <asio/awaitable.hpp>

namespace birch::server {

namespace {

constexpr const std::size_t kWriteQueueDefaultCapacity = 5;

// void ApplyToObservers(const std::vector<std::weak_ptr<IConnectionObserver>>& observers, std::function<void(const std::shared_ptr<IConnectionObserver>&)> func)
// {
//     for (const auto& observer : observers)
//     {
//         if (auto obs = observer.lock())
//         {
//             func(obs);
//         }
//     }
// }

} // namespace

AsioConnection::AsioConnection(ConnId connId, tcp::socket&& socket)
    : m_connId{connId}
    , m_socket{std::move(socket)}
    , m_strand{asio::make_strand(m_socket.get_executor())}
    , m_writeQueue{m_strand, kWriteQueueDefaultCapacity}
    , m_message{}
    , m_parser{}
    , m_buffer{}
    , m_serializer{}
    , m_writeBuffer{}
{
}

AsioConnection::~AsioConnection()
{
    CloseUnderLock();
}

IConnection::ConnId AsioConnection::GetConnId() const
{
    return m_connId;
}

void AsioConnection::OnConnected()
{
    asio::co_spawn(m_strand, [self = shared_from_this()]() -> asio::awaitable<void> {
        co_await self->RunReadLoop();
    }, asio::detached);

    asio::co_spawn(m_strand, [self = shared_from_this()]() -> asio::awaitable<void> {
        co_await self->RunWriteLoop();
    }, asio::detached);
}

WriteResult AsioConnection::DeliverResponse(const irc::Message& message)
{
    asio::error_code ec;
    if (m_writeQueue.try_send(ec, message))
    {
        return WriteResult::Sent;
    }
    else if (ec)
    {
        // channel has been closed
        return WriteResult::Closed;
    }
    else
    {
        // concurrent_channel returns false without setting an error code
        // if the channel is full.
        return WriteResult::Backpressure;
    }
}

void AsioConnection::AddObserver(const std::shared_ptr<IConnectionObserver>& observer)
{
    asio::post(m_strand, [this, self = shared_from_this(), observer]()
    {
        Observable::AddObserver(observer);
    });
}

void AsioConnection::RemoveObserver(const std::shared_ptr<IConnectionObserver>& observer)
{
    asio::post(m_strand, [this, self = shared_from_this(), observer]()
    {
        Observable::RemoveObserver(observer);
    });
}

asio::awaitable<void> AsioConnection::RunReadLoop()
{
    while (true)
    {
        auto [ec, length] = co_await m_socket.async_read_some(
            asio::buffer(m_buffer),
            asio::as_tuple);

        if (ec == asio::error::eof)
        {
            // Client hung up cleanly; tear the connection down so the write
            // loop is cancelled and observers are notified of the disconnect.
            Close();
            co_return;
        }
        else if (ec == asio::error::operation_aborted)
        {
            // We're shutting down; bail;
            Close();
            co_return;
        }
        else if (ec)
        {
            LOG(ERROR) << "Error reading from connection: " << ec.message();
            Close();
            co_return;
        }

        auto pos = m_buffer.begin();
        auto end = pos + length;

        while (pos != end)
        {
            auto parseState = m_parser.Parse(m_message, pos, end);

            if (parseState == ParseState::Complete)
            {
                NotifyMessage(m_message);

                m_message = {};
                m_parser.Reset();
            }
            else if (parseState == ParseState::Invalid)
            {
                LOG(ERROR) << "Invalid message received from client";
                break;
            }
            else if (parseState == ParseState::Incomplete)
            {
                // Incomplete message - keep reading from the socket.
                DCHECK(pos == end) << "MessageParser returned ParseState::Incomplete with buffered data remaining";
                break;
            }
            else
            {
                LOG(FATAL) << "Unexpected parseState value: " << parseState;
                co_return;
            }
        }
    }
}

asio::awaitable<void> AsioConnection::RunWriteLoop()
{
    while (true)
    {
        auto [ec, msg] = co_await m_writeQueue.async_receive(asio::as_tuple);

        if (ec == asio::error::operation_aborted)
        {
            LOG(INFO) << "Write loop shutting down due to connection closure, connId=" << m_connId;
            Close();
            co_return;
        }
        else if (ec)
        {
            LOG(ERROR) << "Error writing to connection: " << ec.message();
            Close();
            co_return;
        }

        std::size_t cbWrite = m_serializer.WriteToBuffer(m_writeBuffer, msg);

        auto [writeEc, cbWritten] = co_await asio::async_write(
            m_socket,
            asio::buffer(m_writeBuffer, cbWrite),
            asio::as_tuple);

        if (writeEc == asio::error::operation_aborted)
        {
            LOG(INFO) << "Write loop shutting down due to connection closure, connId=" << m_connId;
            Close();
            co_return;
        }
        else if (writeEc)
        {
            LOG(ERROR) << "Error writing to connection: " << writeEc.message();
            Close();
            co_return;
        }
    }
}

void AsioConnection::Close()
{
    asio::dispatch(m_strand, [self = shared_from_this()]()
    {
        self->CloseUnderLock();
    });
}

void AsioConnection::CloseUnderLock()
{
    if (!m_socket.is_open())
    {
        return;
    }

    m_writeQueue.close();
    m_writeQueue.cancel();

    if (asio::error_code ec; m_socket.shutdown(asio::socket_base::shutdown_both, ec))
    {
        if (ec != asio::error::not_connected)
        {
            LOG(ERROR) << "Error shutting down connection: " << ec.message();
        }
    }

    if (asio::error_code ec; m_socket.close(ec))
    {
        LOG(ERROR) << "Error closing connection: " << ec.message();
    }

    NotifyDisconnect();
}

void AsioConnection::NotifyMessage(const irc::Message& message)
{
    CHECK(m_strand.running_in_this_thread()) << "NotifyMessage called on wrong thread";

    Observable::Notify([connId = m_connId, &message](const auto& obs)
    {
        obs->OnMessage(connId, message);
    });
}

void AsioConnection::NotifyError(const absl::Status& status)
{
    CHECK(m_strand.running_in_this_thread()) << "NotifyError called on wrong thread";

    Observable::Notify([connId = m_connId, status = status](const auto& obs) { obs->OnError(connId, status); });
}

void AsioConnection::NotifyDisconnect()
{
    CHECK(m_strand.running_in_this_thread()) << "NotifyDisconnect called on wrong thread";

    Observable::Notify([connId = m_connId](const auto& obs) { obs->OnDisconnect(connId); });
}

} // namespace birch

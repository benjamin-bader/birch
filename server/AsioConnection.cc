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

#include <exception>
#include <future>
#include <utility>

#include "absl/log/check.h"
#include "absl/log/log.h"

#include <asio.hpp>
#include <asio/experimental/channel_error.hpp>

namespace birch::server {

namespace {

constexpr const std::size_t kWriteQueueDefaultCapacity = 5;

void ApplyToObservers(const std::vector<std::weak_ptr<IConnectionObserver>>& observers, std::function<void(const std::shared_ptr<IConnectionObserver>&)> func)
{
    for (const auto& observer : observers)
    {
        if (auto obs = observer.lock())
        {
            func(obs);
        }
    }
}

} // namespace

AsioConnection::AsioConnection(ConnId connId, tcp::socket&& socket)
    : m_connId{connId}
    , m_socket{std::move(socket)}
    , m_strand{asio::make_strand(m_socket.get_executor())}
    , m_writeQueue{m_strand, kWriteQueueDefaultCapacity}
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

WriteResult AsioConnection::DeliverResponse(const Message& message)
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
    asio::post(m_strand, [self = shared_from_this(), observer]()
    {
        self->m_observers.emplace_back(observer);
    });
}

void AsioConnection::RemoveObserver(const std::shared_ptr<IConnectionObserver>& observer)
{
    asio::post(m_strand, [self = shared_from_this(), observer]()
    {
        self->m_observers.erase(std::remove_if(self->m_observers.begin(), self->m_observers.end(), [observer](const auto& weakObserver) { return weakObserver.expired() || weakObserver.lock() == observer; }), self->m_observers.end());
    });
}

asio::awaitable<void> AsioConnection::RunReadLoop()
{
    try
    {
        while (true)
        {
            std::size_t length = co_await m_socket.async_read_some(
                asio::buffer(m_buffer),
                asio::use_awaitable);

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
                else
                {
                    // Incomplete message - keep reading
                    break;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        LOG(ERROR) << "Error in read-loop; connId= " << m_connId << " e=" << e.what();
        Close();
    }
}

asio::awaitable<void> AsioConnection::RunWriteLoop()
{
    try
    {
        while (true)
        {
            asio::error_code ec;
            auto msg = co_await m_writeQueue.async_receive(asio::use_awaitable);

            std::size_t cbWrite = m_serializer.WriteToBuffer(m_writeBuffer, msg);

            co_await asio::async_write(m_socket, asio::buffer(m_writeBuffer, cbWrite), asio::use_awaitable);
        }
    }
    catch (std::system_error& e)
    {
        if (e.code() == asio::experimental::error::channel_errors::channel_cancelled)
        {
            // Expected, if something closed the channel (read error, or external caller)
            LOG(INFO) << "Write loop shutting down due to connection closure, connId=" << m_connId;
        }
        else
        {
            LOG(ERROR) << "Exception in write loop; connId=" << m_connId << " e=" << e.what();
            Close();
        }
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Exception in write-loop; connId=" << m_connId << " e=" << e.what();
        Close();
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

    asio::error_code ec;
    m_socket.shutdown(asio::socket_base::shutdown_both, ec);
    if (ec)
    {
        LOG(ERROR) << "Error shutting down connection: " << ec.message();
    }

    m_socket.close(ec);
    if (ec)
    {
        LOG(ERROR) << "Error closing connection: " << ec.message();
    }

    NotifyDisconnect();

    m_observers.clear();
}

void AsioConnection::NotifyMessage(const Message& message)
{
    CHECK(m_strand.running_in_this_thread()) << "NotifyMessage called on wrong thread";

    ApplyToObservers(m_observers, [connId = m_connId, message = message](const std::shared_ptr<IConnectionObserver>& obs)
    {
        obs->OnMessage(connId, message);
    });
}

void AsioConnection::NotifyError(const absl::Status& status)
{
    CHECK(m_strand.running_in_this_thread()) << "NotifyError called on wrong thread";

    ApplyToObservers(m_observers, [connId = m_connId, status = status](const std::shared_ptr<IConnectionObserver>& obs)
    {
        obs->OnError(connId, status);
    });
}

void AsioConnection::NotifyDisconnect()
{
    CHECK(m_strand.running_in_this_thread()) << "NotifyDisconnect called on wrong thread";

    ApplyToObservers(m_observers, [connId = m_connId](const std::shared_ptr<IConnectionObserver>& obs)
    {
        obs->OnDisconnect(connId);
    });
}

} // namespace birch

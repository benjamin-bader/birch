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

#include "Dispatcher.h"

#include "absl/log/log.h"

namespace birch {

class Dispatcher : public IDispatcher
{
public:
    void OnMessage(const IClientConnection& connection, const Message& message) override;
    void OnMessage(const IServerConnection& connection, const Message& message) override;
};

void Dispatcher::OnMessage(const IClientConnection& connection, const Message& message)
{
    LOG(ERROR) << "Client connection received message: " << message;
}

void Dispatcher::OnMessage(const IServerConnection& connection, const Message& message)
{
    LOG(ERROR) << "Server connection received message: " << message;
}

}

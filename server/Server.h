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

#ifndef BIRCH_SERVER_H
#define BIRCH_SERVER_H

#include <cstdint>
#include <memory>

namespace birch {

class IServerConfig
{
public:
    virtual ~IServerConfig() = default;

    virtual uint16_t GetPort() const = 0;
    virtual uint16_t GetTlsPort() const = 0;
};

class IServer
{
public:
    virtual ~IServer() = default;

    virtual void ServeForever() = 0;
};

std::unique_ptr<IServer> MakeServer(const IServerConfig& config);

}

#endif
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

#ifndef BIRCH_IRC_SESSION_H
#define BIRCH_IRC_SESSION_H

#include <cstdint>
#include <memory>
#include <vector>

#include "absl/status/status.h"

#include "irc/CapabilitySet.h"
#include "irc/Message.h"
#include "util/Observable.h"

namespace birch::irc {

using SessionId = std::uint64_t;

class ISessionObserver
{
public:
    virtual ~ISessionObserver() = default;

    /**
     * OnMessageEmitted is invoked whenever the observed session emits a message for its
     * remote endpoint.
     */
    virtual void OnMessageEmitted(const Message& message) = 0;
};

class PendingRegistration
{
    std::string m_requestedNick;
    std::string m_user;

public:
    PendingRegistration();

    absl::Status RequestNick(const std::string& nick);
    absl::Status SetUser(const std::string& user);

    const std::string& GetUser() const;
    const std::string& GetNick() const;
};

/**
 * Represents a user's session with Birch.
 *
 * A session encapsulates the entire lifecycle of a client connection, from registration
 * and capability negotiation, to active use, to termination.
 *
 * Sessions begin in the "registering" phase; clients are expected to provide the usual
 * USER/NICK couplet.
 */
class Session : protected util::Observable<ISessionObserver>
{
    enum class Phase
    {
        Registering = 0,
        Active,
    };

    SessionId m_id{0};
    Phase m_phase{Phase::Registering};
    CapabilitySet m_caps;

public:
    Session(SessionId id);
    ~Session() override = default;

    SessionId GetId() const;

    void AcceptMessage(const Message& message);
    void Close(const absl::Status& status = absl::OkStatus());

    void AddSessionObserver(const std::shared_ptr<ISessionObserver>& observer);
    void RemoveSessionObserver(const std::shared_ptr<ISessionObserver>& observer);

private:

};

} // namespace birch::irc

#endif

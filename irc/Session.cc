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

#include "Session.h"

namespace birch::irc {

Session::Session(SessionId id)
    : m_id{id}
{}

SessionId Session::GetId() const
{
    return m_id;
}

void Session::AcceptMessage(const Message& message)
{
    // TODO
}

void Session::AddSessionObserver(const std::shared_ptr<ISessionObserver>& observer)
{
    AddObserver(observer);
}

void Session::RemoveSessionObserver(const std::shared_ptr<ISessionObserver>& observer)
{
    RemoveObserver(observer);
}

} // namespace birch::irc

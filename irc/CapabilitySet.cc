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

#include "CapabilitySet.h"

#include <utility>

#include "absl/log/check.h"

namespace birch::irc {

Capability::Capability(const std::string& name)
    : m_name(name)
    , m_values()
{}

Capability::Capability(std::string&& name)
    : m_name(std::move(name))
    , m_values()
{}

const std::string& Capability::GetName() const
{
    return m_name;
}

void Capability::SetValue(const std::string& name, const std::string& value)
{
    m_values.emplace(name, value);
}

void Capability::EmplaceValue(std::string&& name, std::string&& value)
{
    m_values.emplace(std::move(name), std::move(value));
}

std::optional<std::string> Capability::GetValue(const std::string& name) const
{
    const auto it = m_values.find(name);
    if (it != m_values.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::size_t CapabilitySet::GetSize() const
{
    return m_caps.size();
}

bool CapabilitySet::HasCapability(const std::string& name) const
{
    return m_caps.contains(name);
}

Capability* CapabilitySet::Add(const std::string& name)
{
    auto result = Find(name);
    if (result == nullptr)
    {
        auto [it, didEmplace] = m_caps.emplace(name, std::make_unique<Capability>(name));
        CHECK(didEmplace) << "whoops we lost a race";
        return it->second.get();
    }
    else
    {
        return result;
    }
}

Capability* CapabilitySet::Find(const std::string& name) const
{
    Capability* result = nullptr;

    auto it = m_caps.find(name);
    if (it != m_caps.end())
    {
        result = it->second.get();
    }

    return result;
}

void CapabilitySet::Revoke(const std::string& name)
{
    m_caps.erase(name);
}

} // namespace birch::irc

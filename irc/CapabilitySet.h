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

#ifndef BIRCH_IRC_CAPABILITY_SET_H
#define BIRCH_IRC_CAPABILITY_SET_H

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"

namespace birch::irc {

class Capability
{
    std::string m_name;
    absl::flat_hash_map<std::string, std::string> m_values;

public:
    Capability(const std::string& name);
    Capability(std::string&& name);

    const std::string& GetName() const;

    void SetValue(const std::string& name, const std::string& value);
    void EmplaceValue(std::string&& name, std::string&& value);

    std::optional<std::string> GetValue(const std::string& name) const;
};

class CapabilitySet
{
    absl::flat_hash_map<std::string, std::unique_ptr<Capability>> m_caps;

public:
    CapabilitySet() = default;
    CapabilitySet(const CapabilitySet&) = default;
    CapabilitySet(CapabilitySet&&) = default;

    CapabilitySet& operator=(const CapabilitySet&) = default;
    CapabilitySet& operator=(CapabilitySet&&) = default;

    std::size_t GetSize() const;

    bool HasCapability(const std::string& name) const;

    Capability* Add(const std::string& name);
    Capability* Find(const std::string& name) const;
    void Revoke(const std::string& name);
};

} // namespace birch::irc

#endif

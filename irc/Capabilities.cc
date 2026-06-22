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

#include "Capabilities.h"

#include <array>

#include "absl/log/log.h"
#include "absl/log/check.h"
#include "google/protobuf/descriptor.h"

#include "proto/caps.pb.h"

namespace birch::irc::Capabilities {

namespace {

using CapsArray = std::array<proto::CapabilitySpec, proto::Capabilities_ARRAYSIZE>;

const CapsArray& GetCaps()
{
    // Why a static local?  Because protobuf depends on static initialization to
    // properly register extensions, etc, and static initialization order is not
    // sufficiently deterministic.
    static const CapsArray kCaps = []() {
        CapsArray result{};

        auto desc = proto::Capabilities_descriptor();
        for (auto ix = 0; ix < desc->value_count(); ++ix)
        {
            auto value = desc->value(ix);
            CHECK(value != nullptr) << "lolwut";

            if (value->options().HasExtension(proto::capability_spec))
            {
                result[ix].CopyFrom(value->options().GetExtension(proto::capability_spec));
            }
            else
            {
                LOG(FATAL) << "Capability enum " << value->full_name() << " does not have a spec";
            }
        }

        return result;
    }();

    return kCaps;
}

} // namespace

std::span<const proto::CapabilitySpec> GetAll()
{
    return GetCaps();
}

} // namespace birch::irc::Capabilities

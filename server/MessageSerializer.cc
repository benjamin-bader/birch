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

#include "server/MessageSerializer.h"

#include <cstddef>
#include <cstring>
#include <string_view>

namespace birch::server {

namespace {

// A sink that writes nothing and merely tallies how many bytes a message
// would occupy once serialized.
class MeasuringSink
{
    std::size_t m_size{0};

public:
    void Append(std::string_view text) noexcept { m_size += text.size(); }
    void Append(char) noexcept { ++m_size; }

    std::size_t Size() const noexcept { return m_size; }
};

// A sink that writes serialized bytes into a caller-provided buffer.
//
// The buffer is assumed to be large enough; size it first with a
// MeasuringSink (i.e. ComputeRequiredSpace).
class BufferSink
{
    std::span<char> m_buffer;
    std::size_t m_offset{0};

public:
    explicit BufferSink(std::span<char> buffer) noexcept
        : m_buffer(buffer)
    {
    }

    void Append(std::string_view text) noexcept
    {
        std::memcpy(m_buffer.data() + m_offset, text.data(), text.size());
        m_offset += text.size();
    }

    void Append(char c) noexcept { m_buffer[m_offset++] = c; }

    std::size_t Size() const noexcept { return m_offset; }
};

// Returns true if `param` must be sent as an IRC "trailing" parameter,
// i.e. prefixed with ':' because it is empty, starts with ':', or contains
// a space.
bool RequiresTrailingMarker(std::string_view param) noexcept
{
    return param.empty()
        || param.front() == ':'
        || param.find(' ') != std::string_view::npos;
}

// The single source of truth for the wire format. Both the measuring and
// writing paths funnel through here, so they can never drift apart.
template <typename Sink>
void SerializeMessage(Sink& sink, const Message& message)
{
    const auto& tags = message.GetTags();
    if (!tags.empty())
    {
        sink.Append('@');
        bool first = true;
        for (const auto& [key, value] : tags)
        {
            if (!first)
            {
                sink.Append(';');
            }
            first = false;

            sink.Append(key);
            if (!value.empty())
            {
                sink.Append('=');
                sink.Append(value);
            }
        }
        sink.Append(' ');
    }

    if (!message.GetPrefix().empty())
    {
        sink.Append(':');
        sink.Append(message.GetPrefix());
        sink.Append(' ');
    }

    sink.Append(message.GetCommand());

    const auto& params = message.GetParams();
    for (std::size_t i = 0; i < params.size(); ++i)
    {
        sink.Append(' ');

        const auto& param = params[i];
        const bool isLast = (i + 1 == params.size());
        if (isLast && RequiresTrailingMarker(param))
        {
            sink.Append(':');
        }
        sink.Append(param);
    }

    sink.Append('\r');
    sink.Append('\n');
}

} // namespace

std::size_t MessageSerializer::ComputeRequiredSpace(const Message& message)
{
    MeasuringSink sink;
    SerializeMessage(sink, message);
    return sink.Size();
}

std::size_t MessageSerializer::WriteToBuffer(std::span<char> buffer, const Message& message)
{
    BufferSink sink{buffer};
    SerializeMessage(sink, message);
    return sink.Size();
}

} // namespace birch::server

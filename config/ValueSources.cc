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

#include "ValueSources.h"

#include <charconv>

namespace birch {

Int64ValueSource::Int64ValueSource(const std::string& key, const std::optional<std::string>& value)
    : m_key(key)
    , m_value(std::nullopt)
{
    if (!value.has_value())
    {
        return;
    }

    std::int64_t parsedValue;
    auto [ptr, ec] = std::from_chars(value->data(), value->data() + value->size(), parsedValue);

    if (ec == std::errc{} && ptr == value->data() + value->size())
    {
        m_value = parsedValue;
    }
}

bool Int64ValueSource::IsValid() const
{
    return m_value.has_value();
}

std::optional<std::int64_t> Int64ValueSource::As() const
{
    return m_value;
}
 

StringValueSource::StringValueSource(const std::string& key, const std::optional<std::string>& value)
    : m_key(key)
    , m_value(value)
{
}

bool StringValueSource::IsValid() const
{
    return m_value.has_value();
}

std::optional<std::string> StringValueSource::As() const
{
    return m_value;
}

BoolValueSource::BoolValueSource(const std::string& key, const std::optional<std::string>& value)
    : m_key(key)
    , m_value(std::nullopt)
{
    if (!value.has_value())
    {
        return;
    }

    if (*value == "true")
    {
        m_value = true;
    }
    else if (*value == "false")
    {
        m_value = false;
    }
}

bool BoolValueSource::IsValid() const
{
    return m_value.has_value();
}

std::optional<bool> BoolValueSource::As() const
{
    return m_value;
}

DoubleValueSource::DoubleValueSource(const std::string& key, const std::optional<std::string>& value)
    : m_key(key)
    , m_value(std::nullopt)
{
    if (!value.has_value())
    {
        return;
    }

    double parsedValue;
    auto [ptr, ec] = std::from_chars(value->data(), value->data() + value->size(), parsedValue);

    if (ec == std::errc{} && ptr == value->data() + value->size())
    {
        m_value = parsedValue;
    }
}

bool DoubleValueSource::IsValid() const
{
    return m_value.has_value();
}

std::optional<double> DoubleValueSource::As() const
{
    return m_value;
}

} // namespace birch

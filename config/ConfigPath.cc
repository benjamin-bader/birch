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

#include "ConfigPath.h"

#include <ostream>

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"

namespace birch::config {

ConfigPath::ConfigPath(std::vector<std::string> segments)
    : m_path(std::move(segments))
{
}

ConfigPath::ConfigPath(const std::vector<std::string>& segments)
    : m_path(segments)
{
}

ConfigPath::ConfigPath(std::initializer_list<std::string> segments)
    : m_path(segments)
{
}

ConfigPath::ConfigPath(std::string_view dotted_path)
    : m_path(absl::StrSplit(dotted_path, '.'))
{
}

ConfigPath& ConfigPath::operator=(std::vector<std::string> segments)
{
    m_path = std::move(segments);
    return *this;
}

ConfigPath& ConfigPath::operator=(const std::vector<std::string>& segments)
{
    m_path = segments;
    return *this;
}

ConfigPath& ConfigPath::operator=(std::string_view dotted_path)
{
    m_path = absl::StrSplit(dotted_path, '.');
    return *this;
}

void swap(ConfigPath& lhs, ConfigPath& rhs) noexcept
{
    lhs.swap(rhs);
}

void ConfigPath::swap(ConfigPath& other) noexcept
{
    m_path.swap(other.m_path);
}

ConfigPath::size_type ConfigPath::size() const noexcept
{
    return m_path.size();
}

bool ConfigPath::empty() const noexcept
{
    return m_path.empty();
}

const std::string& ConfigPath::operator[](size_type index) const
{
    return m_path[index];
}

const std::string& ConfigPath::at(size_type index) const
{
    return m_path.at(index);
}

const std::vector<std::string>& ConfigPath::segments() const noexcept
{
    return m_path;
}

ConfigPath::const_iterator ConfigPath::begin() const noexcept
{
    return m_path.begin();
}

ConfigPath::const_iterator ConfigPath::end() const noexcept
{
    return m_path.end();
}

std::string ConfigPath::ToString() const
{
    return absl::StrJoin(m_path, ".");
}

ConfigPath ConfigPath::Append(std::string_view key) const
{
    ConfigPath result;
    result.m_path = m_path;
    result.m_path.emplace_back(key);
    return result;
}

ConfigPath ConfigPath::Append(const ConfigPath& path) const
{
    ConfigPath result;
    result.m_path.reserve(m_path.size() + path.m_path.size());
    result.m_path = m_path;
    result.m_path.insert(result.m_path.end(), path.m_path.begin(), path.m_path.end());
    return result;
}

std::ostream& operator<<(std::ostream& os, const ConfigPath& path)
{
    return os << path.ToString();
}

} // namespace birch

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

#ifndef BIRCH_CONFIG_CONFIGPATH_H
#define BIRCH_CONFIG_CONFIGPATH_H

#include <compare>
#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"

namespace birch::config {

// Dot-separated configuration key path (e.g. "server.port").
class ConfigPath
{
    std::vector<std::string> m_path;

public:
    using value_type = std::string;
    using size_type = std::size_t;
    using const_iterator = std::vector<std::string>::const_iterator;

    ConfigPath() = default;
    ConfigPath(const ConfigPath&) = default;
    ConfigPath(ConfigPath&&) noexcept = default;
    ~ConfigPath() = default;

    explicit ConfigPath(std::vector<std::string> segments);
    explicit ConfigPath(const std::vector<std::string>& segments);
    ConfigPath(std::initializer_list<std::string> segments);
    ConfigPath(std::string_view dotted_path);

    ConfigPath& operator=(const ConfigPath&) = default;
    ConfigPath& operator=(ConfigPath&&) noexcept = default;
    ConfigPath& operator=(std::vector<std::string> segments);
    ConfigPath& operator=(const std::vector<std::string>& segments);
    ConfigPath& operator=(std::string_view dotted_path);

    friend void swap(ConfigPath& lhs, ConfigPath& rhs) noexcept;

    void swap(ConfigPath& other) noexcept;

    friend bool operator==(const ConfigPath&, const ConfigPath&) = default;
    auto operator<=>(const ConfigPath&) const = default;

    [[nodiscard]] std::string ToString() const;
    [[nodiscard]] ConfigPath Append(std::string_view key) const;
    [[nodiscard]] ConfigPath Append(const ConfigPath& path) const;

    [[nodiscard]] size_type size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::string& operator[](size_type index) const;
    [[nodiscard]] const std::string& at(size_type index) const;
    [[nodiscard]] const std::vector<std::string>& segments() const noexcept;
    [[nodiscard]] const_iterator begin() const noexcept;
    [[nodiscard]] const_iterator end() const noexcept;

    template <typename Sink>
    friend void AbslStringify(Sink& sink, const ConfigPath& path)
    {
        absl::Format(&sink, "%v", path.ToString());
    }

    template <typename H>
    friend H AbslHashValue(H h, const ConfigPath& path)
    {
        return H::combine(std::move(h), path.ToString());
    }
};

std::ostream& operator<<(std::ostream& os, const ConfigPath& path);

} // namespace birch

#endif // BIRCH_CONFIG_CONFIGPATH_H

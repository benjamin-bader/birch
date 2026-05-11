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

#ifndef BIRCH_CONFIG_VALUESOURCES_H
#define BIRCH_CONFIG_VALUESOURCES_H

#include <cstdint>
#include <optional>
#include <string>

#include "IValueSource.h"

namespace birch {

class Int64ValueSource : public IValueSource<std::int64_t>
{
    std::string m_key;
    std::optional<std::int64_t> m_value;

public:
    Int64ValueSource(const std::string& key, const std::optional<std::string>& value);

    bool IsValid() const override;

    std::optional<std::int64_t> As() const override;
};

class StringValueSource : public IValueSource<std::string>
{
    std::string m_key;
    std::optional<std::string> m_value;

public:
    StringValueSource(const std::string& key, const std::optional<std::string>& value);

    bool IsValid() const override;

    std::optional<std::string> As() const override;
};

class BoolValueSource : public IValueSource<bool>
{
    std::string m_key;
    std::optional<bool> m_value;

public:
    BoolValueSource(const std::string& key, const std::optional<std::string>& value);

    bool IsValid() const override;

    std::optional<bool> As() const override;
};

class DoubleValueSource : public IValueSource<double>
{
    std::string m_key;
    std::optional<double> m_value;

public:
    DoubleValueSource(const std::string& key, const std::optional<std::string>& value);

    bool IsValid() const override;

    std::optional<double> As() const override;
};

}

#endif // BIRCH_CONFIG_VALUESOURCES_H
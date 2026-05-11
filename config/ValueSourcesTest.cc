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

#include "gtest/gtest.h"

namespace birch {

TEST(ValueSourcesTest, Int64ValueSource)
{
    Int64ValueSource valueSource("test", "123");
    EXPECT_TRUE(valueSource.IsValid());
    EXPECT_EQ(valueSource.As(), 123);
}

TEST(ValueSourcesTest, StringValueSource)
{
    StringValueSource valueSource("test", "hello");
    EXPECT_TRUE(valueSource.IsValid());
    EXPECT_EQ(valueSource.As(), "hello");
}

TEST(ValuesSourcesTest, StringValueSourceNullopt)
{
    StringValueSource valueSource("test", std::nullopt);
    EXPECT_FALSE(valueSource.IsValid());
    EXPECT_EQ(valueSource.As(), std::nullopt);
}

TEST(ValuesSourcesTest, StringValueSourceEmpty)
{
    StringValueSource valueSource("test", "");
    EXPECT_TRUE(valueSource.IsValid());
    EXPECT_EQ(valueSource.As(), "");
}

TEST(ValueSourcesTest, BoolValueSource)
{
    BoolValueSource valueSource("test", "true");
    EXPECT_TRUE(valueSource.IsValid());
    EXPECT_EQ(valueSource.As(), true);

    valueSource = { "test", "false" };
    EXPECT_TRUE(valueSource.IsValid());
    EXPECT_EQ(valueSource.As(), false);

    valueSource = { "test", "invalid" };
    EXPECT_FALSE(valueSource.IsValid());
    EXPECT_EQ(valueSource.As(), std::nullopt);
}

TEST(ValueSourcesTest, DoubleValueSource)
{
    DoubleValueSource valueSource("test", "1.23");
    EXPECT_TRUE(valueSource.IsValid());
    EXPECT_EQ(valueSource.As(), 1.23);
}

} // namespace birch

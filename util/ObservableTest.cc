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

#include "util/Observable.h"

#include <memory>
#include <utility>

#include "gtest/gtest.h"

namespace birch::util {

struct TestObserver
{
    int calls{0};
};

class TestObservable : public Observable<TestObserver>
{
public:
    void AddObserver(const std::shared_ptr<TestObserver>& obs)
    {
        Observable::AddObserver(obs);
    }

    void RemoveObserver(const std::shared_ptr<TestObserver>& obs)
    {
        Observable::RemoveObserver(obs);
    }

    template <typename F>
    void Notify(F&& fn)
    {
        Observable::Notify(std::move(fn));
    }
};

TEST(ObservableTest, BasicFunctionalityWorks)
{
    auto obs = std::make_shared<TestObserver>();
    TestObservable subject;
    subject.AddObserver(obs);

    subject.Notify([](const auto& observer) {
        observer->calls++;
    });

    EXPECT_EQ(1, obs->calls);
}

} // namespace birch::util

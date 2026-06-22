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

#ifndef BIRCH_UTIL_OBSERVABLE_H
#define BIRCH_UTIL_OBSERVABLE_H

#include <algorithm>
#include <concepts>
#include <memory>
#include <mutex>
#include <vector>

namespace birch::util {

/**
 * An Observable class can be subscribed to by observers.  It holds weak pointers to those
 * observers, so subscribing does not keep an observer alive; expired observers are pruned
 * lazily on subscription changes and during notification.  All operations are thread-safe.
 *
 * There is no public API - classes are expected to derive Observable and expose their own
 * API as appropriate; Notify is never intended to be exposed to external callers, but
 * AddObserver and RemoveObserver are good candidates.
 */
template <typename Observer>
class Observable
{
    std::mutex m_mutex;
    std::vector<std::weak_ptr<Observer>> m_observers;

public:
    Observable()
        : m_mutex{}
        , m_observers{}
    {}

    virtual ~Observable() = default;

    Observable(const Observable&) = delete;
    Observable(Observable&&) = delete;

    Observable& operator=(const Observable&) = delete;
    Observable& operator=(Observable&&) = delete;

protected:
    void AddObserver(const std::shared_ptr<Observer>& observer)
    {
        std::lock_guard lock(m_mutex);
        m_observers.push_back(observer);
        EraseDeallocated();
    }

    void RemoveObserver(const std::shared_ptr<Observer>& observer)
    {
        std::lock_guard lock(m_mutex);
        m_observers.erase(
            std::remove_if(m_observers.begin(), m_observers.end(), [&observer](const auto& weak) {
                auto ptr = weak.lock();
                return !ptr || ptr == observer;
            }),
            m_observers.end()
        );
    }

    void Clear()
    {
        std::lock_guard lock(m_mutex);
        m_observers.clear();
    }

    template <typename Func>
    requires std::invocable<Func, std::shared_ptr<Observer>>
    void Notify(Func&& fn)
    {
        std::vector<std::shared_ptr<Observer>> liveObservers;
        {
            std::lock_guard lock(m_mutex);
            for (const auto& weakPtr : m_observers)
            {
                if (auto ptr = weakPtr.lock())
                {
                    liveObservers.push_back(ptr);
                }
            }

            if (liveObservers.size() != m_observers.size())
            {
                EraseDeallocated();
            }
        }

        for (const auto& observer : liveObservers)
        {
            fn(observer);
        }
    }

private:
    void EraseDeallocated()
    {
        m_observers.erase(
            std::remove_if(m_observers.begin(), m_observers.end(), [](const auto& ptr) { return ptr.expired(); }),
            m_observers.end()
        );
    }
};

} // namespace birch::util

#endif

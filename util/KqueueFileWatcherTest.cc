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

#include "util/KqueueFileWatcher.h"

#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace birch::util {

namespace {

// Generous compared to the watcher's 50ms coalesce window; tests fail fast on
// the happy path and only consume the full budget when something is broken.
constexpr auto kCallbackTimeout = std::chrono::milliseconds(2000);

// How long to wait when asserting that a callback does NOT fire.
constexpr auto kQuiescentWindow = std::chrono::milliseconds(250);

} // namespace

// Captures callbacks from the watcher thread and lets test bodies wait on
// them. All public methods are safe to call from any thread.
class CallbackTracker
{
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<std::filesystem::path> m_paths;

public:
    IFileWatcher::Callback MakeCallback()
    {
        return [this](const std::filesystem::path& path) { Notify(path); };
    }

    bool WaitForCount(std::size_t count,
                      std::chrono::milliseconds timeout = kCallbackTimeout)
    {
        std::unique_lock lock(m_mutex);
        return m_cv.wait_for(lock, timeout, [&] { return m_paths.size() >= count; });
    }

    std::size_t Count()
    {
        std::lock_guard lock(m_mutex);
        return m_paths.size();
    }

    std::vector<std::filesystem::path> Paths()
    {
        std::lock_guard lock(m_mutex);
        return m_paths;
    }

    void Reset()
    {
        std::lock_guard lock(m_mutex);
        m_paths.clear();
    }

private:
    void Notify(const std::filesystem::path& path)
    {
        std::lock_guard lock(m_mutex);
        m_paths.push_back(path);
        m_cv.notify_all();
    }
};

class KqueueFileWatcherTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        std::filesystem::path base = ::testing::TempDir();
        std::string tmpl = (base / "kqueue_watcher_test.XXXXXX").string();
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');

        char* result = ::mkdtemp(buf.data());
        ASSERT_NE(result, nullptr) << "mkdtemp failed: " << std::strerror(errno);
        m_scratch = std::filesystem::path(result);

        auto watcher = KqueueFileWatcher::Create();
        ASSERT_TRUE(watcher.ok()) << watcher.status();
        m_watcher = *watcher;
    }

    void TearDown() override
    {
        // Tear down the watcher first so any outstanding handles unwind
        // before we yank the directory out from under it.
        m_watcher.reset();

        if (!m_scratch.empty())
        {
            std::error_code ec;
            std::filesystem::remove_all(m_scratch, ec);
        }
    }

    std::filesystem::path Scratch(const std::filesystem::path& name) const
    {
        return m_scratch / name;
    }

    void WriteFile(const std::filesystem::path& path, std::string_view contents)
    {
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(f.is_open()) << "failed to open " << path;
        f.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        ASSERT_TRUE(f.good());
    }

    void AppendFile(const std::filesystem::path& path, std::string_view contents)
    {
        std::ofstream f(path, std::ios::binary | std::ios::app);
        ASSERT_TRUE(f.is_open()) << "failed to open " << path;
        f.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        ASSERT_TRUE(f.good());
    }

    // Mimics how editors (vim, VS Code, atomic-write libraries) save files:
    // write a sibling tmp file, then rename it over the target.
    void AtomicReplace(const std::filesystem::path& path, std::string_view contents)
    {
        auto tmp = path;
        tmp += ".tmp";
        WriteFile(tmp, contents);

        std::error_code ec;
        std::filesystem::rename(tmp, path, ec);
        ASSERT_FALSE(ec) << "rename failed: " << ec.message();
    }

    void DeleteFile(const std::filesystem::path& path)
    {
        std::error_code ec;
        std::filesystem::remove(path, ec);
        ASSERT_FALSE(ec) << "remove failed: " << ec.message();
    }

    void RenameFile(const std::filesystem::path& from, const std::filesystem::path& to)
    {
        std::error_code ec;
        std::filesystem::rename(from, to, ec);
        ASSERT_FALSE(ec) << "rename failed: " << ec.message();
    }

    std::shared_ptr<KqueueFileWatcher> m_watcher;
    std::filesystem::path              m_scratch;
};

TEST_F(KqueueFileWatcherTest, FixtureCreatesUniqueScratch)
{
    EXPECT_TRUE(std::filesystem::exists(m_scratch));
    EXPECT_TRUE(std::filesystem::is_directory(m_scratch));
}

TEST_F(KqueueFileWatcherTest, InPlaceWriteFiresCallback)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "initial = true\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    AppendFile(path, "added = 1\n");

    EXPECT_TRUE(tracker.WaitForCount(1));
    EXPECT_EQ(tracker.Count(), 1u);
}

TEST_F(KqueueFileWatcherTest, NoEventBeforeFileMutates)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "initial = true\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    // Nothing happens to the file -> no callback should fire.
    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));
    EXPECT_EQ(tracker.Count(), 0u);
}

TEST_F(KqueueFileWatcherTest, AtomicReplaceFiresOnceAndWatcherRecovers)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    // Editor-style save: the original inode is unlinked, and a different
    // inode now lives at `path`.
    AtomicReplace(path, "v2\n");

    ASSERT_TRUE(tracker.WaitForCount(1));
    // Coalescing should keep this to a single callback even though kqueue
    // produced multiple kevents (NOTE_DELETE on the file fd plus NOTE_WRITE
    // on the directory).
    EXPECT_FALSE(tracker.WaitForCount(2, kQuiescentWindow));
    EXPECT_EQ(tracker.Count(), 1u);

    // After atomic replace the watcher should have re-armed a fresh file
    // fd, so a subsequent in-place write must still fire.
    tracker.Reset();
    AppendFile(path, "v3\n");
    EXPECT_TRUE(tracker.WaitForCount(1));
    EXPECT_EQ(tracker.Count(), 1u);
}

TEST_F(KqueueFileWatcherTest, RenameAwayThenReplacementFiresAndWatcherRecovers)
{
    auto path = Scratch("config.toml");
    auto moved = Scratch("config.toml.old");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    // Unlike AtomicReplace(), this leaves a visible gap where the watched path
    // does not exist. The file event should disarm the stale fd, fail to reopen
    // immediately, and rely on the directory watch to recover when the path
    // comes back.
    RenameFile(path, moved);
    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));

    AtomicReplace(path, "v2\n");
    ASSERT_TRUE(tracker.WaitForCount(1));
    EXPECT_EQ(tracker.Count(), 1u);

    tracker.Reset();
    AppendFile(path, "v3\n");
    EXPECT_TRUE(tracker.WaitForCount(1));
    EXPECT_EQ(tracker.Count(), 1u);
}

TEST_F(KqueueFileWatcherTest, DeleteWithoutRecreateDoesNotFire)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    DeleteFile(path);

    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));
    EXPECT_EQ(tracker.Count(), 0u);
}

TEST_F(KqueueFileWatcherTest, DeleteThenRecreateFiresOnRecreate)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    DeleteFile(path);
    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));

    WriteFile(path, "v2\n");
    EXPECT_TRUE(tracker.WaitForCount(1));
    EXPECT_EQ(tracker.Count(), 1u);
}

TEST_F(KqueueFileWatcherTest, WatchingNonexistentFileFiresOnCreate)
{
    auto path = Scratch("future.toml");
    // Note: the file does NOT exist yet.

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    // No file means no events yet.
    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));

    WriteFile(path, "i'm here now\n");
    EXPECT_TRUE(tracker.WaitForCount(1));
    EXPECT_EQ(tracker.Count(), 1u);
}

TEST_F(KqueueFileWatcherTest, MultipleWatchesSamePathReceiveIndependentCallbacks)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker1;
    CallbackTracker tracker2;
    auto handle1 = m_watcher->WatchFile(path, tracker1.MakeCallback());
    auto handle2 = m_watcher->WatchFile(path, tracker2.MakeCallback());
    ASSERT_TRUE(handle1.ok()) << handle1.status();
    ASSERT_TRUE(handle2.ok()) << handle2.status();

    AppendFile(path, "v2\n");
    EXPECT_TRUE(tracker1.WaitForCount(1));
    EXPECT_TRUE(tracker2.WaitForCount(1));

    (*handle1)->Unwatch();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    tracker1.Reset();
    tracker2.Reset();
    AppendFile(path, "v3\n");

    EXPECT_FALSE(tracker1.WaitForCount(1, kQuiescentWindow));
    EXPECT_TRUE(tracker2.WaitForCount(1));
}

TEST_F(KqueueFileWatcherTest, UnwatchSuppressesFutureCallbacks)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    // Sanity-check that callbacks fire while watched.
    AppendFile(path, "v2\n");
    ASSERT_TRUE(tracker.WaitForCount(1));

    (*handle)->Unwatch();

    // Unwatch is asynchronous: it queues a request and wakes the watcher
    // thread. Give the io thread a beat to drain its pending-unwatch queue
    // before we mutate the file, otherwise the mutation could race ahead of
    // the drain and still produce a callback.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    tracker.Reset();
    AppendFile(path, "v3\n");
    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));
    EXPECT_EQ(tracker.Count(), 0u);
}

TEST_F(KqueueFileWatcherTest, DroppingHandleSuppressesFutureCallbacks)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    AppendFile(path, "v2\n");
    ASSERT_TRUE(tracker.WaitForCount(1));

    // Destroying the handle should call Unwatch implicitly.
    handle->reset();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    tracker.Reset();
    AppendFile(path, "v3\n");
    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));
    EXPECT_EQ(tracker.Count(), 0u);
}

TEST_F(KqueueFileWatcherTest, DestroyingWatcherWithOutstandingHandleIsSafe)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    // Drop the watcher while the handle is still alive. The handle holds
    // only a weak reference, so destroying the watcher should leave the
    // handle valid (but inert).
    m_watcher.reset();

    // No more callbacks should be possible: the watcher's thread is gone
    // and the watcher's copy of the callback was destroyed with it.
    AppendFile(path, "v2\n");
    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));

    // Destroying the now-orphaned handle must be a no-op rather than a
    // use-after-free.
    handle->reset();
}

TEST_F(KqueueFileWatcherTest, ConcurrentWatchUnwatchAndMutateIsSafe)
{
    constexpr int kThreadCount = 4;
    constexpr int kIterationsPerThread = 8;

    std::atomic<int> callbackCount{0};
    std::atomic<bool> failed{false};
    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (int threadId = 0; threadId < kThreadCount; ++threadId)
    {
        threads.emplace_back([&, threadId]
        {
            for (int i = 0; i < kIterationsPerThread; ++i)
            {
                auto path = Scratch(
                    std::filesystem::path("stress") /
                    (std::to_string(threadId) + "-" + std::to_string(i) + ".toml"));

                std::error_code ec;
                std::filesystem::create_directories(path.parent_path(), ec);
                if (ec)
                {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }

                {
                    std::ofstream f(path, std::ios::binary | std::ios::trunc);
                    if (!f.is_open())
                    {
                        failed.store(true, std::memory_order_relaxed);
                        return;
                    }
                    f << "v1\n";
                }

                auto handle = m_watcher->WatchFile(path, [&](const std::filesystem::path&)
                {
                    callbackCount.fetch_add(1, std::memory_order_relaxed);
                });
                if (!handle.ok())
                {
                    failed.store(true, std::memory_order_relaxed);
                    return;
                }

                {
                    std::ofstream f(path, std::ios::binary | std::ios::app);
                    if (!f.is_open())
                    {
                        failed.store(true, std::memory_order_relaxed);
                        return;
                    }
                    f << "v2\n";
                }

                (*handle)->Unwatch();
            }
        });
    }

    for (auto& thread : threads)
    {
        thread.join();
    }

    EXPECT_FALSE(failed.load(std::memory_order_relaxed));

    // Let the watcher drain pending events/unwatches. This is primarily a
    // race-safety test, so the exact callback count is intentionally not part
    // of the contract; events may or may not win the race against Unwatch().
    std::this_thread::sleep_for(kQuiescentWindow);
    EXPECT_GE(callbackCount.load(std::memory_order_relaxed), 0);
}

} // namespace birch::util

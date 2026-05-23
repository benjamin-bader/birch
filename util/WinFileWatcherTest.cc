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

#include "util/WinFileWatcher.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace birch::util {

namespace {

constexpr auto kCallbackTimeout = std::chrono::milliseconds(2000);
constexpr auto kQuiescentWindow = std::chrono::milliseconds(250);

std::filesystem::path MakeUniqueTempDir()
{
    std::filesystem::path base = ::testing::TempDir();
    std::random_device rd;
    std::mt19937_64 rng(rd());

    for (int attempt = 0; attempt < 32; ++attempt)
    {
        std::wstring name = L"win_file_watcher_test_";
        name += std::to_wstring(::GetCurrentProcessId());
        name += L"_";
        name += std::to_wstring(rng());

        auto candidate = base / name;
        std::error_code ec;
        if (std::filesystem::create_directory(candidate, ec) && !ec)
        {
            return candidate;
        }
    }
    return {};
}

} // namespace

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

class WinFileWatcherTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_scratch = MakeUniqueTempDir();
        ASSERT_FALSE(m_scratch.empty()) << "failed to create scratch directory";

        auto watcher = WinFileWatcher::Create();
        ASSERT_TRUE(watcher.ok()) << watcher.status();
        m_watcher = *watcher;
    }

    void TearDown() override
    {
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

    // Mimics how editors save: write a sibling tmp file, then rename it over
    // the target. On Windows, std::filesystem::rename calls MoveFileExW with
    // MOVEFILE_REPLACE_EXISTING, so this works against a pre-existing target.
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

    std::shared_ptr<WinFileWatcher> m_watcher;
    std::filesystem::path           m_scratch;
};

TEST_F(WinFileWatcherTest, FixtureCreatesUniqueScratch)
{
    EXPECT_TRUE(std::filesystem::exists(m_scratch));
    EXPECT_TRUE(std::filesystem::is_directory(m_scratch));
}

TEST_F(WinFileWatcherTest, InPlaceWriteFiresCallback)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "initial = true\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    AppendFile(path, "added = 1\n");

    EXPECT_TRUE(tracker.WaitForCount(1));
    EXPECT_GE(tracker.Count(), 1u);
}

TEST_F(WinFileWatcherTest, NoEventBeforeFileMutates)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "initial = true\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));
    EXPECT_EQ(tracker.Count(), 0u);
}

TEST_F(WinFileWatcherTest, AtomicReplaceFiresOnceAndWatcherRecovers)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    AtomicReplace(path, "v2\n");

    ASSERT_TRUE(tracker.WaitForCount(1));
    // Coalescing should keep the temp create + rename + size + last-write
    // events down to a single callback.
    EXPECT_FALSE(tracker.WaitForCount(2, kQuiescentWindow));
    EXPECT_EQ(tracker.Count(), 1u);

    tracker.Reset();
    AppendFile(path, "v3\n");
    EXPECT_TRUE(tracker.WaitForCount(1));
    EXPECT_GE(tracker.Count(), 1u);
}

TEST_F(WinFileWatcherTest, RenameAwayThenReplacementFiresAndWatcherRecovers)
{
    auto path = Scratch("config.toml");
    auto moved = Scratch("config.toml.old");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    // Rename-away leaves the watched path missing. The dir watcher should
    // still pick up the subsequent atomic-replace and fire.
    RenameFile(path, moved);
    // A rename-away IS a change to the watched filename; we expect a callback.
    ASSERT_TRUE(tracker.WaitForCount(1));

    tracker.Reset();
    AtomicReplace(path, "v2\n");
    ASSERT_TRUE(tracker.WaitForCount(1));

    tracker.Reset();
    AppendFile(path, "v3\n");
    EXPECT_TRUE(tracker.WaitForCount(1));
}

TEST_F(WinFileWatcherTest, DeleteFiresCallback)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    DeleteFile(path);

    // On Windows we get a FILE_ACTION_REMOVED for the watched filename, which
    // our matcher treats as a change. (This differs from the kqueue impl,
    // which deliberately swallows a bare delete to wait for a recreate.)
    EXPECT_TRUE(tracker.WaitForCount(1));
}

TEST_F(WinFileWatcherTest, DeleteThenRecreateFiresOnRecreate)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    DeleteFile(path);
    ASSERT_TRUE(tracker.WaitForCount(1));

    tracker.Reset();
    WriteFile(path, "v2\n");
    EXPECT_TRUE(tracker.WaitForCount(1));
}

TEST_F(WinFileWatcherTest, WatchingNonexistentFileFiresOnCreate)
{
    auto path = Scratch("future.toml");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));

    WriteFile(path, "i'm here now\n");
    EXPECT_TRUE(tracker.WaitForCount(1));
}

TEST_F(WinFileWatcherTest, MultipleWatchesSamePathReceiveIndependentCallbacks)
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

TEST_F(WinFileWatcherTest, UnwatchSuppressesFutureCallbacks)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    AppendFile(path, "v2\n");
    ASSERT_TRUE(tracker.WaitForCount(1));

    (*handle)->Unwatch();

    // Unwatch posts to the worker via the IOCP — give the worker a moment to
    // drain its pending-unwatch queue before we mutate the file again.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    tracker.Reset();
    AppendFile(path, "v3\n");
    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));
    EXPECT_EQ(tracker.Count(), 0u);
}

TEST_F(WinFileWatcherTest, DroppingHandleSuppressesFutureCallbacks)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    AppendFile(path, "v2\n");
    ASSERT_TRUE(tracker.WaitForCount(1));

    handle->reset();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    tracker.Reset();
    AppendFile(path, "v3\n");
    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));
    EXPECT_EQ(tracker.Count(), 0u);
}

TEST_F(WinFileWatcherTest, DestroyingWatcherWithOutstandingHandleIsSafe)
{
    auto path = Scratch("config.toml");
    WriteFile(path, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(path, tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    m_watcher.reset();

    AppendFile(path, "v2\n");
    EXPECT_FALSE(tracker.WaitForCount(1, kQuiescentWindow));

    handle->reset();
}

TEST_F(WinFileWatcherTest, ConcurrentWatchUnwatchAndMutateIsSafe)
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

    std::this_thread::sleep_for(kQuiescentWindow);
    EXPECT_GE(callbackCount.load(std::memory_order_relaxed), 0);
}

TEST_F(WinFileWatcherTest, CaseInsensitiveFilenameMatch)
{
    // Watch with one case; mutate with another. RDCW will hand us back the
    // on-disk filename casing, and our matcher must still find the watch.
    auto onDisk = Scratch("MixedCase.TOML");
    WriteFile(onDisk, "v1\n");

    CallbackTracker tracker;
    auto handle = m_watcher->WatchFile(Scratch("mixedcase.toml"), tracker.MakeCallback());
    ASSERT_TRUE(handle.ok()) << handle.status();

    AppendFile(onDisk, "v2\n");
    EXPECT_TRUE(tracker.WaitForCount(1));
}

} // namespace birch::util

#include "thread_pool.h"

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <numeric>
#include <string>


TEST(Future, ReturnsInt) {
    ThreadPool pool(2);
    auto f = pool.Submit([] { return 42; });
    EXPECT_EQ(f.get(), 42);
}

TEST(Future, ReturnsString) {
    ThreadPool pool(1);
    auto f = pool.Submit([] { return std::string("hello"); });
    EXPECT_EQ(f.get(), "hello");
}

TEST(Future, PropagatesException) {
    ThreadPool pool(2);
    auto f = pool.Submit([]() -> int {
        throw std::runtime_error("oops");
        return 0;
    });
    EXPECT_THROW(f.get(), std::runtime_error);
}

TEST(Future, ExceptionMessage) {
    ThreadPool pool(1);
    auto f = pool.Submit([]() -> int {
        throw std::runtime_error("exact message");
        return 0;
    });
    try {
        f.get();
        FAIL() << "Ожидалось исключение";
    } catch (const std::runtime_error& e) {
        EXPECT_STREQ(e.what(), "exact message");
    }
}

TEST(Future, IsReadyAfterGet) {
    ThreadPool pool(2);
    auto f = pool.Submit([] { return 1; });
    f.get();
    EXPECT_TRUE(f.is_ready());
}

TEST(ThreadPool, SingleTask) {
    ThreadPool pool(1);
    auto f = pool.Submit([] { return 7; });
    EXPECT_EQ(f.get(), 7);
}

TEST(ThreadPool, MultipleTasks) {
    ThreadPool pool(4);
    std::vector<Future<int>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.Submit([i] { return i * i; }));
    }
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(futures[i].get(), i * i);
    }
}

TEST(ThreadPool, TasksRunInParallel) {
    ThreadPool pool(4);
    std::atomic<int> running{0};
    std::atomic<int> maxRunning{0};

    auto task = [&] {
        int cur = ++running;
        int expected = maxRunning.load();
        while (cur > expected &&
               !maxRunning.compare_exchange_weak(expected, cur)) {}
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        --running;
        return 0;
    };

    std::vector<Future<int>> futures;
    for (int i = 0; i < 4; ++i) {
        futures.push_back(pool.Submit(task));
    }
    for (auto& f : futures) f.get();

    // При 4 потоках хотя бы 2 задачи должны были работать одновременно
    EXPECT_GE(maxRunning.load(), 2);
}

TEST(ThreadPool, AtomicCounter) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};

    std::vector<Future<int>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.Submit([&counter] {
            counter.fetch_add(1);
            return 0;
        }));
    }
    for (auto& f : futures) f.get();

    EXPECT_EQ(counter.load(), 100);
}

TEST(ThreadPool, SumIsCorrect) {
    ThreadPool pool(4);
    std::vector<Future<int>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.Submit([i] { return i; }));
    }
    int total = 0;
    for (auto& f : futures) total += f.get();
    EXPECT_EQ(total, 4950); 
}

TEST(ThreadPool, OneThread) {
    ThreadPool pool(1);
    std::vector<Future<int>> futures;
    for (int i = 0; i < 5; ++i) {
        futures.push_back(pool.Submit([i] { return i; }));
    }
    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(futures[i].get(), i);
    }
}

TEST(ThreadPool, ManyTasksFewThreads) {
    ThreadPool pool(2);
    std::atomic<int> done{0};

    std::vector<Future<int>> futures;
    for (int i = 0; i < 1000; ++i) {
        futures.push_back(pool.Submit([&done] {
            done.fetch_add(1);
            return 0;
        }));
    }
    for (auto& f : futures) f.get();

    EXPECT_EQ(done.load(), 1000);
}

TEST(ThreadPool, ExceptionDoesNotBreakPool) {
    ThreadPool pool(2);

    auto bad = pool.Submit([]() -> int {
        throw std::runtime_error("fail");
        return 0;
    });
    EXPECT_THROW(bad.get(), std::runtime_error);

    auto good = pool.Submit([] { return 99; });
    EXPECT_EQ(good.get(), 99);
}

TEST(ThreadPool, ModifiesSharedVariable) {
    ThreadPool pool(2);
    int shared = 0;
    auto f = pool.Submit([&shared] {
        shared = 100;
        return shared;
    });
    EXPECT_EQ(f.get(), 100);
    EXPECT_EQ(shared, 100);
}

TEST(ThreadPool, DestructorWaitsForTasks) {
    std::atomic<int> done{0};
    {
        ThreadPool pool(2);
        for (int i = 0; i < 10; ++i) {
            pool.Submit([&done] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                done.fetch_add(1);
                return 0;
            });
        }
    }
    EXPECT_EQ(done.load(), 10);
}

TEST(ThreadPool, SubmitAfterDestroyThrows) {
    auto pool = std::make_unique<ThreadPool>(2);
    pool.reset();  // уничтожаем пул
    SUCCEED();
}
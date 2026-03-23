#include "mutex.h"

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>


TEST(Mutex, LockUnlock) {
    Mutex mtx;
    mtx.lock();
    mtx.unlock();
}

TEST(Mutex, SequentialLocks) {
    Mutex mtx;
    // Несколько последовательных захватов
    for (int i = 0; i < 100; ++i) {
        mtx.lock();
        mtx.unlock();
    }
}


TEST(Mutex, CounterNoRace) {
    Mutex mtx;
    int counter = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < 1000; ++j) {
                mtx.lock();
                ++counter;
                mtx.unlock();
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(counter, 4000);
}

TEST(Mutex, CounterManyThreads) {
    Mutex mtx;
    int counter = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < 500; ++j) {
                mtx.lock();
                ++counter;
                mtx.unlock();
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(counter, 4000);
}


TEST(Mutex, MutualExclusion) {
    Mutex mtx;
    std::atomic<int> insideCount{0};  
    std::atomic<int> maxInside{0};    
    bool violated = false;

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < 200; ++j) {
                mtx.lock();

                int cur = ++insideCount;
                if (cur > 1) violated = true;  

                int expected = maxInside.load();
                while (cur > expected &&
                       !maxInside.compare_exchange_weak(expected, cur)) {}

                std::this_thread::yield();  
                --insideCount;

                mtx.unlock();
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_FALSE(violated) << "Два потока одновременно в критической секции!";
    EXPECT_EQ(maxInside.load(), 1); 
}


TEST(Mutex, AllThreadsGetAccess) {
    Mutex mtx;
    std::atomic<int> accessCount{0};
    const int numThreads = 6;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&] {
            mtx.lock();
            ++accessCount;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            mtx.unlock();
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(accessCount.load(), numThreads);
}

TEST(Mutex, ProtectsCompoundOperation) {

    Mutex mtx;
    int value = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < 250; ++j) {
                mtx.lock();
                int tmp = value; 
                tmp += 1;        
                value = tmp;     
                mtx.unlock();
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(value, 1000);
}


TEST(Mutex, HighContention) {
    Mutex mtx;
    int counter = 0;
    const int numThreads = 16;
    const int itersPerThread = 1000;

    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < itersPerThread; ++j) {
                mtx.lock();
                ++counter;
                mtx.unlock();
            }
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(counter, numThreads * itersPerThread);
}

TEST(Mutex, NoDeadlockAfterContention) {
    Mutex mtx;
    int counter = 0;

    {
        std::vector<std::thread> threads;
        for (int i = 0; i < 8; ++i) {
            threads.emplace_back([&] {
                for (int j = 0; j < 100; ++j) {
                    mtx.lock();
                    ++counter;
                    mtx.unlock();
                }
            });
        }
        for (auto& t : threads) t.join();
    }

    EXPECT_EQ(counter, 800);

    mtx.lock();
    counter = 0;
    mtx.unlock();

    EXPECT_EQ(counter, 0);
}
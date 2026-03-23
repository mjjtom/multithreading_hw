#include <gtest/gtest.h>
#include "apply_function.h"

#include <vector>
#include <numeric>
#include <cmath>
#include <atomic>

// создаёт вектор [0..n-1]
static std::vector<int> makeIota(int n) {
    std::vector<int> v(n);
    std::iota(v.begin(), v.end(), 0);
    return v;
}

// пустой вектор: ничего не делает и не падает
TEST(ApplyFunction, EmptyVectorDoesNothing) {
    std::vector<int> v;
    EXPECT_NO_THROW(ApplyFunction<int>(v, [](int& x){ x *= 2; }, 1));
    EXPECT_TRUE(v.empty());
}

// один элемент: корректно обрабатывается
TEST(ApplyFunction, SingleElement_SingleThread) {
    std::vector<int> v = {7};
    ApplyFunction<int>(v, [](int& x){ x *= 3; }, 1);
    EXPECT_EQ(v[0], 21);
}

// один поток: обычный последовательный случай
TEST(ApplyFunction, DoubleAllElements_SingleThread) {
    auto v = makeIota(10);
    ApplyFunction<int>(v, [](int& x){ x *= 2; }, 1);
    for (int i = 0; i < 10; ++i) EXPECT_EQ(v[i], i * 2);
}

// несколько потоков: результат должен совпадать
TEST(ApplyFunction, DoubleAllElements_FourThreads) {
    auto v = makeIota(100);
    ApplyFunction<int>(v, [](int& x){ x *= 2; }, 4);
    for (int i = 0; i < 100; ++i) EXPECT_EQ(v[i], i * 2);
}

// много потоков: проверка деления на части
TEST(ApplyFunction, DoubleAllElements_TwentyThreads) {
    auto v = makeIota(100);
    ApplyFunction<int>(v, [](int& x){ x *= 2; }, 20);
    for (int i = 0; i < 100; ++i) EXPECT_EQ(v[i], i * 2);
}

// потоков больше чем элементов: корректно ограничиваются
TEST(ApplyFunction, ThreadCountExceedsElements_Clamped) {
    std::vector<int> v = {1, 2, 3};
    ApplyFunction<int>(v, [](int& x){ x += 10; }, 1000);
    EXPECT_EQ(v[0], 11);
    EXPECT_EQ(v[1], 12);
    EXPECT_EQ(v[2], 13);
}

// 0 потоков: считаем как 1
TEST(ApplyFunction, ThreadCountZeroTreatedAsOne) {
    auto v = makeIota(5);
    ApplyFunction<int>(v, [](int& x){ x += 1; }, 0);
    for (int i = 0; i < 5; ++i) EXPECT_EQ(v[i], i + 1);
}

// отрицательное число потоков: тоже 1
TEST(ApplyFunction, NegativeThreadCountTreatedAsOne) {
    auto v = makeIota(5);
    ApplyFunction<int>(v, [](int& x){ x += 1; }, -5);
    for (int i = 0; i < 5; ++i) EXPECT_EQ(v[i], i + 1);
}

// один элемент и много потоков: не ломается
TEST(ApplyFunction, SingleElement_ManyThreads) {
    std::vector<int> v = {42};
    ApplyFunction<int>(v, [](int& x){ x = 0; }, 8);
    EXPECT_EQ(v[0], 0);
}

// проверка для double
TEST(ApplyFunction, WorksWithDouble) {
    std::vector<double> v = {1.0, 4.0, 9.0, 16.0};
    ApplyFunction<double>(v, [](double& x){ x = std::sqrt(x); }, 2);
    EXPECT_DOUBLE_EQ(v[0], 1.0);
    EXPECT_DOUBLE_EQ(v[1], 2.0);
    EXPECT_DOUBLE_EQ(v[2], 3.0);
    EXPECT_DOUBLE_EQ(v[3], 4.0);
}

// проверка для string
TEST(ApplyFunction, WorksWithString) {
    std::vector<std::string> v = {"hello", "world"};
    ApplyFunction<std::string>(v, [](std::string& s){ s += "!"; }, 2);
    EXPECT_EQ(v[0], "hello!");
    EXPECT_EQ(v[1], "world!");
}

// каждый элемент должен быть обработан ровно один раз
TEST(ApplyFunction, EachElementTouchedExactlyOnce_MultiThread) {
    const int N = 1000;

    std::vector<int> v(N, 0);
    int* base = v.data();

    std::vector<std::atomic<int>> hits(N);
    for (auto& h : hits) h.store(0);

    ApplyFunction<int>(v, [base, &hits](int& x){
        int idx = static_cast<int>(&x - base);
        hits[idx].fetch_add(1, std::memory_order_relaxed);
        x = idx;
    }, 8);

    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(hits[i].load(), 1);
        EXPECT_EQ(v[i], i);
    }
}

// по умолчанию используется один поток
TEST(ApplyFunction, DefaultThreadCount) {
    auto v = makeIota(5);
    ApplyFunction<int>(v, [](int& x){ x *= 10; });
    for (int i = 0; i < 5; ++i) EXPECT_EQ(v[i], i * 10);
}

// большой массив: проверка стабильности
TEST(ApplyFunction, LargeVector_MultiThread) {
    const int N = 1'000'000;
    std::vector<long long> v(N, 1LL);
    ApplyFunction<long long>(v, [](long long& x){ x *= 2; }, 8);
    for (int i = 0; i < N; ++i) EXPECT_EQ(v[i], 2LL);
}
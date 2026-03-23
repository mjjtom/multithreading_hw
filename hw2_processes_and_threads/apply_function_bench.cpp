#include <benchmark/benchmark.h>
#include "apply_function.h"

#include <vector>
#include <cmath>
#include <numeric>

// маленький вектор и дешёвая операция
// здесь один поток обычно быстрее, потому что создание потоков дороже самой работы
static void BM_Small_Cheap_1Thread(benchmark::State& state) {
    const int N = 64;
    std::vector<int> v(N);
    std::iota(v.begin(), v.end(), 0);
    auto orig = v;

    for (auto _ : state) {
        v = orig;
        ApplyFunction<int>(v, [](int& x){ x *= 2; }, 1);
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Small_Cheap_1Thread);

static void BM_Small_Cheap_4Threads(benchmark::State& state) {
    const int N = 64;
    std::vector<int> v(N);
    std::iota(v.begin(), v.end(), 0);
    auto orig = v;

    for (auto _ : state) {
        v = orig;
        ApplyFunction<int>(v, [](int& x){ x *= 2; }, 4);
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Small_Cheap_4Threads);

static void BM_Small_Cheap_8Threads(benchmark::State& state) {
    const int N = 64;
    std::vector<int> v(N);
    std::iota(v.begin(), v.end(), 0);
    auto orig = v;

    for (auto _ : state) {
        v = orig;
        ApplyFunction<int>(v, [](int& x){ x *= 2; }, 8);
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Small_Cheap_8Threads);

// большой вектор и тяжёлая операция
// здесь несколько потоков обычно быстрее, потому что вычислений много

// тяжёлая функция для одного элемента
static void heavyTransform(double& x) {
    x = std::sqrt(std::abs(x)) + std::pow(x, 1.0 / 3.0)
        + std::sin(x) * std::cos(x);
}

static void BM_Large_Heavy_1Thread(benchmark::State& state) {
    const int N = 4'000'000;
    std::vector<double> v(N);
    for (int i = 0; i < N; ++i) v[i] = static_cast<double>(i + 1);
    auto orig = v;

    for (auto _ : state) {
        v = orig;
        ApplyFunction<double>(v, heavyTransform, 1);
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Large_Heavy_1Thread);

static void BM_Large_Heavy_4Threads(benchmark::State& state) {
    const int N = 4'000'000;
    std::vector<double> v(N);
    for (int i = 0; i < N; ++i) v[i] = static_cast<double>(i + 1);
    auto orig = v;

    for (auto _ : state) {
        v = orig;
        ApplyFunction<double>(v, heavyTransform, 4);
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Large_Heavy_4Threads);

static void BM_Large_Heavy_8Threads(benchmark::State& state) {
    const int N = 4'000'000;
    std::vector<double> v(N);
    for (int i = 0; i < N; ++i) v[i] = static_cast<double>(i + 1);
    auto orig = v;

    for (auto _ : state) {
        v = orig;
        ApplyFunction<double>(v, heavyTransform, 8);
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Large_Heavy_8Threads);

// средний размер данных и средняя нагрузка
// здесь результат зависит от процессора и системы
static void BM_Medium_Mid_1Thread(benchmark::State& state) {
    const int N = 100'000;
    std::vector<double> v(N);
    for (int i = 0; i < N; ++i) v[i] = static_cast<double>(i + 1);
    auto orig = v;

    for (auto _ : state) {
        v = orig;
        ApplyFunction<double>(v, [](double& x){ x = std::sqrt(x); }, 1);
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Medium_Mid_1Thread);

static void BM_Medium_Mid_4Threads(benchmark::State& state) {
    const int N = 100'000;
    std::vector<double> v(N);
    for (int i = 0; i < N; ++i) v[i] = static_cast<double>(i + 1);
    auto orig = v;

    for (auto _ : state) {
        v = orig;
        ApplyFunction<double>(v, [](double& x){ x = std::sqrt(x); }, 4);
        benchmark::DoNotOptimize(v.data());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_Medium_Mid_4Threads);

BENCHMARK_MAIN();
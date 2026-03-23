#pragma once

#include <vector>
#include <functional>
#include <thread>
#include <algorithm>

template <typename T>
void ApplyFunction(std::vector<T>& data,
    const std::function<void(T&)>& transform,
    const int threadCount = 1)
{
    if (data.empty())
        return;

    int n = (int)data.size();

    // нельзя взять потоков больше чем элементов, ну и меньше 1 тоже
    int threads_to_use = threadCount;
    if (threads_to_use < 1) threads_to_use = 1;
    if (threads_to_use > n) threads_to_use = n;

    // однопоточный случай, просто проходим по вектору
    if (threads_to_use == 1) {
        for (int i = 0; i < n; i++) {
            transform(data[i]);
        }
        return;
    }

    int chunk = n/threads_to_use; // сколько элементов получает каждый поток
    int leftover = n % threads_to_use; // остаток раскидываем по первым потокам

    std::vector<std::thread> threads;
    int start = 0;
    for (int i = 0; i < threads_to_use; i++) {
        int end = start + chunk;
        if (i < leftover)
            end += 1; // первые leftover потоков берут на 1 больше

        // струячим поток внутри вектора
        threads.emplace_back([&data, &transform, start, end]() {
            for (int i = start; i < end; i++) {
                transform(data[i]);
            }
        });
        start = end;
    }

    // ждём пока все завершатся
    for (auto& th : threads) {
        th.join();
    }
}
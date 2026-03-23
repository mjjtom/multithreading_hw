#pragma once

#include <atomic>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

static void FutexWait(int* addr, int expected) {
    syscall(SYS_futex, addr, FUTEX_WAIT_PRIVATE, expected, nullptr, nullptr, 0);
}

static void FutexWake(int* addr, int count) {
    syscall(SYS_futex, addr, FUTEX_WAKE_PRIVATE, count, nullptr, nullptr, 0);
}

class Mutex {
public:
    void lock() {
        int expected = 0;

        // Пытаемся захватить мьютекс, если он свободен, то есть меняем с 0 на 1
        if (state_.compare_exchange_strong(expected, 1)) {
            return;
        }

        // Кто-то держит мьютекс, помечаем что будем ждать и ждем)
        while (state_.exchange(2) != 0) {
            FutexWait(reinterpret_cast<int*>(&state_), 2);
        }
        // Когда вышли из цикла, захватили мьютекс
    }

    void unlock() {
        // Освобождаем мьютекс
        int prev = state_.exchange(0);
        if (prev == 2) {
            // Будим один поток
            FutexWake(reinterpret_cast<int*>(&state_), 1);
        }
    }

// state_: 0 = свободен, 1 = занят (никто не ждет), 2 = занят и есть ждущие потоки
private:
    std::atomic<int> state_{0};
};
#pragma once

#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>


template <typename T>
struct SharedState {
    std::mutex              mtx;
    std::condition_variable cv;
    std::optional<T>        value;        // результат если успех
    std::exception_ptr      exception;    // исключение если ошибка
    bool                    ready = false;
};

template <typename T>
class Future {
public:
    explicit Future(std::shared_ptr<SharedState<T>> state)
        : state_(std::move(state)) {}

    // Блокируется пока результат не готов
    // Возвращает значение или перебрасывает исключение
    T get() {
        std::unique_lock lock(state_->mtx);
        state_->cv.wait(lock, [this] { return state_->ready; });

        if (state_->exception) {
            std::rethrow_exception(state_->exception);
        }
        return std::move(*state_->value);
    }

    // Проверить готов ли результат без блокировки
    bool is_ready() const {
        std::lock_guard lock(state_->mtx);
        return state_->ready;
    }

private:
    std::shared_ptr<SharedState<T>> state_;
};

class ThreadPool {
public:
    explicit ThreadPool(int threadCount) {
        threads_.reserve(threadCount);
        for (int i = 0; i < threadCount; ++i) {
            threads_.emplace_back([this] { workerLoop(); });
        }
    }

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        // Сигнализируем всем потокам что нужно завершиться
        {
            std::lock_guard lock(mtx_);
            stopped_ = true;
        }
        cv_.notify_all();  // будим всех спящих

        for (auto& t : threads_) {
            t.join();
        }
    }

    template <typename F>
    auto Submit(F&& func) -> Future<std::invoke_result_t<F>> {
        using T = std::invoke_result_t<F>;

        // Создаём общее состояние
        auto state = std::make_shared<SharedState<T>>();

        // Оборачиваем задачу: выполнить func и записать результат в state
        auto task = [state, func = std::forward<F>(func)]() mutable {
            try {
                state->value = func();  // выполняем, сохраняем результат
            } catch (...) {
                state->exception = std::current_exception();  // ловим любое исключение
            }
            // Уведомляем всех кто ждёт на get()
            {
                std::lock_guard lock(state->mtx);
                state->ready = true;
            }
            state->cv.notify_all();
        };

        // Кладём задачу в очередь
        {
            std::lock_guard lock(mtx_);
            if (stopped_) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            tasks_.push(std::move(task));
        }
        cv_.notify_one();  // будим одного спящего потока

        return Future<T>(std::move(state));
    }

private:
    void workerLoop() {
        while (true) {
            std::function<void()> task;

            {
                std::unique_lock lock(mtx_);
                // Спим пока нет задач или не попросили остановиться
                cv_.wait(lock, [this] {
                    return !tasks_.empty() || stopped_;
                });

                if (stopped_ && tasks_.empty()) {
                    return;  // завершаем поток
                }

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            task();  // выполняем задачу за пределами блокировки
        }
    }

    std::mutex                         mtx_;
    std::condition_variable            cv_;
    bool                               stopped_ = false;
    std::queue<std::function<void()>>  tasks_;
    std::vector<std::thread>           threads_;
};

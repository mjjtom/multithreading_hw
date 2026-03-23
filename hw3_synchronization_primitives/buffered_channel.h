#pragma once

#include <optional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

template <class T>
class BufferedChannel {
public:
    // Задаем размер канала и состояние открыт
    explicit BufferedChannel(int size) : capacity_(size), closed_(false) {
    }

    // Поток вызывает send, он вызывает mutex_ чтобы никто в этот момент не менял очередь
    void Send(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);

        // Если очередь полная, то сладко спим)
        cv_send_.wait(lock, [this]() {
            return (int)queue_.size() < capacity_ || closed_;
        });

        // Если канал уже закрыт, отправка запрещена
        if (closed_) {
            throw std::runtime_error("Channel is closed");
        }

        // Кладём элемент в очередь
        queue_.push(value);

        // Будим один поток, который возможно ждёт данные в Recv
        cv_recv_.notify_one();
    }

    std::optional<T> Recv() {
        
        // Тоже захватываем mutex_
        std::unique_lock<std::mutex> lock(mutex_);

        // Если очередь пуста, поток засыпает на cv_recv_
        cv_recv_.wait(lock, [this]() {
            return !queue_.empty() || closed_;
        });

        // Если очередь пуста и канал закрыт, возвращаем жопу
        if (queue_.empty()) {
            return std::nullopt;
        }

        // Забираем элемент из очереди
        T val = queue_.front();
        queue_.pop();

        // Будим один поток, ожидающий send
        cv_send_.notify_one();

        return val;
    }

    // Данных больше не будет, аревуар/ауфвидерзеен
    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);

        // Помечаем канал как закрытый
        closed_ = true;

        // Будим все потоки, чтобы они не зависли
        cv_send_.notify_all();
        cv_recv_.notify_all();
    }

// Эти поля нельзя трогать напрямую извне класса
private:
    int capacity_;  // максимальный размер буфера
    bool closed_;   // флаг закрытия канала
    std::queue<T> queue_;   // очередь
    std::mutex mutex_;  // защита всех данных
    std::condition_variable cv_send_; // ожидание места для send
    std::condition_variable cv_recv_; // ожидание данных для recv
};
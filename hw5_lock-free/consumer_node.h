#pragma once
#include "ring_buffer.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <functional>
#include <thread>
#include <chrono>
#include <string>
#include <stdexcept>
#include <algorithm>

class ConsumerNode {
public:
    ConsumerNode(const std::string& name, uint32_t capacity)
        : total_size_(sizeof(ShmHeader) + capacity)
    {
        while (true) {
            fd_ = shm_open(name.c_str(), O_RDWR, 0666);
            if (fd_ >= 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        ptr_ = mmap(nullptr, total_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (ptr_ == MAP_FAILED)
            throw std::runtime_error("mmap failed");

        queue_ = new MpscRingBuffer(ptr_);
    }

    using Handler = std::function<void(MessageType, const std::vector<uint8_t>&)>;

    // читает сообщения и вызывает handler для каждого подходящего
    void Run(const std::vector<MessageType>& filter, Handler handler) {
        MessageType type;
        std::vector<uint8_t> data;

        while (running_) {
            if (!queue_->TryPop(type, data)) {
                std::this_thread::yield();
                continue;
            }

            if (!filter.empty()) {
                bool wanted = std::find(filter.begin(), filter.end(), type) != filter.end();
                if (!wanted) continue;
            }

            handler(type, data);
        }
    }

    void Stop() { running_ = false; }

private:
    size_t            total_size_;
    int               fd_    = -1;
    void*             ptr_   = nullptr;
    MpscRingBuffer*   queue_ = nullptr;
    std::atomic<bool> running_{true};
};
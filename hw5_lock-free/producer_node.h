#pragma once
#include "ring_buffer.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <thread>
#include <string>
#include <stdexcept>

class ProducerNode {
public:
    ProducerNode(const std::string& name, uint32_t capacity)
        : name_(name)
        , total_size_(sizeof(ShmHeader) + capacity)
    {
        // на случай если shared memory с таким именем осталась с прошлого запуска
        shm_unlink(name_.c_str());

        fd_ = shm_open(name_.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd_ < 0)
            throw std::runtime_error("shm_open failed");

        if (ftruncate(fd_, (off_t)total_size_) < 0)
            throw std::runtime_error("ftruncate failed");

        ptr_ = mmap(nullptr, total_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (ptr_ == MAP_FAILED)
            throw std::runtime_error("mmap failed");

        MpscRingBuffer::Init(ptr_, capacity);
        queue_ = new MpscRingBuffer(ptr_);
    }

    ~ProducerNode() {
        delete queue_;
        munmap(ptr_, total_size_);
        close(fd_);
        shm_unlink(name_.c_str());
    }

    void Send(MessageType type, const void* data, uint32_t size) {
        while (!queue_->TryPush(type, data, size))
            std::this_thread::yield();
    }

private:
    std::string     name_;
    size_t          total_size_;
    int             fd_    = -1;
    void*           ptr_   = nullptr;
    MpscRingBuffer* queue_ = nullptr;
};
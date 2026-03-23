#pragma once
#include "protocol.h"
#include <atomic>
#include <cstring>
#include <vector>
#include <stdexcept>

struct ShmHeader {
    uint32_t version;
    uint32_t capacity;
    std::atomic<uint32_t> head;
    std::atomic<uint32_t> tail;
};

// Перед каждым сообщением в буфере лежит этот заголовок
// ready=0 пока producer ещё пишет данные, ready=1 когда можно читать
struct Slot {
    std::atomic<uint32_t> ready;
    MessageHeader msg;
};

class MpscRingBuffer {
public:
    // вызывается один раз перед началом работы
    static void Init(void* mem, uint32_t capacity) {
        auto* hdr = new (mem) ShmHeader{};
        hdr->version  = PROTOCOL_VERSION;
        hdr->capacity = capacity;
        hdr->head.store(0);
        hdr->tail.store(0);
        memset((char*)mem + sizeof(ShmHeader), 0, capacity);
    }

    explicit MpscRingBuffer(void* mem)
        : hdr_((ShmHeader*)mem)
        , buf_((char*)mem + sizeof(ShmHeader))
        , cap_(hdr_->capacity)
        , head_(hdr_->head)
        , tail_(hdr_->tail)
    {
        if (hdr_->version != PROTOCOL_VERSION)
            throw std::runtime_error("Protocol version mismatch");
    }

    bool TryPush(MessageType type, const void* data, uint32_t size) {
        uint32_t needed = sizeof(Slot) + size;

        while (true) {
            uint32_t tail = tail_.load(std::memory_order_relaxed);
            uint32_t head = head_.load(std::memory_order_acquire);

            uint32_t used = (tail - head + cap_) % cap_;
            if (used + needed >= cap_)
                return false;

            uint32_t new_tail = (tail + needed) % cap_;

            if (!tail_.compare_exchange_weak(tail, new_tail,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed))
                continue;

            // место зарезервировано, записываем данные
            Slot* slot = slotAt(tail);
            slot->ready.store(0, std::memory_order_relaxed);
            slot->msg = {type, size};
            if (size > 0)
                memcpy((char*)slot + sizeof(Slot), data, size);
            // только теперь помечаем слот как готовый к чтению
            slot->ready.store(1, std::memory_order_release);
            return true;
        }
    }

    // возвращает false если очередь пуста или слот ещё пишется
    bool TryPop(MessageType& out_type, std::vector<uint8_t>& out_data) {
        uint32_t head = head_.load(std::memory_order_relaxed);
        uint32_t tail = tail_.load(std::memory_order_acquire);

        if (head == tail)
            return false;

        Slot* slot = slotAt(head);

        // producer мог зарезервировать слот но ещё не дописать данные
        if (slot->ready.load(std::memory_order_acquire) == 0)
            return false;

        out_type = slot->msg.type;
        uint32_t size = slot->msg.size;
        out_data.resize(size);
        if (size > 0)
            memcpy(out_data.data(), (char*)slot + sizeof(Slot), size);

        slot->ready.store(0, std::memory_order_relaxed);
        head_.store((head + sizeof(Slot) + size) % cap_, std::memory_order_release);
        return true;
    }

private:
    Slot* slotAt(uint32_t offset) {
        return (Slot*)(buf_ + offset % cap_);
    }

    ShmHeader*             hdr_;
    char*                  buf_;
    uint32_t               cap_;
    std::atomic<uint32_t>& head_;
    std::atomic<uint32_t>& tail_;
};
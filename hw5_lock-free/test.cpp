#include <gtest/gtest.h>
#include "producer_node.h"
#include "consumer_node.h"

#include <thread>
#include <atomic>
#include <vector>
#include <algorithm>
#include <cstring>

static const char* kShm = "/test_mpsc_queue";
static const uint32_t kCap = 65536;

static void* makeBuf(uint32_t cap) {
    void* mem = malloc(sizeof(ShmHeader) + cap);
    MpscRingBuffer::Init(mem, cap);
    return mem;
}


TEST(RingBuffer, PopFromEmptyReturnsFalse) {
    void* mem = makeBuf(1024);
    MpscRingBuffer rb(mem);

    MessageType type;
    std::vector<uint8_t> data;
    EXPECT_FALSE(rb.TryPop(type, data));
    free(mem);
}

TEST(RingBuffer, PushPopNumber) {
    void* mem = makeBuf(1024);
    MpscRingBuffer rb(mem);

    int val = 42;
    ASSERT_TRUE(rb.TryPush(MessageType::Number, &val, sizeof(val)));

    MessageType type;
    std::vector<uint8_t> data;
    ASSERT_TRUE(rb.TryPop(type, data));

    EXPECT_EQ(type, MessageType::Number);
    int got = 0;
    memcpy(&got, data.data(), sizeof(got));
    EXPECT_EQ(got, 42);

    free(mem);
}

TEST(RingBuffer, PushPopText) {
    void* mem = makeBuf(1024);
    MpscRingBuffer rb(mem);

    std::string msg = "hello";
    ASSERT_TRUE(rb.TryPush(MessageType::Text, msg.data(), (uint32_t)msg.size()));

    MessageType type;
    std::vector<uint8_t> data;
    ASSERT_TRUE(rb.TryPop(type, data));
    EXPECT_EQ(type, MessageType::Text);
    EXPECT_EQ(std::string(data.begin(), data.end()), "hello");

    free(mem);
}

TEST(RingBuffer, PushPingNoData) {
    void* mem = makeBuf(1024);
    MpscRingBuffer rb(mem);

    ASSERT_TRUE(rb.TryPush(MessageType::Ping, nullptr, 0));

    MessageType type;
    std::vector<uint8_t> data;
    ASSERT_TRUE(rb.TryPop(type, data));
    EXPECT_EQ(type, MessageType::Ping);
    EXPECT_TRUE(data.empty());

    free(mem);
}

TEST(RingBuffer, FifoOrder) {
    void* mem = makeBuf(4096);
    MpscRingBuffer rb(mem);

    for (int i = 0; i < 10; i++)
        ASSERT_TRUE(rb.TryPush(MessageType::Number, &i, sizeof(i)));

    MessageType type;
    std::vector<uint8_t> data;
    for (int i = 0; i < 10; i++) {
        ASSERT_TRUE(rb.TryPop(type, data));
        int got = 0;
        memcpy(&got, data.data(), sizeof(got));
        EXPECT_EQ(got, i);
    }

    free(mem);
}

TEST(RingBuffer, FullBufferReturnsFalse) {
    uint32_t slot = sizeof(Slot) + sizeof(int);
    void* mem = makeBuf(slot * 3);
    MpscRingBuffer rb(mem);

    int val = 1;
    EXPECT_TRUE(rb.TryPush(MessageType::Number, &val, sizeof(val)));
    EXPECT_TRUE(rb.TryPush(MessageType::Number, &val, sizeof(val)));
    EXPECT_FALSE(rb.TryPush(MessageType::Number, &val, sizeof(val)));

    free(mem);
}

TEST(RingBuffer, WrapAround) {
    uint32_t slot = sizeof(Slot) + sizeof(int);
    void* mem = makeBuf(slot * 4);
    MpscRingBuffer rb(mem);

    MessageType type;
    std::vector<uint8_t> data;

    for (int i = 0; i < 20; i++) {
        ASSERT_TRUE(rb.TryPush(MessageType::Number, &i, sizeof(i)));
        ASSERT_TRUE(rb.TryPop(type, data));
        int got = 0;
        memcpy(&got, data.data(), sizeof(got));
        EXPECT_EQ(got, i);
    }

    free(mem);
}

TEST(RingBuffer, ProtocolVersionMismatch) {
    void* mem = makeBuf(1024);
    ((ShmHeader*)mem)->version = 999;
    EXPECT_THROW(MpscRingBuffer rb(mem), std::runtime_error);
    free(mem);
}


TEST(RingBuffer, MultipleProducers) {
    const int N = 1000;
    const int P = 4; // число producer-потоков

    void* mem = makeBuf(1024 * 1024);
    MpscRingBuffer rb(mem);

    std::vector<std::thread> producers;
    for (int t = 0; t < P; t++) {
        producers.emplace_back([&rb, t, N, P]() {
            for (int i = t; i < N; i += P) {
                while (!rb.TryPush(MessageType::Number, &i, sizeof(i)))
                    std::this_thread::yield();
            }
        });
    }

    std::vector<int> received;
    MessageType type;
    std::vector<uint8_t> data;
    while ((int)received.size() < N) {
        if (rb.TryPop(type, data)) {
            int v = 0;
            memcpy(&v, data.data(), sizeof(v));
            received.push_back(v);
        } else {
            std::this_thread::yield();
        }
    }

    for (auto& p : producers) p.join();

    std::sort(received.begin(), received.end());
    for (int i = 0; i < N; i++)
        EXPECT_EQ(received[i], i);

    free(mem);
}

TEST(RingBuffer, NoLostNoDuplicated) {
    const int N = 600;
    void* mem = makeBuf(1024 * 1024);
    MpscRingBuffer rb(mem);

    std::thread p1([&rb, N]() {
        for (int i = 0; i < N; i += 2)
            while (!rb.TryPush(MessageType::Number, &i, sizeof(i)))
                std::this_thread::yield();
    });
    std::thread p2([&rb, N]() {
        for (int i = 1; i < N; i += 2)
            while (!rb.TryPush(MessageType::Number, &i, sizeof(i)))
                std::this_thread::yield();
    });

    std::vector<int> got;
    MessageType type;
    std::vector<uint8_t> data;
    while ((int)got.size() < N) {
        if (rb.TryPop(type, data)) {
            int v = 0;
            memcpy(&v, data.data(), sizeof(v));
            got.push_back(v);
        } else {
            std::this_thread::yield();
        }
    }

    p1.join();
    p2.join();

    std::sort(got.begin(), got.end());
    for (int i = 0; i < N; i++)
        EXPECT_EQ(got[i], i);

    free(mem);
}


TEST(Nodes, TextMessage) {
    ProducerNode prod(kShm, kCap);

    std::string sent = "hello from test";
    prod.Send(MessageType::Text, sent.data(), (uint32_t)sent.size());
    prod.Send(MessageType::Ping, nullptr, 0);

    ConsumerNode cons(kShm, kCap);
    std::string received;

    cons.Run({}, [&](MessageType type, const std::vector<uint8_t>& data) {
        if (type == MessageType::Text)
            received = std::string(data.begin(), data.end());
        else if (type == MessageType::Ping)
            cons.Stop();
    });

    EXPECT_EQ(received, sent);
}

TEST(Nodes, MultipleNumbers) {
    ProducerNode prod(kShm, kCap);

    for (int i = 0; i < 10; i++)
        prod.Send(MessageType::Number, &i, sizeof(i));
    prod.Send(MessageType::Ping, nullptr, 0);

    ConsumerNode cons(kShm, kCap);
    std::vector<int> received;

    cons.Run({}, [&](MessageType type, const std::vector<uint8_t>& data) {
        if (type == MessageType::Number) {
            int v = 0;
            memcpy(&v, data.data(), sizeof(v));
            received.push_back(v);
        } else if (type == MessageType::Ping) {
            cons.Stop();
        }
    });

    ASSERT_EQ((int)received.size(), 10);
    for (int i = 0; i < 10; i++)
        EXPECT_EQ(received[i], i);
}

TEST(Nodes, FilterDropsUnwantedTypes) {
    ProducerNode prod(kShm, kCap);

    std::string text = "should be ignored";
    prod.Send(MessageType::Text, text.data(), (uint32_t)text.size());
    int num = 42;
    prod.Send(MessageType::Number, &num, sizeof(num));
    prod.Send(MessageType::Ping, nullptr, 0);

    ConsumerNode cons(kShm, kCap);
    int text_count = 0;
    int number_value = -1;

    cons.Run({MessageType::Number, MessageType::Ping},
        [&](MessageType type, const std::vector<uint8_t>& data) {
            if (type == MessageType::Text) {
                text_count++;
            } else if (type == MessageType::Number) {
                memcpy(&number_value, data.data(), sizeof(number_value));
            } else if (type == MessageType::Ping) {
                cons.Stop();
            }
        });

    EXPECT_EQ(text_count, 0);
    EXPECT_EQ(number_value, 42);
}
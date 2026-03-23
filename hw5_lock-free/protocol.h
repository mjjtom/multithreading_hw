#pragma once
#include <cstdint>

static constexpr uint32_t PROTOCOL_VERSION = 1;

enum class MessageType : uint32_t {
    Text   = 1,
    Number = 2,
    Ping   = 3,
};

// заголовок каждого сообщения
struct MessageHeader {
    MessageType type;
    uint32_t    size; // размер полезных данных после заголовка
};
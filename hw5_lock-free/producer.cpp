#include "producer_node.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    ProducerNode producer("/my_mpsc_queue", 65536);

    for (int i = 0; i < 10; i++) {
        std::string msg = "message #" + std::to_string(i);
        producer.Send(MessageType::Text, msg.data(), (uint32_t)msg.size());

        int num = i * 10;
        producer.Send(MessageType::Number, &num, sizeof(num));

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    producer.Send(MessageType::Ping, nullptr, 0);
    return 0;
}
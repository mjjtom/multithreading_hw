#include "consumer_node.h"
#include <iostream>
#include <cstring>

int main() {
    ConsumerNode consumer("/my_mpsc_queue", 65536);

    consumer.Run({}, [&](MessageType type, const std::vector<uint8_t>& data) {
        if (type == MessageType::Text) {
            std::string text(data.begin(), data.end());
            std::cout << "[Text]   " << text << "\n";
        } else if (type == MessageType::Number) {
            int num = 0;
            memcpy(&num, data.data(), sizeof(num));
            std::cout << "[Number] " << num << "\n";
        } else if (type == MessageType::Ping) {
            std::cout << "[Ping]   done\n";
            consumer.Stop();
        }
    });

    return 0;
}
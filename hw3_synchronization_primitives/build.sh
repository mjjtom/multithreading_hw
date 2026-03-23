cmake -B build
cmake --build build
./build/buffered_channel_test
./build/buffered_channel_benchmark
rm -rf build
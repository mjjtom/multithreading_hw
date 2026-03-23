cmake -B build
cmake --build build
./build/apply_function_test
./build/apply_function_bench
rm -rf build

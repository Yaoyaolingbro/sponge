# if exist build directory, remove it
cmake -S . -B build
cd build
make -j$(nproc)

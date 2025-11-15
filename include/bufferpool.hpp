#pragma once

#include <iostream>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include <optional>


class BufferPool {
    public:
        BufferPool(size_t initial, size_t maximal); // constructor with threshold
        ~BufferPool(); // destructor to free memory
        std::optional<...> search(int sstNum, int pageOffset);
    private:
        HashMap hashMap;
        


};

class HashMap {

};
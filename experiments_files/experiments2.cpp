#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <fstream>
#include <cstdint>
#include <filesystem>
#include <string>

#include <filesystem>
#include <random>

#include "../include/mrkdb.hpp"

#define ONETWENTYEIGHT_MB_KV 2048

using namespace std::chrono;

static const uint64_t NUM_OPS = 50000;

void dbSetup() {
    std::string oldName = "expDB";
    for (int iterNum = 0; iterNum < 8; iterNum++) {
        DB db;
        db.Open(oldName);

        uint64_t start = ONETWENTYEIGHT_MB_KV * iterNum;
        uint64_t end = start + ONETWENTYEIGHT_MB_KV;

        for (uint64_t k = start; k < end; k++) {
            db.Put(k, k);
        }

        db.Close();

        std::string newName = oldName + std::to_string(iterNum);
        std::filesystem::copy(
            oldName,
            newName,
            std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing
        );

    }
}

int main() {
    dbSetup();
}

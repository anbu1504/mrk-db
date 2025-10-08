#include "../include/mrkdb.hpp"
#include <filesystem>
#include <fstream>
#include <unistd.h>

#define MEMTABLE_SST_FILENAME "0.sst"
#define METADATA_FILENAME ".metadata"

#define PAGE_SIZE 4096

int DB::Open(const std::string dbName) {
    databaseName = dbName;
    memtable = new Memtable(THRESHOLD);
    sstCount = 0;

    if (!std::filesystem::create_directory(dbName)) { // If the DB already exists
        std::ifstream metadataFile(databaseName + "/" + METADATA_FILENAME);

        if (metadataFile.is_open()) {
            metadataFile >> sstCount;
            metadataFile.close();
        }
    }

    return 0;
};

uint64_t DB::Get(uint64_t key) {
    std::optional<uint64_t> memtableValue = memtable->getValue(key);

    if (memtableValue.has_value()) {
        return memtableValue.value();
    }

    // If we get to this point, then the key doesn't exist in the memtable
    uint64_t sstValue;

    return 0;
}

int DB::Put(uint64_t key, uint64_t value) {
    bool success = memtable->insert(key, value);

    if (memtable->isThresholdReached()) {
        sstCount++;
        memtable->flushToDisk(databaseName + "/" + std::to_string(sstCount) + ".sst");
    }

    return !success; // 0 for success, 1 for failure
};

int DB::Close() {
    if (!memtable->isEmpty()) {
        sstCount++;
        memtable->flushToDisk(databaseName + "/" + std::to_string(sstCount) + ".sst");
    }

    std::ofstream metadataFile(databaseName + "/" + METADATA_FILENAME);

    if (metadataFile.is_open()) {
        metadataFile << sstCount;
        metadataFile << std::endl;
        metadataFile.close();
    }

    delete memtable;

    return 0;
};

int DB::sstBinSearch(uint64_t key, int fd) {
    return 0;
}
#include "../include/mrkdb.hpp"
#include <filesystem>
#include <fstream>
#include <unistd.h>

#define METADATA_FILENAME ".metadata"
#define PAGE_SIZE 4096
#define SST_PATH(x) (databaseName + "/" + std::to_string(x) + ".sst")

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

        for (int i = 0; i < sstCount; i++) {
            size_t entryCount;
            std::ifstream sstFile(databaseName + "/" + std::to_string(i) + ".sst");

            if (sstFile.is_open()) {
                sstFile >> entryCount;
                sstEntryCounts.push_back(entryCount);
                sstFile.close();
            } else {
                return 1; // Error: This SST should exist
            }
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
        size_t entryCount = memtable->flushToDisk(databaseName + "/" + std::to_string(sstCount) + ".sst");
        sstEntryCounts.push_back(entryCount);
        sstCount++;
    }

    return !success; // 0 for success, 1 for failure
};

int DB::Close() {
    if (!memtable->isEmpty()) {
        memtable->flushToDisk(databaseName + "/" + std::to_string(sstCount) + ".sst");
        sstCount++;
    }

    delete memtable;

    std::ofstream metadataFile(databaseName + "/" + METADATA_FILENAME);

    if (metadataFile.is_open()) {
        metadataFile << sstCount;
        metadataFile << std::endl;
        metadataFile.close();
    }

    return 0;
};

int DB::sstBinSearch(uint64_t key, int fd) {
    return 0;
}
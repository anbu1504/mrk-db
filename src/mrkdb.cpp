#include "../include/mrkdb.hpp"
#include <filesystem>
#include <fstream>
#include <unistd.h>

#define MEMTABLE_SST_FILENAME "0.sst"
#define METADATA_FILENAME ".metadata"

#define SST_SIZE 16384
#define PAGE_SIZE 4096
#define VALUE_SIZE 8

// 8 bytes per entry, 2 entries per KV-pair
#define IDX_TO_BYTES(i) (i * 2 * 8)

int DB::Open(const std::string dbName) {
    sstCount = 0;
    databaseName = dbName;
    memtable = new Memtable(THRESHOLD);

    bool created_dir = std::filesystem::create_directory(dbName);

    if (!created_dir) {
        std::ifstream memtableSst(databaseName + "/" + MEMTABLE_SST_FILENAME);

        if (memtableSst.is_open()) {
            uint64_t key;
            uint64_t value;

            while (!memtableSst.eof()) {
                memtableSst >> key;
                memtableSst >> value;

                memtable->insert(key, value);
            }

            memtableSst.close();

            std::filesystem::remove(databaseName + "/" + MEMTABLE_SST_FILENAME);
        }

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




}

int DB::Put(uint64_t key, uint64_t value) {
    bool success = memtable->insert(key, value);

    if (memtable->isThresholdReached()) {
        sstCount++;
        memtable->flushToDisk(databaseName + "/" + std::to_string(sstCount) + ".sst");
    }

    return success;
};

int DB::Close() {
    if (!memtable->isEmpty()) {
        memtable->flushToDisk(databaseName + "/" + MEMTABLE_SST_FILENAME);
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
    int lo = 0;
    int hi = SST_SIZE / PAGE_SIZE;

    int vals_per_page = PAGE_SIZE / VALUE_SIZE;

    uint64_t curr_idx = (lo + hi) / 2;
    uint64_t curr_page[vals_per_page];

    bool found_page = false;

    while (!found_page) {
        ssize_t ret = pread(fd, &curr_page, PAGE_SIZE, curr_idx * PAGE_SIZE);

        if (curr_page[0] > key && curr_idx != 0) {
            hi = curr_idx;
            curr_idx = (lo + hi) / 2;
        } else if (curr_page[vals_per_page - 2] < key && curr_idx != (hi - 1)) {
            lo = curr_idx;
            curr_idx = (lo + hi) / 2;
        }
        // to be cont.
        
    }








}
#include "../include/mrkdb.hpp"
#include <filesystem>
#include <fstream>

#define MEMTABLE_SST_FILENAME "0.sst"
#define METADATA_FILENAME ".metadata"

int DB::Open(const std::string dbName) {
    databaseName = dbName;
    memtable = new Memtable(threshold);
    sstCount = 0;

    bool created_file = std::filesystem::create_directory(dbName);

    if (!created_file) {
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

int DB::Put(uint64_t key, uint64_t value) {
    memtable->insert(key, value);

    if (memtable->isThresholdReached()) {
        sstCount++;
        memtable->flushToDisk(databaseName + "/" + std::to_string(sstCount) + ".sst");
    }

    return 0;
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

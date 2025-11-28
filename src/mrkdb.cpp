#define _ALL_SOURCE  // Needed for O_DIRECT(?)

#include "../include/mrkdb.hpp"

#include <fcntl.h>  // Also needed for O_DIRECT(?)
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <tuple>    // Needed for std::get
#include <utility>  // Needed for std::swap

int DB::Open(const std::string dbName, bool useBTreeSearchValue = true, uint64_t bitsPerEntryValue = 12,
             uint64_t initialDirSizeValue = 4, uint64_t maxDirSizeValue = 64, uint64_t maxNumPagesValue = 4096,
             uint64_t memtableThresholdValue = 16384) {
    if (!std::filesystem::exists(dbName)) {
        std::filesystem::create_directory(dbName);

        useBTreeSearch = useBTreeSearchValue;
        bitsPerEntry = bitsPerEntryValue;
        initialDirSize = initialDirSizeValue;
        maxDirSize = maxDirSizeValue;
        maxNumPages = maxNumPagesValue;
        memtableThreshold = memtableThresholdValue;

        BufferPool* bufferPoolMake = new BufferPool(initialDirSize, maxDirSize, maxNumPages, dbName);
        LSMTree* lsmTreeMake = new LSMTree(bufPool, static_cast<uint64_t>(0));  // static cast done to get rid of C++ issue
    } else {
        std::string metaFile = dbName + "/meta.sst";
        PageBuffer pageBuf;

        BufferPool* bufPoolTemp = new BufferPool(0, 0, 0, dbName);
        bufPoolTemp->bread(metaFile, 0, pageBuf, true);
        bufPoolTemp->evictAllPages();

        delete bufPoolTemp;
        bufPoolTemp = nullptr;

        BufferPool* bufPoolMake = new BufferPool(pageBuf[2], pageBuf[3], pageBuf[4], dbName);
        LSMTree* lsmTree = new LSMTree(bufPoolMake, pageBuf);
    }
    return 0;
};

std::optional<uint64_t> DB::Get(uint64_t key) {
    uint64_t res = lsmTree->Get(key);
    if (res == TOMBSTONE) {
        return std::nullopt;
    }
    return res;
}

kvPairs DB::Scan(uint64_t key1, uint64_t key2) {
    kvPairs resScan = lsmTree->Scan(key1, key2);
    kvPairs finalRes;

    for (auto& kv : resScan) {
        uint64_t key = std::get<0>(kv);
        uint64_t value = std::get<1>(kv);

        // Skip tombstone entries
        if (value != TOMBSTONE) {
            finalRes.push_back(std::make_tuple(key, value));
        }
    }
    return finalRes;
}

int DB::Put(uint64_t key, uint64_t value) {
    lsmTree->Put(key, value);
    return 0;
};

int DB::Delete(uint64_t key) {
    lsmTree->Put(key, TOMBSTONE);
    return 0;
}

int DB::Close() {
    std::string metaFile = dbName + "/meta.sst";
    PageBuffer pageBuf;  // used for writing into meta.sst

    lsmTree->Close();
    bufPool->evictAllPages();

    uint64_t lsmTreeLevels = lsmTree->getNumLevels();
    std::vector<uint64_t> lsmOccupancyLevels = lsmTree->getOccupancyLevels();

    // NOTE the structure of meta.sst

    // Indices 0 - 5 of pageBuf are as follows:
    // 0: Whether or not this uses B tree search (stored as 0 or 1)
    // 1: Bits per entry
    // 2: Initial directory size
    // 3: Maximum directory size
    // 4: Maximum number of pages
    // 5: number of LSM Tree levels

    pageBuf[0] = static_cast<uint64_t>(useBTreeSearch);
    pageBuf[1] = bitsPerEntry;
    pageBuf[2] = initialDirSize;
    pageBuf[3] = maxDirSize;
    pageBuf[4] = maxNumPages;
    pageBuf[5] = lsmTreeLevels;

    // Then the rest of the indices of pageBuf are used for
    // determining the occupancy status of the LSM tree levels

    for (uint64_t i = 0; i < lsmTreeLevels; i++) {
        pageBuf[i + 6] =
            lsmOccupancyLevels[i];  // i + 6 for levels since pageBuf already has first 6 indices with other stuff
    }

    bufPool->bwrite(metaFile, 0, pageBuf, true);  // writing into meta.sst

    delete lsmTree;
    lsmTree = nullptr;

    delete bufPool;
    bufPool = nullptr;

    return 0;
};

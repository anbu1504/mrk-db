#include "../include/lsmtree.hpp"

LSMTree::LSMTree(BufferPool* bufPool, uint64_t numLevelsValue)
    : bufPool(bufPool),
      memtable(new Memtable(ENTRIES_PER_PAGE)),
      numLevels(numLevelsValue),
      scaleFactor(SCALE_FACTOR),
      levels(numLevelsValue, 0) {}

LSMTree::LSMTree(BufferPool* bufPool, PageBuffer metadataPageBuf)
    : bufPool(bufPool), memtable(new Memtable(ENTRIES_PER_PAGE)), scaleFactor(SCALE_FACTOR) {
    this->numLevels = metadataPageBuf[5];

    levels.clear();
    levels.resize(this->numLevels);

    for (uint64_t i = 0; i < numLevels; i++) {
        // The occupancy for level 'i' is stored at index 'i + 6' in the PageBuffer.
        uint64_t count = metadataPageBuf[i + 6];
        levels.push_back(count);
    }
}

LSMTree::~LSMTree() { delete memtable; }

void LSMTree::Put(uint64_t key, uint64_t value) {
    memtable->insert(key, value);

    if (memtable->isThresholdReached()) {
        // SSTWriter sstWriter(bufPool, );
        // check if there's an SST at level 1, if so then compact

        if (levels[1] == 0) {
            SSTWriter sw(bufPool, 1);
            std::vector<uint64_t> memtableData = memtable->inorderTraversalDel();
            sw.writeMiniSST(&memtableData);
        }

        else {
            uint64_t candidateLevel = 1;
            SSTWriter sw(bufPool, SST_TEMP_NUM(candidateLevel));
            std::vector<uint64_t> memtableData = memtable->inorderTraversalDel();
            sw.writeMiniSST(&memtableData);

            while (levels[candidateLevel] == 1) {
                uint64_t sstNumNew = candidateLevel + 1;
                if (levels[sstNumNew] == 1){
                    sstNumNew = SST_TEMP_NUM(sstNumNew);
                }
                SSTWriter sw(bufPool, sstNumNew);
                compaction(SST_TEMP_NUM(candidateLevel), candidateLevel);  // merging 2 sst's
                candidateLevel++;
            }
        }
    }
}

uint64_t LSMTree::Get(uint64_t key) {
    auto getValue = memtable->getValue(key);
    if (getValue.has_value()) {
        return getValue.value();
    }

    for (uint64_t level = 0; level < numLevels; level++) {
        if (levels[level] == 0) {
            continue;
        }

        else {
            uint64_t sstNum = level + 1;  // since memtable is level 0
            SSTView sv(bufPool, sstNum);

            if (!sv.checkForKey(key)) {
                continue;
            }
            sv.findPage(key);
            sv.fastFwd(key);

            if (sv.getCurrKey() != key) {
                continue;
            }
            return sv.getCurrValue();
        }
    }
}

kvPairs LSMTree::Scan(uint64_t key1, uint64_t key2) {
    kvPairs output;

    for (uint64_t level = 0; level < numLevels; level++) {
        if (levels[level] == 0) {
            continue;
        }

        else {
            uint64_t sstNum = level + 1;  // since memtable is level 0
            SSTView sv(bufPool, sstNum);

            for (uint64_t j = key1; j < key2 + 1; j++) {  // looping from start key to end key
                if (!sv.checkForKey(j)) {
                    continue;
                }
                sv.findPage(j);
                sv.fastFwd(j);

                if (sv.getCurrKey() != j) {
                    continue;
                }

                uint64_t currKey = sv.getCurrKey();
                uint64_t currVal = sv.getCurrValue();

                output.push_back(std::make_tuple(currKey, currVal));
            }
        }
    }
    return output;
}

void LSMTree::Close() {
    if (memtable->isEmpty()) {
        return;
    }

    if (levels[1] == 0) {
        SSTWriter sw(bufPool, 1);
        std::vector<uint64_t> memtableData = memtable->inorderTraversalDel();
        sw.writeMiniSST(&memtableData);
    }

    else {
        uint64_t candidateLevel = 1;
        SSTWriter sw(bufPool, SST_TEMP_NUM(candidateLevel));
        std::vector<uint64_t> memtableData = memtable->inorderTraversalDel();
        sw.writeMiniSST(&memtableData);
        candidateLevel++;  // start loop at next level

        while (levels[candidateLevel] == 1) {
            SSTWriter sw(bufPool, SST_TEMP_NUM(candidateLevel));
            compaction(candidateLevel, SST_TEMP_NUM(candidateLevel));  // merging 2 sst's
            candidateLevel++;
        }
        // sw.writeMiniSST(&memtableData);
        // compaction(candidateLevel, candidateLevel + 1); // merging 2 sst's
    }
}

uint64_t LSMTree::getNumLevels() { return numLevels; }

std::vector<uint64_t> LSMTree::getOccupancyLevels() { return levels; }

void LSMTree::compaction(uint64_t sstNum1, uint64_t sstNum2) {
    SSTWriter sw(bufPool, SST_TEMP_NUM(sstNum2));  // sstNum2 is assumed to be the largest number

    sw.mergeSSTs(sstNum1, sstNum2);
    bufPool->bdelete(sstNum1);
    bufPool->bdelete(sstNum2);
}

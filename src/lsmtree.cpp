#include "../include/lsmtree.hpp"

LSMTree::LSMTree(BufferPool* bufPool)
    : bufPool(bufPool),
      memtable(new Memtable(memtableThreshold)),
      levels(1, 0),
      scaleFactor(SCALE_FACTOR) {}

LSMTree::LSMTree(BufferPool* bufPool, std::vector<uint64_t> levels)
    : bufPool(bufPool), memtable(new Memtable(memtableThreshold)), levels(levels), scaleFactor(SCALE_FACTOR) {}

LSMTree::~LSMTree() { delete memtable; }

void LSMTree::flushHelper() {
    if (levels.size() > 1 && levels[1] == 1) { // at least 1 level
        uint64_t candidateLevel = 1;

        SSTWriter sw(bufPool, SST_TEMP_NUM(candidateLevel));
        std::vector<uint64_t> memtableData = memtable->inorderTraversalDel();
        sw.writeMiniSST(&memtableData);
        
        while (levels.size() > candidateLevel && levels[candidateLevel] == 1) {
            uint64_t sstNumNew = candidateLevel + 1;
            if (levels[sstNumNew] == 1) {
                sstNumNew = SST_TEMP_NUM(sstNumNew); // create compacted sst as temp{sst}
            }
            SSTWriter sw(bufPool, sstNumNew);
            if (candidateLevel + 1 == levels.size()) {
                levels.push_back(static_cast<uint64_t>(0));
            }
            sw.mergeSSTs(SST_TEMP_NUM(candidateLevel), candidateLevel);
            // compaction();  // merging 2 sst's
            levels[candidateLevel] = 0;
            if (levels[candidateLevel + 1] == 0){
                levels[candidateLevel + 1] = 1;
                break;
            }
            candidateLevel++;
        }
    }
    
    else {
        SSTWriter sw(bufPool, 1);
        std::vector<uint64_t> memtableData = memtable->inorderTraversalDel();
        sw.writeMiniSST(&memtableData);
        if (levels.size() == 1 ) {
            levels.push_back(1);
        }
        else {
            levels[1] = 1;
        }
    }

}

void LSMTree::Put(uint64_t key, uint64_t value) {
    memtable->insert(key, value);

    if (memtable->isThresholdReached()) {
        flushHelper();
    }
}

uint64_t LSMTree::Get(uint64_t key) {
    auto getValue = memtable->getValue(key);
    if (getValue.has_value()) {
        return getValue.value();
    }

    for (uint64_t level = 0; level < levels.size(); level++) {
        if (levels[level] == 0) {
            continue;
        }
        else {
            uint64_t sstNum = level;  // since memtable is level 0
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
    return TOMBSTONE; // if not found return TOMBSTONE
}

kvPairs LSMTree::Scan(uint64_t key1, uint64_t key2) {
    kvPairs memtableOutput = memtable->scanTree(key1, key2);
    kvPairs output;
    std::vector<std::optional<SSTView>> sstViewsVec(levels.size());

    for (uint64_t level = 0; level < levels.size(); level++) {
        if (levels[level] == 1) {
            sstViewsVec[level].emplace(bufPool, level);
        }
    }

    for (uint64_t j = key1; j < key2 + 1; j++) {
        uint64_t memtableHandle = 0;
        while (std::get<0>(memtableOutput[memtableHandle]) < j) {
            memtableHandle++;
        }
        if (std::get<0>(memtableOutput[memtableHandle]) == j) {
            output.push_back(memtableOutput[memtableHandle]);
            memtableHandle++;
            continue;
        }

        for (uint64_t level = 1; level < levels.size(); level++) {  // looping from start key to end key
            // need to watch out for last page of sst stuff for fastFwd
            if (levels[level] == 0) {
                continue;
            }
            if (sstViewsVec[level].has_value()) {
                SSTView sv = sstViewsVec[level].value();
                // first check if it is in the memtable 
                if (!sv.checkForKey(j)) {
                    continue;
                }
                sv.findPage(j);
                sv.fastFwd(j);

                if (sv.getCurrKey() == j) {
                    uint64_t currKey = sv.getCurrKey();
                    uint64_t currVal = sv.getCurrValue();
                    output.push_back(std::make_tuple(currKey, currVal));                
                    break;
                }
            }
            
            else {
                continue;
            }
        }
    }
    return output;
}

void LSMTree::Close() {
    if (memtable->isEmpty()) {
        return;
    }
    flushHelper();
}

std::vector<uint64_t> LSMTree::getOccupancyLevels() { return levels; }

// void LSMTree::compaction(uint64_t sstNum1, uint64_t sstNum2) {
//     SSTWriter sw(bufPool, SST_TEMP_NUM(sstNum2));  // sstNum2 is assumed to be the largest number

//     sw.mergeSSTs(sstNum1, sstNum2);
//     bufPool->bdelete(sstNum1);
//     bufPool->bdelete(sstNum2);
// }

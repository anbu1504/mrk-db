#include "../include/sstwriter.hpp"

SSTWriter::SSTWriter(BufferPool* bufPool, uint64_t sstNum) : bufPool(bufPool), sstNum(sstNum) {}

void SSTWriter::writeMiniSST(std::vector<uint64_t>* memtableData) {
    uint64_t numKeys = memtableData->size() / 2;
    uint64_t minKey = memtableData->at(0);
    uint64_t maxKey = memtableData->at(memtableData->size() - 2);

    PageBuffer pageBuf = {0};
    pageBuf[0] = numKeys;
    pageBuf[1] = minKey;
    pageBuf[2] = maxKey;

    bufPool->bwrite(sstNum, 0, pageBuf);
    std::fill(pageBuf, pageBuf + 3, 0);

    uint64_t numLeafPages = CALC_NUM_PAGES(numKeys * 2, UINT64_SIZE);
    uint64_t numFilterPages = CEIL_DIV(numKeys * bitsPerEntry, PAGE_SIZE * 8);

    uint64_t itemsInLastPage = memtableData->size() % UINT64S_PER_PAGE;
    for (int pageNum = 1; pageNum <= numLeafPages; pageNum++) {
        std::vector<uint64_t>::iterator copyBegin = memtableData->begin() + (pageNum - 1) * UINT64S_PER_PAGE;
        std::vector<uint64_t>::iterator copyEnd =  // For the last page, use memtableData->end() instead
            (pageNum < numLeafPages) ? memtableData->begin() + pageNum * UINT64S_PER_PAGE : memtableData->end();

        std::copy(copyBegin, copyEnd, pageBuf);

        if ((pageNum == numLeafPages) && itemsInLastPage) {
            std::fill(pageBuf + itemsInLastPage, pageBuf + UINT64S_PER_PAGE, 0);
        }

        bufPool->bwrite(sstNum, pageNum, pageBuf);
    }

    BloomFilter bloomFilter(bufPool, sstNum, numKeys, 1 + numLeafPages);
    bloomFilter.wipePages();
    bloomFilter.addMultiKeys(memtableData);

    BTree bTree(bufPool, sstNum, numKeys, 1 + numLeafPages + numFilterPages);
    bTree.createBTree(memtableData);
}

void SSTWriter::mergeSSTs(uint64_t sstNum1, uint64_t sstNum2) {
    uint64_t numKeys = multiwayMergeSort(sstNum1, sstNum2);
    uint64_t minKey;
    uint64_t maxKey;

    uint64_t numLeafPages = CALC_NUM_PAGES(numKeys * 2, UINT64_SIZE);
    uint64_t numFilterPages = CEIL_DIV(numKeys * bitsPerEntry, PAGE_SIZE * 8);

    BloomFilter bloomFilter(bufPool, sstNum, numKeys, 1 + numLeafPages);
    bloomFilter.wipePages();

    PageBuffer pageBuf;
    uint64_t pageNum = 0;

    for (uint64_t currKeyIdx = 0; currKeyIdx < numKeys; currKeyIdx++) {
        uint64_t pageIdx = currKeyIdx * 2;

        if (!(pageIdx % UINT64S_PER_PAGE)) {
            bufPool->bread(sstNum, pageNum, pageBuf);
            pageNum++;
            pageIdx = 0;
        }

        if (currKeyIdx == 0) {
            minKey = pageBuf[pageIdx];
        }
        if (currKeyIdx == numKeys - 1) {
            maxKey = pageBuf[pageIdx];
        }
        bloomFilter.addKey(pageBuf[pageIdx]);
    }

    std::fill(pageBuf, pageBuf + UINT64S_PER_PAGE, 0);
    pageBuf[0] = numKeys;
    pageBuf[1] = minKey;
    pageBuf[2] = maxKey;

    bufPool->bwrite(sstNum, 0, pageBuf);

    BTree bTree(bufPool, sstNum, numKeys, 1 + numLeafPages + numFilterPages);
    std::vector<uint64_t> emptyVec;
    bTree.createBTree(&emptyVec);
}

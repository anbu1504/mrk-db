#pragma once

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

    for (int pageNum = 1; pageNum <= numLeafPages; pageNum++) {
        if (pageNum < numLeafPages) {
            std::copy(memtableData->begin() + (pageNum - 1) * UINT64S_PER_PAGE,
                      memtableData->begin() + (pageNum)*UINT64S_PER_PAGE, pageBuf);
        } else {
            std::copy(memtableData->begin() + (pageNum - 1) * UINT64S_PER_PAGE, memtableData->end(), pageBuf);

            uint64_t itemsInPage = memtableData->size() % UINT64S_PER_PAGE;
            if (itemsInPage) {
                std::fill(pageBuf + itemsInPage, pageBuf + UINT64S_PER_PAGE, 0);
            }
        }

        bufPool->bwrite(sstNum, pageNum, pageBuf);
    }

    BloomFilter bloomFilter(bufPool, sstNum, numKeys, 1 + numLeafPages);
    bloomFilter.wipePages();
    bloomFilter.addMultiKeys(memtableData);

    BTree bTree(bufPool, sstNum, numKeys, 1 + numLeafPages + numFilterPages);
    bTree.createBTree(memtableData);
}

void SSTWriter::mergeSSTs(uint64_t sstNum1, uint64_t sstNum2) {}

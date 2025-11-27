#pragma once

#include "../include/sstview.hpp"
    
SSTView::SSTView(BufferPool* bufPool, uint64_t sstNum)
  : bufPool(bufPool),
    sstNum(sstNum) {

    PageBuffer pagebuf;
    bufPool->bread(sstNum, 0, pageBuf);

    numKeys = pagebuf[0];
    minKey = pagebuf[1];
    maxKey = pagebuf[2];
}

bool SSTView::checkForKey(uint64_t key) {
    if (key < minKey || key > maxKey) {
        return false;
    }

    uint64_t numLeafPages = CALC_NUM_PAGES(numKeys * 2, UINT64_SIZE);

    BloomFilter bloomFilter(bufPool, sstNum, numKeys, 1 + numLeafPages);
    
}

void SSTView::findPage(uint64_t key) {}
void SSTView::fastFwd(uint64_t key) {}
uint64_t SSTView::getCurrKey() {}

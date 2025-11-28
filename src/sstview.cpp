#pragma once

#include "../include/sstview.hpp"
    
SSTView::SSTView(BufferPool* bufPool, uint64_t sstNum)
  : bufPool(bufPool),
    sstNum(sstNum),
    pageBuf() { // Is this initialization correct?

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
    return bloomFilter.checkKey(key);
}

void SSTView::findPage(uint64_t key) {
    uint64_t itemsRead;

    uint64_t numLeafPages = CALC_NUM_PAGES(numKeys * 2, UINT64_SIZE);
    uint64_t numFilterPages = CEIL_DIV(numKeys * bitsPerEntry, PAGE_SIZE * 8);

    if (useBTreeSearch) {
        BTree bTree(bufPool, sstNum, numKeys, 1 + numLeafPages + numLeafPages);
        pageNum = bTree.findLeafPage(key);
    } else {
        // Binary search variables
        uint64_t lo = 1;
        uint64_t hi = numLeafPages;

        // uint64_t itemsRead;
        pageNum = binSearch(lo, hi, [&](uint64_t m) {
            bufPool->bread(sstNum, m, pageBuf);
            // page num and total items needed for helper function
            itemsRead = calcNumItemsInPage(m);

            return (key < pageBuf[0]) ? -1 : (key > pageBuf[itemsRead - 2]) ? 1 : 0;
        });
    }

    bufPool->bread(sstNum, pageNum, pageBuf);
    itemsRead = calcNumItemsInPage(pageNum);
    keysRead = itemsRead / 2; // itemsRead should not be 0!!!

    currKeyIdx = binSearch(0, keysRead - 1, [&](uint64_t m) {
        return (key < pageBuf[m * 2]) ? -1 : (key > pageBuf[m * 2]) ? 1 : 0;
    });
}

// number of pages = ceil(total entries / entries in a page)
// if not last page, return entries in a page (entries in a page is actually keys in a page so we have to x2)
// if last page is not full, then return total entries % entries in a page
// if last page is full (i.e. modulo returns 0), then return entries in a page
// pageNum MUST BE >= 1!!
uint64_t SSTView::calcNumItemsInPage(uint64_t pageNum) {
    uint64_t currPageNum = pageNum - 1;
    size_t numItems = 2 * numKeys;
    uint64_t numPages = CALC_NUM_PAGES(numKeys * 2, UINT64_SIZE);

    if (currPageNum == numPages - 1) {
        size_t itemsLastPage = numItems % UINT64S_PER_PAGE;
        if (itemsLastPage == 0) { // 0
            return UINT64S_PER_PAGE;
        }
        else {
            return itemsLastPage;
        }
    }
    return UINT64S_PER_PAGE;
}

// Fast-forward currKeyIdx until getCurrKey() >= key
void SSTView::fastFwd(uint64_t key) {
    uint64_t itemsRead;
    uint64_t currKey = pageBuf[currKeyIdx * 2];

    while (currKey < key) {
        currKeyIdx++;

        if (currKeyIdx >= keysRead) {
            pageNum++;
            bufPool->bread(sstNum, pageNum, pageBuf);
            itemsRead = calcNumItemsInPage(pageNum);
            keysRead = itemsRead / 2; // itemsRead should not be 0!!!

            currKeyIdx = 0;
        }

        currKey = pageBuf[currKeyIdx * 2];
    }
}

uint64_t SSTView::getCurrKey() {
    return pageBuf[currKeyIdx * 2];
}

uint64_t SSTView::getCurrValue() {
    return pageBuf[(currKeyIdx * 2) + 1];
}

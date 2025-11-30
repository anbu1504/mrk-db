#include "../include/sstview.hpp"

SSTView::SSTView(BufferPool* bufPool, uint64_t sstNum)
    : bufPool(bufPool), sstNum(sstNum), pageBuf() {  // Is this initialization correct?

    PageBuffer metadataPageBuf;
    // bufPool->printHashMap();
    bufPool->bread(sstNum, 0, metadataPageBuf);

    numKeys = metadataPageBuf[0];
    minKey = metadataPageBuf[1];
    maxKey = metadataPageBuf[2];
}

bool SSTView::checkForKey(uint64_t key) {
    PRINT("key in the checkForKey");
    PRINT(key);
    if (key < minKey || key > maxKey) {
        PRINT("key out of bounds");
        return false;
    }

    uint64_t numLeafPages = CALC_NUM_PAGES(numKeys * 2, UINT64_SIZE);

    BloomFilter bloomFilter(bufPool, sstNum, numKeys, 1 + numLeafPages);
    return true;//bloomFilter.checkKey(key);
}

void SSTView::findPage(uint64_t key) {
    uint64_t itemsRead;

    uint64_t numLeafPages = CALC_NUM_PAGES(numKeys * 2, UINT64_SIZE);
    uint64_t numFilterPages = CEIL_DIV(numKeys * bitsPerEntry, PAGE_SIZE * 8);

    if (useBTreeSearch) {
        BTree bTree(bufPool, sstNum, numKeys, 1 + numLeafPages + numFilterPages);
        pageNum = bTree.findLeafPage(key);
    } else {
        // Binary search variables
        uint64_t lo = 1;
        uint64_t hi = numLeafPages;

        // uint64_t itemsRead;
        pageNum = binSearch(lo, hi, [&](uint64_t m) {
            bufPool->bread(sstNum, m, pageBuf);
            // page num and total items needed for helper function
            itemsRead = calcNumItemsInPage(numKeys, m);

            return (key < pageBuf[0]) ? -1 : (key > pageBuf[itemsRead - 2]) ? 1 : 0;
        });
    }

    bufPool->bread(sstNum, pageNum, pageBuf);

    itemsRead = calcNumItemsInPage(numKeys, pageNum);
    keysRead = itemsRead / 2;  // itemsRead should not be 0!!!

    currKeyIdx = binSearch(0, keysRead - 1, [&](uint64_t m) {

        return (key < pageBuf[m * 2]) ? -1 : (key > pageBuf[m * 2]) ? 1 : 0;
    });
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
            itemsRead = calcNumItemsInPage(numKeys, pageNum);
            keysRead = itemsRead / 2;  // itemsRead should not be 0!!!

            currKeyIdx = 0;
        }

        currKey = pageBuf[currKeyIdx * 2];
    }
}

uint64_t SSTView::getCurrKey() { return pageBuf[currKeyIdx * 2]; }

uint64_t SSTView::getCurrValue() { return pageBuf[(currKeyIdx * 2) + 1]; }

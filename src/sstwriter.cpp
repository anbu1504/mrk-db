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
    for (uint64_t pageNum = 1; pageNum <= numLeafPages; pageNum++) {
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
        uint64_t pageIdx = (currKeyIdx * 2) % UINT64S_PER_PAGE; //chat said dis fr

        if (!(pageIdx % UINT64S_PER_PAGE)) {
            pageNum++;
            bufPool->bread(sstNum, pageNum, pageBuf);

            pageIdx = 0;
        }

        uint64_t currKey = pageBuf[pageIdx];

        if (currKeyIdx == 0) {
            minKey = currKey;
        }
        if (currKeyIdx == numKeys - 1) {
            maxKey = currKey;
        }
        bloomFilter.addKey(currKey);
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

uint64_t SSTWriter::multiwayMergeSort(uint64_t sstNum1, uint64_t sstNum2) {
    // Assuming sstNum1 is the newer sst
    PageBuffer metadataPageBufOne;
    PageBuffer metadataPageBufTwo;

    uint64_t numKeysOne;
    uint64_t numKeysTwo;

    bufPool->bread(sstNum1, 0, metadataPageBufOne, false);  // page 0 for metadata page
    numKeysOne = metadataPageBufOne[0];                     // first index holds the number of keys

    bufPool->bread(sstNum2, 0, metadataPageBufTwo, false);  // page 0 for metadata page
    numKeysTwo = metadataPageBufTwo[0];                     // first index holds the number of keys

    // Metadata for tracking when to stop loop
    uint64_t numPagesOne = CALC_NUM_PAGES(numKeysOne * 2, UINT64_SIZE);
    uint64_t numPagesTwo = CALC_NUM_PAGES(numKeysTwo * 2, UINT64_SIZE);
    uint64_t currPageNumOne = 1;
    uint64_t currPageNumTwo = 1;
    uint64_t itemsInPageOne =
        calcNumItemsInPage(numKeysOne, 0);  // used to check if we are at the end of the current buffer
    uint64_t itemsInPageTwo = calcNumItemsInPage(numKeysTwo, 0);
    uint64_t currKeyIndOne = 0;
    uint64_t currKeyIndTwo = 0;
    uint64_t totalKeys = 0;

    uint64_t currKeyIndOut = 0;
    uint64_t currPageNumOut = 1;

    uint64_t currKeyIndTemp = 0;
    uint64_t currPageNumTemp = 0;

    PageBuffer bufferOneRead;
    PageBuffer bufferTwoRead;
    PageBuffer bufferOut;
    PageBuffer bufferTemp;  // name is just 'temp'

    bool fileOneEmpty = false;
    bool fileTwoEmpty = false;

    bufPool->bread(sstNum1, currPageNumOne, bufferOneRead, false);  // page 0 for metadata page
    bufPool->bread(sstNum2, currPageNumTwo, bufferTwoRead, false);  // page 0 for metadata page

    bool done = false;

    while (!done) {
        if (!fileOneEmpty && !fileTwoEmpty) {
            if (bufferOneRead[currKeyIndOne] <= bufferTwoRead[currKeyIndTwo]) {
                bufferOut[currKeyIndOut] = bufferOneRead[currKeyIndOne];          // Place key in output
                bufferOut[currKeyIndOut + 1] = bufferOneRead[currKeyIndOne + 1];  // Place value in output
                currKeyIndOne = currKeyIndOne + 2;
                if (bufferOneRead[currKeyIndOne] == bufferTwoRead[currKeyIndTwo]) {
                    currKeyIndTwo = currKeyIndTwo + 2;
                }
            } else {
                bufferOut[currKeyIndOut] = bufferTwoRead[currKeyIndTwo];          // Place key in output
                bufferOut[currKeyIndOut + 1] = bufferTwoRead[currKeyIndTwo + 1];  // Place value in output
                currKeyIndTwo = currKeyIndTwo + 2;
            }
        } else if (fileOneEmpty) {
            bufferOut[currKeyIndOut] = bufferTwoRead[currKeyIndTwo];          // Place key in output
            bufferOut[currKeyIndOut + 1] = bufferTwoRead[currKeyIndTwo + 1];  // Place value in output
            currKeyIndTwo = currKeyIndTwo + 2;
        } else if (fileTwoEmpty) {
            bufferOut[currKeyIndOut] = bufferOneRead[currKeyIndOne];          // Place key in output
            bufferOut[currKeyIndOut + 1] = bufferOneRead[currKeyIndOne + 1];  // Place value in output
            currKeyIndOne = currKeyIndOne + 2;
        }

        currKeyIndOut = currKeyIndOut + 2;
        totalKeys++;

        // Check output buffer, if full flush and rest params
        if (currKeyIndOut == UINT64S_PER_PAGE) {
            bufPool->bwrite(sstNum, currPageNumOut, bufferOut);

            bufferTemp[currKeyIndTemp] = currPageNumOut;
            bufferTemp[currKeyIndTemp + 1] = bufferOut[currKeyIndOut - 2];
            currKeyIndTemp = currKeyIndTemp + 2;

            if (currKeyIndTemp == UINT64S_PER_PAGE) {
                bufPool->bwrite("temp", currPageNumTemp, bufferTemp);
                currKeyIndTemp = 0;
                currPageNumTemp++;
            }
            currKeyIndOut = 0;
            currPageNumOut++;
        }

        // Check both input buffers, if either are at the end, read in next page
        // If at end and no more pages left, what to do?
        // Done if both files are empty (done reading)
        if (currKeyIndOne == itemsInPageOne) {
            if (currPageNumOne == numPagesOne - 1) {
                fileOneEmpty = true;
            } else {
                currPageNumOne++;
                bufPool->bread(sstNum1, currPageNumOne, bufferOneRead, false);
                currKeyIndOne = 0;
                itemsInPageOne = calcNumItemsInPage(numKeysOne, currPageNumOne);
            }
        }

        if (currKeyIndTwo == itemsInPageTwo) {
            if (currPageNumTwo == numPagesTwo - 1) {
                fileTwoEmpty = true;
            } else {
                currPageNumTwo++;
                bufPool->bread(sstNum2, currPageNumTwo, bufferTwoRead, false);
                currKeyIndTwo = 0;
                itemsInPageTwo = calcNumItemsInPage(numKeysTwo, currPageNumTwo);
            }
        }

        if (fileOneEmpty && fileTwoEmpty) {
            done = true;
        }
    }

    if (currKeyIndOut > 0) {
        if (currKeyIndOut != UINT64S_PER_PAGE) {
            std::fill(bufferOut + currKeyIndOut, bufferOut + UINT64S_PER_PAGE, 0);
        }
        bufPool->bwrite(sstNum, currPageNumOut, bufferOut);

        bufferTemp[currKeyIndTemp] = currPageNumOut;
        bufferTemp[currKeyIndTemp + 1] = bufferOut[currKeyIndOut - 2];
        currKeyIndTemp = currKeyIndTemp + 2;

        currKeyIndOut = 0;
        currPageNumOut++;
    }
    if (currKeyIndTemp != UINT64S_PER_PAGE) {
        std::fill(bufferTemp + currKeyIndTemp, bufferTemp + UINT64S_PER_PAGE, 0);
    }
    bufPool->bwrite("temp", currPageNumTemp, bufferTemp);
    currKeyIndTemp = 0;
    currPageNumTemp++;
    
    bufPool->bdelete(sstNum1);
    bufPool->bdelete(sstNum2);

    return totalKeys;
}
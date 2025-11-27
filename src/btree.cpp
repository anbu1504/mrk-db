#pragma once

#include "../include/btree.hpp"

#define BRANCH_FACTOR 256

BTree::BTree(BufferPool* bufPool, uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset)
  : bufPool(bufPool),
    sstNum(sstNum),
    numKeys(numKeys),
    pageOffset(pageOffset)
{
    numLeafNodes = CALC_NUM_PAGES(numKeys * 2, UINT64_SIZE);
    numInternalNodes = 1;

    uint64_t currLayer = CEIL_DIV(numLeafNodes, BRANCH_FACTOR);
    while (currLayer != 1) {
        numInternalNodes += currLayer;
        currLayer = CEIL_DIV(currLayer, BRANCH_FACTOR);
    }
}

uint64_t BTree::findLeafPage(uint64_t key) {
    PageBuffer pageBuf;

    uint64_t currPage = pageOffset; // page corresponding to root node
    while (currPage >= 1 + numLeafNodes) { // while not a leaf node
        bufPool->bread(sstNum, currPage, pageBuf);
        currPage = getNextBTreeNode(key, pageBuf);
    }

    return currPage;
}

uint64_t BTree::getNextBTreeNode(uint64_t currKey, PageBuffer pageBuf) {
    uint64_t numKeysInNode = pageBuf[0];
    uint64_t startOfChildren = 1 + numKeysInNode;

    // If the key we're looking for is larger than the last delimiting
    // key, then we can just immediately go down to the rightmost child
    if (currKey > pageBuf[numKeysInNode]) {
        return pageBuf[startOfChildren + numKeysInNode];
    }

    // LINEAR SEARCH

    // // Otherwise, we know that currKey must be less than (or equal to)
    // // one of the delimiting keys in this node, which we must find
    // for (uint64_t delimKeyIdx = 0; delimKeyIdx < numKeysInNode; delimKeyIdx++) {
    //     if (currKey <= pageBuf[1 + delimKeyIdx]) {
    //         currPage = pageBuf[startOfChildren + delimKeyIdx];
    //         break;
    //     }
    // }

    // BINARY SEARCH

    // If the key we're looking for is smaller than/equall to the first delimiting
    // key, then we can just immediately go down to the leftmost child
    if (currKey <= pageBuf[1]) {
        return pageBuf[startOfChildren];
    }

    uint64_t lo = 1;                  // Corresponds to the second key (we alr. checked for left child of the first key)
    uint64_t hi = numKeysInNode - 1;  // Index of last key (we alr. checked for right child of the last key)

    // Note, pageBuf[1 + mid] is the key we're currently inspecting
    // (+1 for offset), while pageBuf[mid] is the key before it
    uint64_t mid = binSearch(lo, hi, [&](int m) {
        return (currKey <= pageBuf[m]) ? -1 : (currKey > pageBuf[1 + m]) ? 1 : 0;
    });  // Else case: pageBuf[mid] < currKey && currKey <= pageBuf[1 + mid]

    return pageBuf[startOfChildren + mid];
}

uint64_t BTree::createFromMem(std::vector<uint64_t>* memtableData) {}
uint64_t BTree::createFromDisk() {}
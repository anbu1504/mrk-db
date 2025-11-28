#pragma once

#include "bufferpool.hpp"
#include "constants.hpp"

typedef std::tuple<uint64_t, uint64_t> BTNodeData;
// A B-Tree node, consisting of (in this order): # keys, keys vector, children vector
typedef std::tuple<uint64_t, std::vector<uint64_t>, std::vector<uint64_t>> BTNode;

class BTree {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;
    uint64_t numKeys;
    uint64_t pageOffset;
    uint64_t numInternalNodes;
    uint64_t numLeafNodes;

    uint64_t getNextBTreeNode(uint64_t currKey, PageBuffer pageBuf);
    void constructLayer(uint64_t currLayerOffset, uint64_t currLayerSize, uint64_t childLayerSize,
                        std::vector<BTNodeData>* currLayerData, std::vector<BTNodeData>* childLayerData);
    void loadLayerData(std::vector<BTNodeData>* layerData, uint64_t pageNum);
    void writeLayerData(std::vector<BTNodeData>* layerData, uint64_t pageNum);
    void writeNode(BTNode node, uint64_t pageNum);

   public:
    BTree(BufferPool* bufPool, uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset);

    uint64_t findLeafPage(uint64_t key);
    uint64_t createBTree(std::vector<uint64_t>* memtableData);
};

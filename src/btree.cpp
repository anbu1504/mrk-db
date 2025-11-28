#include "../include/btree.hpp"

#define BRANCH_FACTOR 256
#define LAYER_DATA_THRESHOLD 256  // Corresponds to a page's worth of layer data

BTree::BTree(BufferPool* bufPool, uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset)
    : bufPool(bufPool),
      sstNum(sstNum),
      numKeys(numKeys),
      pageOffset(pageOffset),
      numLeafNodes(CALC_NUM_PAGES(numKeys * 2, UINT64_SIZE)) {
    numInternalNodes = 1;

    uint64_t currLayer = CEIL_DIV(numLeafNodes, BRANCH_FACTOR);
    while (currLayer != 1) {
        numInternalNodes += currLayer;
        currLayer = CEIL_DIV(currLayer, BRANCH_FACTOR);
    }
}

uint64_t BTree::findLeafPage(uint64_t key) {
    PageBuffer pageBuf;

    uint64_t currPage = pageOffset;         // page corresponding to root node
    while (currPage >= 1 + numLeafNodes) {  // while not a leaf node
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

uint64_t BTree::createBTree(std::vector<uint64_t>* memtableData) {
    std::vector<uint64_t> layerSizes = {
        CEIL_DIV(numLeafNodes, BRANCH_FACTOR),
    };  // the sizes of each internal layer, bottom-up

    while (layerSizes.back() != 1) {
        layerSizes.push_back(CEIL_DIV(layerSizes.back(), BRANCH_FACTOR));
    }

    std::vector<BTNodeData> leafData;

    if (!memtableData->empty()) {
        for (uint64_t leafNum = 0; leafNum < numLeafNodes - 1; leafNum++) {
            // Start with the index for the *beginning* of the *next* page,
            // then subtract 2 to obtain the index of the *end* of *this* page
            uint64_t lastIdxInPage = UINT64S_PER_PAGE * (leafNum + 1) - 2;

            leafData.push_back(std::make_tuple(1 + leafNum, memtableData->at(lastIdxInPage)));
        }

        // add the very last page, and the very last key in memdata_table
        leafData.push_back(std::make_tuple(1 + numLeafNodes - 1, memtableData->at(memtableData->size() - 2)));
    }

    // pageOffset should correspond to numLeafNodes + numFilterPages + 1
    uint64_t currLayerOffset = pageOffset + numInternalNodes;

    uint64_t childLayerSize = numLeafNodes;
    std::vector<BTNodeData> childLayerData = leafData;  // list of page-nums and max-keys
    std::vector<BTNodeData> currLayerData;              // list of page-nums and max-keys
    std::vector<BTNode> currLayer;                      // list of nodes in the current layer

    for (size_t layerNum = 0; layerNum < layerSizes.size(); layerNum++) {
        // Used to calculate the actual page number of each node
        currLayerOffset -= layerSizes[layerNum];

        constructLayer(currLayerOffset, layerSizes[layerNum], childLayerSize, &currLayerData, &childLayerData);

        childLayerData.clear();
        std::swap(currLayerData, childLayerData);

        // Update childLayerSize value for next iteration
        childLayerSize = layerSizes[layerNum];
    }

    // TODO: remember to delete the temp file here if memtableData was empty!!
    if (!memtableData->empty()) {
        bufPool->bdelete("temp");
    }
}

void BTree::constructLayer(uint64_t currLayerOffset, uint64_t currLayerSize, uint64_t childLayerSize,
                           std::vector<BTNodeData>* currLayerData, std::vector<BTNodeData>* childLayerData) {
    uint64_t minChildrenPerNode = childLayerSize / currLayerSize;
    uint64_t extraChildren = childLayerSize % currLayerSize;

    // Used when reading from the temp file
    uint64_t numTempFilePagesRead = 0;
    // Used when writing to the temp file
    uint64_t numTempFilePagesWritten = 0;

    std::vector<BTNode> currLayer;

    uint64_t currChildIdx = 0;
    for (uint64_t nodeNum = 0; nodeNum < currLayerSize; nodeNum++) {
        uint64_t nodeChildrenCount = minChildrenPerNode + (nodeNum < extraChildren);

        std::vector<uint64_t> keysVec;
        std::vector<uint64_t> childrenVec;
        uint64_t finalChildMax;

        for (uint64_t childNum = 0; childNum < nodeChildrenCount; childNum++) {
            if (currChildIdx >= childLayerData->size()) {
                loadLayerData(childLayerData, numTempFilePagesRead);
                currChildIdx = 0;
                numTempFilePagesRead++;
            }

            BTNodeData currChild = childLayerData->at(currChildIdx);

            if (childNum == nodeChildrenCount - 1) {
                finalChildMax = std::get<1>(currChild);
            } else {
                keysVec.push_back(std::get<1>(currChild));
            }
            childrenVec.push_back(std::get<0>(currChild));
            currChildIdx++;
        }

        writeNode(std::make_tuple(nodeChildrenCount - 1, keysVec, childrenVec), currLayerOffset + nodeNum);
        currLayerData->push_back(std::make_tuple(currLayerOffset + nodeNum, finalChildMax));

        if (currLayerData->size() >= LAYER_DATA_THRESHOLD) {
            writeLayerData(currLayerData, numTempFilePagesWritten);
            numTempFilePagesWritten++;
        }
    }

    // if we've written to the temp file for this layer, then write out remaining data as well
    if (numTempFilePagesWritten) {
        writeLayerData(currLayerData, numTempFilePagesWritten);
    }
}

void BTree::loadLayerData(std::vector<BTNodeData>* layerData, uint64_t pageNum) {
    layerData->clear();

    PageBuffer pageBuf;
    bufPool->bread("temp", pageNum, pageBuf);

    for (uint64_t nodeDataIdx = 0; nodeDataIdx < UINT64S_PER_PAGE / 2; nodeDataIdx++) {
        layerData->push_back(std::make_tuple(pageBuf[nodeDataIdx], pageBuf[nodeDataIdx + 1]));
    }
}

void BTree::writeLayerData(std::vector<BTNodeData>* layerData, uint64_t pageNum) {
    PageBuffer pageBuf = {0};

    for (uint64_t nodeDataIdx = 0; nodeDataIdx < layerData->size(); nodeDataIdx++) {
        pageBuf[nodeDataIdx * 2] = std::get<0>(layerData->at(nodeDataIdx));
        pageBuf[nodeDataIdx * 2 + 1] = std::get<1>(layerData->at(nodeDataIdx));
    }

    bufPool->bwrite("temp", pageNum, pageBuf);

    layerData->clear();
}

void BTree::writeNode(BTNode node, uint64_t pageNum) {
    PageBuffer pageBuf = {0};
    pageBuf[0] = std::get<0>(node);
    std::copy(std::get<1>(node).begin(), std::get<1>(node).end(), pageBuf + 1);
    std::copy(std::get<2>(node).begin(), std::get<2>(node).end(), pageBuf + std::get<1>(node).size());

    bufPool->bwrite(sstNum, pageNum, pageBuf);
}

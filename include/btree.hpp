#pragma once

#include "bufferpool.hpp"
#include "constants.hpp"

typedef std::tuple<uint64_t, uint64_t> BTNodeData;
// A B-Tree node, consisting of (in this order): # keys, keys vector, children vector
typedef std::tuple<uint64_t, std::vector<uint64_t>, std::vector<uint64_t>> BTNode;

/**
 * @class BTree
 * @brief Disk-backed B-Tree used for locating SST pages by key.
 */
class BTree {
   private:
    BufferPool* bufPool;
    uint64_t sstNum;
    uint64_t numKeys;
    uint64_t pageOffset;
    uint64_t numInternalNodes;
    uint64_t numLeafNodes;

    /**
     * @brief Finds the next child node page for the provided key.
     *
     * @param currKey The key used to navigate the B-Tree
     * @param pageBuf Buffer containing the current node's data
     */
    uint64_t getNextBTreeNode(uint64_t currKey, PageBuffer pageBuf);

    /**
     * @brief Constructs a single internal layer of the B-Tree.
     *
     * @param currLayerOffset Page offset where the current layer begins
     * @param currLayerSize Number of nodes in the current layer
     * @param childLayerSize Number of nodes in the child layer
     * @param currLayerData Vector to populate with current layer page numbers and max keys
     * @param childLayerData Vector containing child layer page numbers and max keys
     */
    void constructLayer(uint64_t currLayerOffset, uint64_t currLayerSize, uint64_t childLayerSize,
                        std::vector<BTNodeData>* currLayerData, std::vector<BTNodeData>* childLayerData);

    /**
     * @brief Loads serialized layer data from a temporary page into memory.
     *
     * @param layerData Vector to fill with layer data tuples
     * @param pageNum Temporary page number to read from
     */
    void loadLayerData(std::vector<BTNodeData>* layerData, uint64_t pageNum);

    /**
     * @brief Writes serialized layer data to a temporary page.
     *
     * @param layerData Vector containing layer data tuples to persist
     * @param pageNum Temporary page number to write to
     */
    void writeLayerData(std::vector<BTNodeData>* layerData, uint64_t pageNum);

    /**
     * @brief Writes a single B-Tree node to disk.
     *
     * @param node Node data to serialize
     * @param pageNum Page number to write the node to
     */
    void writeNode(BTNode node, uint64_t pageNum);

   public:
    /**
     * @brief Constructor for initializing a BTree.
     *
     * @param bufPool Pointer to the buffer pool used for page I/O
     * @param sstNum The SST number the B-Tree is associated with
     * @param numKeys Number of keys contained in the SST
     * @param pageOffset The starting page offset for B-Tree storage
     */
    BTree(BufferPool* bufPool, uint64_t sstNum, uint64_t numKeys, uint64_t pageOffset);

    /**
     * @brief Finds the leaf page that may contain a given key.
     *
     * @param key The key to search for
     */
    uint64_t findLeafPage(uint64_t key);

    /**
     * @brief Builds the B-Tree from memtable data.
     *
     * @param memtableData Pointer to a vector storing memtable data in <key, value> order
     */
    void createBTree(std::vector<uint64_t>* memtableData);
};

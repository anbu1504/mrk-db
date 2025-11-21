#pragma once

#include "bloomfilter.hpp"
#include "constants.hpp"
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <tuple>
#include <optional>

// Number of 8-byte items that can fit in a page (4096/8)
#define ENTRIES_PER_PAGE 512
// B-Tree branch factor: Size of a page divided by size of a (key,page #) pair (4096/16)
#define BRANCH_FACTOR 256

// A B-Tree node, consisting of (in this order): # keys, keys vector, children vector
typedef std::tuple<uint64_t, std::vector<uint64_t>, std::vector<uint64_t>> BTNode;

/**
 * @class Node
 * @brief Represents a node in the AVL tree used in the memtable.
 *
 * Each node stores a key-value pair (both int64_t), pointers to left and right children,
 * and the height of the node for balancing purposes.
 */
class Node {
public:
    uint64_t key;
    uint64_t value;
    Node* left;
    Node* right;    
    int height;
    void deleteNode();
    Node(uint64_t k, uint64_t v) : key(k), value(v), left(nullptr), right(nullptr), height(1) {}
};

/**
 * @class Memtable
 * @brief Balanced AVL tree implementation of a memtable for storing key-value pairs.
 *
 * Supports insertion of key-value pairs. Automatically flushes to disk
 * when the number of entries reaches the memtable_size threshold.
 */
class Memtable {
    friend class MemtableTester; // to be used in tester
private:
    Node* root;
    size_t size;
    size_t threshold;

    // Private helper functions
    Node* insertRec(Node* node, uint64_t key, uint64_t value); // recursive insert
    int height(Node* node); // get height of node
    int getBalance(Node* node); // get balance factor
    Node* rotateRight(Node* y); // right rotation
    Node* rotateLeft(Node* x); // left rotation
    void deleteTree(); // helper to free memory
    void scanTreeRec(std::vector<std::tuple<uint64_t, uint64_t>> *entries, Node* node, uint64_t min, uint64_t max); // recurive scan
    std::optional<uint64_t> getValueRec(Node* node, uint64_t k); // recursive get value
    std::vector<uint64_t> inorderTraversalDel(); // helper for display 
    void inorderTraversalDelRec(std::vector<uint64_t> *entries, Node* node); // recursive inorder traversal
    uint64_t getMax(Node* node); // helper for flush
    uint64_t getMin(Node* node); // helper for flush
    std::vector<BTNode> constructInternalNodes(std::vector<uint64_t>* memtable_data, uint64_t num_filter_pages); // helper for flush (B-Tree)
    std::vector<uint64_t> flattenInternalNodes(std::vector<BTNode>* internalNodes); // helper for flush (B-Tree)
    BloomFilter constructBloomFilter(std::vector<uint64_t>* memtable_data);
    std::vector<BTNode> constructLayer(
        uint64_t page_offset,
        uint64_t layer_size,
        std::vector<std::tuple<uint64_t, uint64_t>>* child_layer_data,
        std::vector<std::tuple<uint64_t, uint64_t>>* curr_layer_data // output for list of page-nums and max-keys
    );
    void checkWrite(ssize_t written, int fd, ssize_t desiredWriteAmount);

public:
    Memtable(size_t threshold); // constructor with threshold
    ~Memtable(); // destructor to free memory

    // Public helper functions
    bool insert(uint64_t key, uint64_t value); // public insert method
    bool isThresholdReached(); // check if threshold has been reached
    size_t getSize(); // get current size
    std::vector<std::tuple<uint64_t, uint64_t>> scanTree(uint64_t min, uint64_t max); // scan method
    std::optional<uint64_t> getValue(uint64_t key); 
    std::tuple<size_t, size_t, uint64_t, uint64_t, uint64_t>  flushToDiskBTree(std::string filename); // flush to disk as a B-Tree
    Node* getRoot(); // get root of memtable
    bool isEmpty(); // helper function for checking if empty
};
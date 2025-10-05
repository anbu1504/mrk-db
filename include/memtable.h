#pragma once

#include <iostream>
#include <cstdint>
#include <algorithm>
#include <fstream>
using namespace std;

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
    Node(int64_t k, int64_t v) : key(k), value(v), left(nullptr), right(nullptr), height(1) {}
};

/**
 * @class Memtable
 * @brief Balanced AVL tree implementation of a memtable for storing key-value pairs.
 *
 * Supports insertion of key-value pairs. Automatically flushes to disk
 * when the number of entries reaches the memtable_size threshold.
 */
class Memtable {
private:
    Node* root;
    size_t size;
    size_t threshold;

    // Private helper functions
    int height(Node* N); // get height of node
    int getBalance(Node* N); // get balance factor
    Node* rotateRight(Node* y); // right rotation
    Node* rotateLeft(Node* x); // left rotation
    Node* insert(Node* node, int64_t key, int64_t value); // recursive insert
    void inorderTraversal(Node* root, std::ofstream& ofs); // helper for display
    void deleteTree(); // helper to free memory

public:
    Memtable(size_t threshold); // constructor with threshold
    ~Memtable(); // destructor to free memory

    // Public helper functions
    void insert(int64_t key, int64_t value); // public insert method
    void display(); // public display method
    bool isThresholdReached(); // check if threshold has been reached
    size_t getSize(); // get current size
    int flushToDisk(std::string filename); // flush to disk
};
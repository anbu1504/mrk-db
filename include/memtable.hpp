#pragma once

#include "constants.hpp"

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
    /**
     * @brief Recursively deletes this node and its children.
     */
    void deleteNode();
    /**
     * @brief Constructs an AVL tree node.
     *
     * @param k Key to store
     * @param v Value to store
     */
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
    friend class MemtableTester;  // to be used in tester
   private:
    Node* root;
    size_t size;
    size_t threshold;

    // Private helper functions
    /**
     * @brief Recursively inserts a key-value pair into the AVL tree.
     *
     * @param node Current subtree root
     * @param key Key to insert
     * @param value Value to insert
     */
    Node* insertRec(Node* node, uint64_t key, uint64_t value);

    /**
     * @brief Returns the height of a node.
     *
     * @param node Node to inspect
     */
    int height(Node* node);

    /**
     * @brief Calculates the balance factor for a node.
     *
     * @param node Node to inspect
     */
    int getBalance(Node* node);

    /**
     * @brief Performs a right rotation around the given node.
     *
     * @param y Root of the subtree to rotate
     */
    Node* rotateRight(Node* y);

    /**
     * @brief Performs a left rotation around the given node.
     *
     * @param x Root of the subtree to rotate
     */
    Node* rotateLeft(Node* x);

    /**
     * @brief Recursively frees all nodes in the tree.
     */
    void deleteTree();

    /**
     * @brief Recursively collects key-value pairs within a range.
     *
     * @param entries Output vector of entries
     * @param node Current subtree root
     * @param min Minimum key (inclusive)
     * @param max Maximum key (inclusive)
     */
    void scanTreeRec(std::vector<std::tuple<uint64_t, uint64_t>>* entries, Node* node, uint64_t min, uint64_t max);

    /**
     * @brief Recursively retrieves the value for a key.
     *
     * @param node Current subtree root
     * @param k Key to search for
     */
    std::optional<uint64_t> getValueRec(Node* node, uint64_t k);

    /**
     * @brief Recursively performs inorder traversal and collects keys and deletes nodes in the tree.
     *
     * @param entries Output vector of keys
     * @param node Current subtree root
     */
    void inorderTraversalDelRec(std::vector<uint64_t>* entries, Node* node);

    /**
     * @brief Finds the maximum key in a subtree.
     *
     * @param node Subtree root
     */
    uint64_t getMax(Node* node);

    /**
     * @brief Finds the minimum key in a subtree.
     *
     * @param node Subtree root
     */
    uint64_t getMin(Node* node);

   public:
    /**
     * @brief Constructs a memtable with a flush threshold.
     *
     * @param threshold Maximum number of entries before flushing
     */
    Memtable(size_t threshold);

    /**
     * @brief Frees the tree and associated memory.
     */
    ~Memtable();

    /**
     * @brief Inserts a key-value pair into the memtable.
     *
     * @param key Key to insert
     * @param value Value to insert
     * @return True if inserted, false if already present
     */
    bool insert(uint64_t key, uint64_t value);

    /**
     * @brief Checks whether the memtable has reached its flush threshold.
     */
    bool isThresholdReached();

    /**
     * @brief Returns the current number of entries.
     */
    size_t getSize();

    /**
     * @brief Scans keys in the given range.
     *
     * @param min Inclusive minimum key
     * @param max Inclusive maximum key
     */
    std::vector<std::tuple<uint64_t, uint64_t>> scanTree(uint64_t min, uint64_t max);

    /**
     * @brief Retrieves the value for a given key.
     *
     * @param key Key to look up
     */
    std::optional<uint64_t> getValue(uint64_t key);

    /**
     * @brief Returns the root node pointer.
     */
    Node* getRoot();

    /**
     * @brief Indicates if the memtable is empty.
     */
    bool isEmpty();

    /**
     * @brief Performs inorder traversal, deleting nodes and returning keys.
     */
    std::vector<uint64_t> inorderTraversalDel();

    /**
     * @brief Prints the memtable contents to stdout.
     */
    void print();

    /**
     * @brief Recursively prints the subtree rooted at the provided node.
     *
     * @param node Subtree root to print
     */
    void printRec(Node* node);
};

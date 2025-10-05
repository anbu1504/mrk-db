#include "memtable.h"
#include <fstream>
#include <iostream>
#include <cstdlib>

/**
 * @brief Constructor to initialize the memtable with a size threshold.
 */

Memtable::Memtable(size_t threshold) : root(nullptr), size(0), threshold(threshold) {}

/**
 * @brief Destructor to free memory allocated for the AVL tree.
 */
Memtable::~Memtable() {
    // Free the memory allocated for the AVL tree
    deleteTree();
}

void Node::deleteNode() {
    if (left) {
        left->deleteNode();
        free(left);
    }

    if (right) {
        right->deleteNode();
        free(right);
    }
    
}
 
/**
 * @brief Method to recursively delete the entire AVL tree
 */

 void Memtable::deleteTree() {
    if (!root) {
        return;
    }
    else {
        root->deleteNode();
        free(root);
    }
 }

/**
 * @brief Get the height of a node.
 */
int Memtable::height(Node* N) {
    if (N == nullptr)
        return 0;
    return N->height;
}

/**
 * @brief Method to get the balance factor of the tree
 */




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

uint64_t Node::getValue(uint64_t k) {
    if (key == k){
        return value;

    } else if (left && key > k){
        return left->getValue(k);

    } else if (right && key < k) {
        return right->getValue(k);
    }

    if (!left && !right){
        return NULL;
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
int Memtable::height(Node* node) {
    if (node == nullptr)
        return 0;
    return node->height;
}

/**
 * @brief Method to get the balance factor of the tree
 */
int Memtable::getBalance(Node* node) {
    if (node == nullptr) {
        return 0;
    }
    return height(node->right) - height(node->left);
}

std::tuple<uint64_t> Memtable::getValue(uint64_t key) {
    if (!root){
        return NULL;
    }
    else {
        return root->getValue(key);
    }
}

std::vector<std::tuple<uint64_t, uint64_t>> scanTree(uint64_t min, uint64_t max){

}

/**
 * @brief Helper method to insert recursively
 */
void Memtable::insert_real(Node* node, uint64_t key, uint64_t value) {
    if (node == nullptr) {
        size++;
    }

    if (key < node->key) {
        insert_real(node->left, key, value);
    }

    if (key > node->key) {
        insert_real(node->right, key, value);
    }

    node->height = 1 + std::max(height(node->left), height(node->right));

    int balance = getBalance(node);

    if (balance < -1 && key < node->left->key)
        rotateRight(node);

    if (balance > 1 && key > node->right->key)
        rotateLeft(node);

    if (balance < -1 && key > node->left->key) {
        rotateLeft(node->left);
        rotateRight(node);
    }

    if (balance > 1 && key < node->right->key) {
        rotateRight(node->right);
        rotateLeft(node);
    }
}

/** 
 * @brief Method to insert
 * */ 
bool Memtable::insert(uint64_t key, uint64_t value) {
    if (isThresholdReached()) {
        return false;
    }
    insert_real(root, key, value);
    return true;
}

/**
 * @brief Method to rotate right
 */
void Memtable::rotateRight(Node* y) {
    Node* x = y->left;
    Node* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = std::max(height(y->left), height(y->right)) + 1;
    x->height = std::max(height(x->left), height(x->right)) + 1;
}

/**
 * @brief Method to rotate left
 */
void Memtable::rotateLeft(Node* x) {
    Node* y = x->right;
    Node* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = std::max(height(x->left), height(x->right)) + 1;
    y->height = std::max(height(y->left), height(y->right)) + 1;
}
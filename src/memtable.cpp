#include "../include/memtable.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <optional>


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
        delete left;
    }

    if (right) {
        right->deleteNode();
        delete right;
    }
    
}

std::optional<uint64_t> Node::getValue(uint64_t k) {
    if (key == k){
        return value;

    } else if (left && key > k){
        return left->getValue(k);

    } else if (right && key < k) {
        return right->getValue(k);
    }

    else {
        return std::nullopt;
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
        delete root;
        root = nullptr;
        size = 0;
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

std::tuple<uint64_t,std::optional<uint64_t>> Memtable::getValue(uint64_t key) {
    if (!root){
        return std::make_tuple(key, std::nullopt);
    }
    else {
        return std::make_tuple(key, root->getValue(key));
    }
}

std::vector<std::tuple<uint64_t, uint64_t>> scanTree(uint64_t min, uint64_t max){
    
}

/**
 * @brief Helper method to insert recursively
 */
Node* Memtable::insertReal(Node* node, uint64_t key, uint64_t value) {
    
    if (node == nullptr) {
        size++;
        return new Node(key, value);
    }
    
    if (key < node->key) {
        node->left = insertReal(node->left, key, value);
    }
    else if (key > node->key) {
        node->right = insertReal(node->right, key, value);
    }    
    node->height = 1 + std::max(height(node->left), height(node->right));

    int balance = getBalance(node);

    if (balance < -1 && key < node->left->key)
        return rotateRight(node);

    if (balance > 1 && key > node->right->key)
        return rotateLeft(node);

    if (balance < -1 && key > node->left->key) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    if (balance > 1 && key < node->right->key) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }
    return node;
}

/** 
 * @brief Method to insert
 * */ 
bool Memtable::insert(uint64_t key, uint64_t value) {
    if (isThresholdReached()) {
        return false;
    }
    root = insertReal(root, key, value);
    return true;
}

/**
 * @brief Method to rotate right
 */
Node* Memtable::rotateRight(Node* y) {
    Node* x = y->left;
    Node* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = std::max(height(y->left), height(y->right)) + 1;
    x->height = std::max(height(x->left), height(x->right)) + 1;
    
    return x;
}

/**
 * @brief Method to rotate left
 */
Node* Memtable::rotateLeft(Node* x) {
    Node* y = x->right;
    Node* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = std::max(height(x->left), height(x->right)) + 1;
    y->height = std::max(height(y->left), height(y->right)) + 1;

    return y;
}

size_t Memtable::getSize() {
    return size;
}

bool Memtable::isThresholdReached() {
    return size >= threshold;
}

Node* Memtable::getRoot() {
    return root;
}


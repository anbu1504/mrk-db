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

std::optional<uint64_t> Memtable::getValueRec(Node* node, uint64_t k) {
    if (node == nullptr){
        return std::nullopt;
    }

    if (node->key == k){
        return node->value;

    } else if (node->key > k){
        return getValueRec(node->left, k);

    } else {
        return getValueRec(node->right, k);
    } 
}

void Memtable::scanTreeRec(std::vector<std::tuple<uint64_t, uint64_t>> *entries, Node* node, uint64_t min, uint64_t max) {
    if (!node) return;

    if (node->key > min)
        scanTreeRec(entries, node->left, min, max);

    if (node->key >= min && node->key <= max)
        entries->push_back(std::make_tuple(node->key, node->value));

    if (node->key < max)
        scanTreeRec(entries, node->right, min, max);
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

std::optional<uint64_t> Memtable::getValue(uint64_t key) {
    return getValueRec(root, key);
}

std::vector<std::tuple<uint64_t, uint64_t>> Memtable::scanTree(uint64_t min, uint64_t max){
    std::vector<std::tuple<uint64_t, uint64_t>> entries;

    scanTreeRec(&entries, root, min, max);

    return entries;
}

/**
 * @brief Helper method to insert recursively
 */
Node* Memtable::insertRec(Node* node, uint64_t key, uint64_t value) {
    
    if (node == nullptr) {
        size++;
        return new Node(key, value);
    }
    
    if (key < node->key) {
        node->left = insertRec(node->left, key, value);
    }
    else if (key > node->key) {
        node->right = insertRec(node->right, key, value);
    }    
    node->height = 1 + std::max(height(node->left), height(node->right));

    int balance = getBalance(node);

    if (balance < -1 && key < node->left->key) {
        return rotateRight(node);
    }

    if (balance > 1 && key > node->right->key) {
        return rotateLeft(node);
    }

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
    root = insertRec(root, key, value);
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

void Memtable::inorderTraversalDelRec(std::vector<std::tuple<uint64_t, uint64_t>> *entries, Node* node){

    if (node->left != nullptr){
        inorderTraversalDelRec(entries, node->left);
        delete node->left;
        node->left = nullptr;
    }

    entries->push_back(std::make_tuple(node->key, node->value));

    if (node->right != nullptr){
        inorderTraversalDelRec(entries, node->right);
        delete node->right;
        node->right = nullptr;
    }
}

std::vector<std::tuple<uint64_t, uint64_t>> Memtable::inorderTraversalDel(){
    std::vector<std::tuple<uint64_t, uint64_t>> entries;

    if (root != nullptr){
        inorderTraversalDelRec(&entries, root);
        delete root;
        root = nullptr;
        size = 0;
    }

    return entries;
}

bool Memtable::isEmpty() {
    return size == 0;
}


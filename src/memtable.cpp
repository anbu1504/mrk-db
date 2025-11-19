#include "../include/memtable.hpp"
#include "../include/bloomfilter.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <optional>

#include <fcntl.h>
#include <unistd.h>  
#include <cerrno>
#include <cstring>

#define PAGE_SIZE 4096
#define CEIL_DIV(x, y) ((x) / (y) + ((x) % (y) != 0))
// Append the elements of v2 to v1
#define VEC_APPEND(v1, v2) ((v1).insert((v1).end(), (v2).begin(), (v2).end()))

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

void Memtable::inorderTraversalDelRec(std::vector<uint64_t> *entries, Node* node){

    if (node->left != nullptr){
        inorderTraversalDelRec(entries, node->left);
        delete node->left;
        node->left = nullptr;
    }

    entries->push_back(node->key);
    entries->push_back(node->value);

    if (node->right != nullptr){
        inorderTraversalDelRec(entries, node->right);
        delete node->right;
        node->right = nullptr;
    }
}

std::vector<uint64_t> Memtable::inorderTraversalDel(){
    std::vector<uint64_t> entries;

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

uint64_t Memtable::getMax(Node* node) {
    if (node->right == nullptr){
        return node->key;
    } else {
        return getMax(node->right);
    }
}

uint64_t Memtable::getMin(Node* node) {
    if (node->left == nullptr){
        return node->key;
    } else {
        return getMin(node->left);
    }
}

std::tuple<size_t, size_t, uint64_t, uint64_t> Memtable::flushToDiskBTree(std::string filename) {

    if (root == nullptr){
        return std::make_tuple(0, 0, 0, 0);
    }

    // ========== Metadata Stuff Begins ==========

    // Metadata values
    uint64_t min = getMin(root);
    uint64_t max = getMax(root);
    size_t flushed_size = size;
    size_t num_internal_nodes;

    std::vector<uint64_t> memtable_data = inorderTraversalDel();
    // Construct the internal nodes of the B-Tree, and flatten them for writing to disk
    std::vector<BTNode> internal_nodes = constructInternalNodes(&memtable_data);
    std::vector<uint64_t> internal_data = flattenInternalNodes(&internal_nodes);
    num_internal_nodes = internal_nodes.size();


    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1){
        close(fd);
        throw std::runtime_error(std::string("open failed: ") + std::strerror(errno));
    }

    ssize_t written = write(fd, &flushed_size, sizeof(size_t));
    checkWrite(written, fd, sizeof(size_t));

    written = write(fd, &num_internal_nodes, sizeof(size_t));
    checkWrite(written, fd, sizeof(size_t));

    written = write(fd, &min, sizeof(uint64_t));
    checkWrite(written, fd, sizeof(uint64_t));

    written = write(fd, &max, sizeof(uint64_t));
    checkWrite(written, fd, sizeof(uint64_t));

    size_t header_bytes = sizeof(size_t) + 3*sizeof(uint64_t);
    size_t padding = 4096 - header_bytes;
    std::vector<char> zero_buf(padding, 0);
    written = write(fd, zero_buf.data(), padding);
    checkWrite(written, fd, padding);

    // ========== Metadata Stuff Ends ==========

    // The SST will be structured as follows:
    // Page 0: Metadata
    // Pages 1 - num_internal_nodes: B-Tree internal nodes
    // Pages (num_internal_nodes + 1) - end: Clustered leaf nodes

    // Each B-Tree internal node will be structured as follows:
    // [uint_64t: # keys in node]|[contiguous uint_64ts: keys/delimiters]|[contiguous uint_64ts: children]

    written = write(fd, internal_data.data(), internal_data.size() * sizeof(uint64_t));
    checkWrite(written, fd, internal_data.size() * sizeof(uint64_t));
    
    written = write(fd, memtable_data.data(), memtable_data.size() * sizeof(uint64_t));
    checkWrite(written, fd, memtable_data.size() * sizeof(uint64_t));

    close(fd);
    return std::make_tuple(flushed_size, num_internal_nodes, min, max);
}

// Bloom Filter Stuff

BloomFilter constructBloomFilter(std::vector<uint64_t>* memtable_data) {
    ssize_t num_keys = memtable_data->size() / 2;

    BloomFilter bloom_filter(num_keys);

    for (int key_num = 0; key_num < num_keys; key_num++) {
        bloom_filter.addKey(memtable_data->at(key_num * 2));
    }

    return bloom_filter;
}

// B-Tree Stuff

std::vector<BTNode> Memtable::constructInternalNodes(std::vector<uint64_t>* memtable_data) {
    uint64_t num_internal_nodes = 1; // Starts at 1 to account for the root node
    uint64_t num_leaf_nodes = memtable_data->size() / ENTRIES_PER_PAGE;

    std::vector<uint64_t> layer_sizes = {CEIL_DIV(num_leaf_nodes, BRANCH_FACTOR), }; // the sizes of each internal layer, bottom-up

    while (layer_sizes.back() != 1) {
        num_internal_nodes += layer_sizes.back();
        layer_sizes.push_back(CEIL_DIV(layer_sizes.back(), BRANCH_FACTOR));
    }

    uint64_t page_offset = num_internal_nodes + 1; // +1 to account for the metadata page

    // let's make a tuple to represent each leaf page, which will just
    // contain the page number (including offset) and the max key in the page
    std::vector<std::tuple<uint64_t, uint64_t>> leaf_pages;

    for (uint64_t leaf_num = 0; leaf_num < num_leaf_nodes - 1; leaf_num++) {
        // Start with the index for the *beginning* of the *next* page,
        // then subtract 2 to obtain the index of the *end* of *this* page
        uint64_t last_idx_in_page = ENTRIES_PER_PAGE * (leaf_num + 1) - 2;

        leaf_pages.push_back(std::make_tuple(page_offset + leaf_num, memtable_data->at(last_idx_in_page)));
    }

    // add the very last page, and the very last key in memdata_table
    leaf_pages.push_back(std::make_tuple(page_offset + num_leaf_nodes - 1, memtable_data->at(memtable_data->size() - 2)));


    std::vector<BTNode> finalNodeVec; // A vector to hold our final output

    std::vector<std::tuple<uint64_t, uint64_t>> child_layer_data = leaf_pages; // list of page-nums and max-keys
    std::vector<std::tuple<uint64_t, uint64_t>> curr_layer_data; // list of page-nums and max-keys
    std::vector<BTNode> curr_layer; // list of nodes in the current layer


    for (size_t layer_num = 0; layer_num < layer_sizes.size(); layer_num++) {
        // Used to calculate the actual page number of each node
        page_offset -= layer_sizes[layer_num];

        curr_layer = constructLayer(page_offset, layer_sizes[layer_num], &child_layer_data, &curr_layer_data);

        // Add everything in curr_layer to finalNodeVec, from largest page-num to smallest
        while (!curr_layer.empty()) {
            finalNodeVec.push_back(curr_layer.back());
            curr_layer.pop_back();
        }

        std::swap(curr_layer_data, child_layer_data);
        curr_layer_data.clear();
    }

    // Reverse finalNodeVec so that the nodes are in order from smallest page-num to largest
    std::reverse(finalNodeVec.begin(), finalNodeVec.end());

    return finalNodeVec;
}

// Constructs a layer of the B-Tree, and returns a list of BTNodes
// (also stores current layer data in the curr_layer_data variable)
std::vector<BTNode> Memtable::constructLayer(
    uint64_t page_offset,
    uint64_t layer_size,
    std::vector<std::tuple<uint64_t, uint64_t>>* child_layer_data,
    std::vector<std::tuple<uint64_t, uint64_t>>* curr_layer_data // output for list of page-nums and max-keys
) {
    std::vector<BTNode> curr_layer; // list of nodes in the current layer

    // The collective total number of children to this layer
    uint64_t total_layer_children = child_layer_data->size();

    // The minimum number of children each node in this layer should have
    uint64_t min_children_per_node = total_layer_children / layer_size;

    // The number of extra children that need to be assigned to nodes as we go
    uint64_t extra_children = total_layer_children % layer_size;

    // Explanation of how the three vars above work:
    // Let's suppose we have 14 total_layer_children, and that this layer has 4 nodes
    // Then, we'd have 3 min_children_per_node, and 2 extra_children left over
    // We will then assign one extra child to each node as we go until we run out
    // (i.e., the first two nodes will have 4 children each, and the rest will have 3 each)

    uint64_t curr_child = 0;

    for (uint64_t node_num = 0; node_num < layer_size; node_num++) {
        // Number of children for this node
        uint64_t node_children_count = min_children_per_node;

        if (extra_children) { // If there're extra children, assign one to this node
            node_children_count++;
            extra_children--;
        }
        
        std::vector<u_int64_t> keys_vector;
        std::vector<u_int64_t> children_vector;

        uint64_t final_child_max;

        for (uint64_t child_num = 0; child_num < node_children_count; child_num++) {
            // If we're on the last child for this node, save its max-key
            // for later, else push its max-key to the keys vector
            if (child_num == node_children_count - 1) {
                final_child_max = std::get<1>(child_layer_data->at(curr_child));
            } else {
                keys_vector.push_back(std::get<1>(child_layer_data->at(curr_child)));
            }
            children_vector.push_back(std::get<0>(child_layer_data->at(curr_child)));
            curr_child++;
        }

        // We do (node_children_count - 1) to get the number of delimiting keys in the node
        curr_layer.push_back(std::make_tuple(node_children_count - 1, keys_vector, children_vector));
        curr_layer_data->push_back(std::make_tuple(page_offset + node_num, final_child_max));
    }

    return curr_layer;
}

// Takes a vector of BTNodes, and returns a flat vector of numbers in the following form (per node)
// [uint_64t: # keys]|[multiple uint_64ts: keys]|[multiple uint_64ts: children]|[0-padding to fill page]
std::vector<uint64_t> Memtable::flattenInternalNodes(std::vector<BTNode>* internalNodes) {
    std::vector<uint64_t> output;

    for (size_t node_num = 0; node_num < internalNodes->size(); node_num++) {
        // Append the number of keys in the node to the output
        output.push_back(std::get<0>(internalNodes->at(node_num)));

        // Append the elements in this node's keys vector to the output
        VEC_APPEND(output, std::get<1>(internalNodes->at(node_num)));

        // Append the elements in this node's children vector to the output
        VEC_APPEND(output, std::get<2>(internalNodes->at(node_num)));

        // Pad with the number of 0s required to fill a page
        output.insert(output.end(), ENTRIES_PER_PAGE - output.size(), 0);
    }

    return output;
}

void Memtable::checkWrite(ssize_t written, int fd, ssize_t desiredWriteAmount) {
    if (written == -1) {
        close(fd);
        throw std::runtime_error(std::string("Write Failed: ") + std::strerror(errno));
    } else if (written != desiredWriteAmount) {
        close(fd);
        throw std::runtime_error("Partial Write: Not all bytes were written.");
    }
}

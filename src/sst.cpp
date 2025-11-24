#include "../include/sst.hpp"

#include <functional>

#include "../include/bloomfilter.hpp"
#include "../include/bufferpool.hpp"

// Append the elements of v2 to v1
#define VEC_APPEND(v1, v2) ((v1).insert((v1).end(), (v2).begin(), (v2).end()))

namespace SST {

int calcNumPages(size_t numKeys) {
    size_t numItems = 2 * numKeys;
    return static_cast<int>(CEIL_DIV(numItems, UINT64S_PER_PAGE));
}
// number of pages = ceil(total entries / entries in a page)
// if not last page, return entries in a page (entries in a page is actually keys in a page so we have to x2)
// if last page is not full, then return total entries % entries in a page
// if last page is full (i.e. modulo returns 0), then return entries in a page

int calcNumItemsInPage(size_t numKeys, int pageNum) {
    int currPageNum = pageNum - 1;
    size_t numItems = 2 * numKeys;
    int numPages = calcNumPages(numItems);

    if (currPageNum == numPages - 1) {
        int itemsLastPage = numItems % UINT64S_PER_PAGE;
        if (!itemsLastPage) { // 0
            return UINT64S_PER_PAGE;
        }
        else {
            return itemsLastPage;
        }
    }
    return UINT64S_PER_PAGE;
}

int binSearch(int lo, int hi, const std::function<int(int)>& comparator) {
    int mid;

    while (lo <= hi) {
        mid = lo + (hi - lo) / 2;

        int direction = comparator(mid);

        if (direction < 0) {
            hi = mid - 1;
        } else if (direction > 0) {
            lo = mid + 1;
        } else {
            break;
        }
    }

    return mid;
}

uint64_t getNextBTreeNode(uint64_t currKey, uint64_t* pageBuf) {
    uint64_t numKeysInNode = pageBuf[0];
    uint64_t startOfChildren = 1 + numKeysInNode;

    // If the key we're looking for is larger than the last delimiting
    // key, then we can just immediately go down to the rightmost child
    if (currKey > pageBuf[numKeysInNode]) {
        return pageBuf[startOfChildren + numKeysInNode];
    }

    // LINEAR SEARCH

    // // Otherwise, we know that currKey must be less than (or equal to)
    // // one of the delimiting keys in this node, which we must find
    // for (uint64_t delimKeyIdx = 0; delimKeyIdx < numKeysInNode; delimKeyIdx++) {
    //     if (currKey <= pageBuf[1 + delimKeyIdx]) {
    //         currPage = pageBuf[startOfChildren + delimKeyIdx];
    //         break;
    //     }
    // }

    // BINARY SEARCH

    // If the key we're looking for is smaller than/equall to the first delimiting
    // key, then we can just immediately go down to the leftmost child
    if (currKey <= pageBuf[1]) {
        return pageBuf[startOfChildren];
    }

    int lo = 1;                  // Corresponds to the second key (we alr. checked for left child of the first key)
    int hi = numKeysInNode - 1;  // Index of last key (we alr. checked for right child of the last key)

    // Note, pageBuf[1 + mid] is the key we're currently inspecting
    // (+1 for offset), while pageBuf[mid] is the key before it
    int mid = binSearch(lo, hi, [&](int m) {
        return (currKey <= pageBuf[m]) ? -1 : (currKey > pageBuf[1 + m]) ? 1 : 0;
    });  // Else case: pageBuf[mid] < currKey && currKey <= pageBuf[1 + mid]

    return pageBuf[startOfChildren + mid];
}



// B-Tree/Binary (depending on USE_BTREE_SEARCH) search in the SST corresponding to sstNum, to find the provided keys
// Returns a tuple of: (1) Vector of KV pairs that were found in the SST, and (2) Keys that weren't found in the SST
std::tuple<kvPairs, std::vector<uint64_t>> sstSearch(std::vector<uint64_t> keys, int sstNum, sstMetadata metadata,
                                                     bool useBTreeSearch, BufferPool* bufferPool,
                                                     std::string databaseName) {
    kvPairs foundPairs;
    std::vector<uint64_t> keysNotFound;
    std::vector<uint64_t> keysToFind;

    auto [entryCount, internalNodeCount, filterBitCount, minKey, maxKey] = metadata;

    BloomFilter filter(filterBitCount, 0);
    uint64_t filterPageCount = filter.getNumPages();
    uint64_t leafPageCount = calcNumPages(entryCount);    

    // number of pages = ceil(total entries / entries in a page)
    // if not last page, return entries in a page (entries in a page is actually keys in a page so we have to x2)
    // if last page is not full, then return total entries % entries in a page
    // if last page is full (i.e. modulo returns 0), then return entries in a page

    uint64_t filterBuf[PAGE_SIZE / sizeof(uint64_t)];
    // Initialize the bloom filter (starts at 1 to account for metadata page)
    for (size_t pageNum = 1 + leafPageCount; pageNum < 1 + leafPageCount + filterPageCount; pageNum++) {
        bufferPool->comboRead(SST_PATH(sstNum), sstNum, PAGE_SIZE * pageNum, filterBuf, PAGE_SIZE);
        filter.initFromBuf(filterBuf);
    }

    // Filter out keys that are either outside the range of this SST, or not in the bloom filter
    for (size_t i = 0; i < keys.size(); i++) {
        if (keys[i] < minKey || keys[i] > maxKey || !filter.checkKey(keys[i])) {
            keysNotFound.push_back(keys[i]);
        } else {
            keysToFind.push_back(keys[i]);
        }
    }
    
    // If we have no keys to look for in this SST, then no need to do any I/O here
    if (keysToFind.empty()) {
        return std::make_tuple(foundPairs, keysNotFound);
    }

    // Reverse our keysToFind list, since popping from the back is O(1)
    std::reverse(keysToFind.begin(), keysToFind.end());

    uint64_t currKey = keysToFind.back();
    // int fd = open(SST_PATH(sstNum).c_str(), O_RDONLY); // | O_DIRECT);

    // Variables for page reads
    uint64_t pageBuf[PAGE_SIZE / sizeof(uint64_t)];
    size_t itemsRead;

    int candidatePageNum;  // The page in which we want to look for currKey
    if (useBTreeSearch) {
        // B-Tree search to find the correct page
        uint64_t currPage = 1 + filterPageCount + leafPageCount;  // page corresponding to root node

        // Keep going until we reach a leaf node (leaf nodes start at page #(1 + leafPageCount))
        while (currPage >= 1 + leafPageCount) {
            bufferPool->comboRead(SST_PATH(sstNum), sstNum, PAGE_SIZE * currPage, pageBuf, PAGE_SIZE);
            // pread(fd, pageBuf, PAGE_SIZE, PAGE_SIZE * currPage);
            currPage = getNextBTreeNode(currKey, pageBuf);
        }
        candidatePageNum = currPage;
        bufferPool->comboRead(SST_PATH(sstNum), sstNum, PAGE_SIZE * currPage, pageBuf, PAGE_SIZE);
        // bytesRead = pread(fd, pageBuf, PAGE_SIZE, PAGE_SIZE * currPage);
        itemsRead = calcNumItemsInPage(entryCount, candidatePageNum);

    } else {
        // Binary search variables
        int lo = 1;
        int hi = leafPageCount;
        candidatePageNum = binSearch(lo, hi, [&](int m) {
            bufferPool->comboRead(SST_PATH(sstNum), sstNum, PAGE_SIZE * m, pageBuf, PAGE_SIZE);
            // page num and total items needed for helper function
            itemsRead = calcNumItemsInPage(entryCount, m);

            return (currKey < pageBuf[0]) ? -1 : (currKey > pageBuf[itemsRead - 2]) ? 1 : 0;
        });
    }

    // At this point, pageBuf should contain the correct page, corresponding to candidatePageNum

    int keysRead = itemsRead / 2; // this itemsRead is fine

    int mid = binSearch(0, keysRead - 1, [&](int m) {
        return (currKey < pageBuf[m * 2]) ? -1 : (currKey > pageBuf[m * 2]) ? 1 : 0;
    });

    // At this point, mid is either equal to the index of currKey itself,
    // or the next smallest key after currKey (if currKey wasn't found)

    uint64_t midKey;
    while (!keysToFind.empty()) {
        currKey = keysToFind.back();
        midKey = pageBuf[mid * 2];

        while (midKey < currKey) {
            mid++;

            // If mid is out of bounds, read next page and set mid to 0
            if (mid >= keysRead) {
                candidatePageNum++;
                bufferPool->comboRead(SST_PATH(sstNum), sstNum, PAGE_SIZE * candidatePageNum, pageBuf, PAGE_SIZE);
                // bytesRead = pread(fd, pageBuf, PAGE_SIZE, PAGE_SIZE * candidatePageNum);
                itemsRead = calcNumItemsInPage(entryCount, candidatePageNum);
                keysRead = itemsRead / 2;

                mid = 0;
            }

            midKey = pageBuf[mid * 2];
        }

        // Now, midKey >= currKey
        if (midKey == currKey) {
            foundPairs.push_back(std::make_tuple(midKey, pageBuf[mid * 2 + 1]));
        } else {
            keysNotFound.push_back(currKey);
        }

        keysToFind.pop_back();
    }
    return std::make_tuple(foundPairs, keysNotFound);
}

// Bloom Filter Stuff

BloomFilter constructBloomFilter(std::vector<uint64_t>* memtable_data) {
    size_t num_keys = memtable_data->size() / 2;

    BloomFilter bloom_filter(num_keys);

    for (size_t key_num = 0; key_num < num_keys; key_num++) {
        bloom_filter.addKey(memtable_data->at(key_num * 2));
    }

    return bloom_filter;
}

// B-Tree Stuff

// Constructs a layer of the B-Tree, and returns a list of BTNodes
// (also stores current layer data in the curr_layer_data variable)
std::vector<BTNode> constructLayer(
    uint64_t page_offset, uint64_t layer_size, std::vector<std::tuple<uint64_t, uint64_t>>* child_layer_data,
    std::vector<std::tuple<uint64_t, uint64_t>>* curr_layer_data  // output for list of page-nums and max-keys
) {
    std::vector<BTNode> curr_layer;  // list of nodes in the current layer

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

        if (extra_children) {  // If there're extra children, assign one to this node
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

std::vector<BTNode> constructInternalNodes(std::vector<uint64_t>* memtable_data, uint64_t num_filter_pages) {
    uint64_t num_internal_nodes = 1;  // Starts at 1 to account for the root node
    uint64_t num_leaf_nodes = CEIL_DIV(memtable_data->size(), ENTRIES_PER_PAGE);

    std::vector<uint64_t> layer_sizes = {
        CEIL_DIV(num_leaf_nodes, BRANCH_FACTOR),
    };  // the sizes of each internal layer, bottom-up

    while (layer_sizes.back() != 1) {
        num_internal_nodes += layer_sizes.back();
        layer_sizes.push_back(CEIL_DIV(layer_sizes.back(), BRANCH_FACTOR));
    }

    uint64_t page_offset = num_leaf_nodes + num_internal_nodes + num_filter_pages + 1;  // +1 to account for the metadata page

    // let's make a tuple to represent each leaf page, which will just
    // contain the page number (including offset) and the max key in the page
    std::vector<std::tuple<uint64_t, uint64_t>> leaf_pages;

    for (uint64_t leaf_num = 0; leaf_num < num_leaf_nodes - 1; leaf_num++) {
        // Start with the index for the *beginning* of the *next* page,
        // then subtract 2 to obtain the index of the *end* of *this* page
        uint64_t last_idx_in_page = ENTRIES_PER_PAGE * (leaf_num + 1) - 2;

        leaf_pages.push_back(std::make_tuple(1 + leaf_num, memtable_data->at(last_idx_in_page)));
    }

    // add the very last page, and the very last key in memdata_table
    leaf_pages.push_back(
        std::make_tuple(1 + num_leaf_nodes - 1, memtable_data->at(memtable_data->size() - 2)));

    std::vector<BTNode> finalNodeVec;  // A vector to hold our final output

    std::vector<std::tuple<uint64_t, uint64_t>> child_layer_data = leaf_pages;  // list of page-nums and max-keys
    std::vector<std::tuple<uint64_t, uint64_t>> curr_layer_data;                // list of page-nums and max-keys
    std::vector<BTNode> curr_layer;                                             // list of nodes in the current layer

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

// Takes a vector of BTNodes, and returns a flat vector of numbers in the following form (per node)
// [uint_64t: # keys]|[multiple uint_64ts: keys]|[multiple uint_64ts: children]|[0-padding to fill page]
std::vector<uint64_t> flattenInternalNodes(std::vector<BTNode>* internalNodes) {
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

void checkWrite(ssize_t written, int fd, ssize_t desiredWriteAmount) {
    if (written == -1) {
        close(fd);
        throw std::runtime_error(std::string("Write Failed: ") + std::strerror(errno));
    } else if (written != desiredWriteAmount) {
        close(fd);
        throw std::runtime_error("Partial Write: Not all bytes were written.");
    }
}

sstMetadata sstWrite(std::string filename, std::vector<uint64_t>* memtable_data, size_t flushed_size) {
    // ========== Metadata Stuff Begins ==========

    // Metadata values
    uint64_t min = memtable_data->at(0);
    uint64_t max = memtable_data->at(memtable_data->size() - 2);
    size_t num_internal_nodes;

    // Construct the bloom filter, and flatten it for writing to disk
    BloomFilter filter = constructBloomFilter(memtable_data);
    std::vector<uint64_t> filter_data = filter.flattenBloomFilter();
    uint64_t filter_bits = filter.getTotalBits();
    size_t filter_bytes = filter_data.size() * sizeof(uint64_t);
    // Construct the internal nodes of the B-Tree, and flatten them for writing to disk
    std::vector<BTNode> internal_nodes = constructInternalNodes(memtable_data, CEIL_DIV(filter_bytes, PAGE_SIZE));
    std::vector<uint64_t> internal_data = flattenInternalNodes(&internal_nodes);

    num_internal_nodes = internal_nodes.size();

    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        close(fd);
        throw std::runtime_error(std::string("open failed: ") + std::strerror(errno));
    }

    ssize_t written = write(fd, &flushed_size, sizeof(size_t));
    checkWrite(written, fd, sizeof(size_t));

    written = write(fd, &num_internal_nodes, sizeof(size_t));
    checkWrite(written, fd, sizeof(size_t));

    written = write(fd, &filter_bits, sizeof(uint64_t));
    checkWrite(written, fd, sizeof(uint64_t));

    written = write(fd, &min, sizeof(uint64_t));
    checkWrite(written, fd, sizeof(uint64_t));

    written = write(fd, &max, sizeof(uint64_t));
    checkWrite(written, fd, sizeof(uint64_t));

    size_t header_bytes = 2 * sizeof(size_t) + 3 * sizeof(uint64_t);
    size_t padding = PAGE_SIZE - header_bytes;
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

    written = write(fd, memtable_data->data(), memtable_data->size() * sizeof(uint64_t));
    checkWrite(written, fd, memtable_data->size() * sizeof(uint64_t));
    size_t memtable_data_bytes = memtable_data->size() * sizeof(uint64_t);
    
    if (memtable_data_bytes % PAGE_SIZE) {
        size_t memtable_data_padding = PAGE_SIZE - (memtable_data_bytes % PAGE_SIZE);
        std::vector<char> memtable_zero_buf(memtable_data_padding, 0);
        written = write(fd, memtable_zero_buf.data(), memtable_data_padding);
        checkWrite(written, fd, memtable_data_padding);
    }

    written = write(fd, filter_data.data(), filter_data.size() * sizeof(uint64_t));
    checkWrite(written, fd, filter_data.size() * sizeof(uint64_t));

    if (filter_bytes % PAGE_SIZE) {  // If we don't nicely fill out a page, pad it with 0s
        size_t filter_padding = PAGE_SIZE - (filter_bytes % PAGE_SIZE);
        std::vector<char> filter_zero_buf(filter_padding, 0);
        written = write(fd, filter_zero_buf.data(), filter_padding);
        checkWrite(written, fd, filter_padding);
    }

    written = write(fd, internal_data.data(), internal_data.size() * sizeof(uint64_t));
    checkWrite(written, fd, internal_data.size() * sizeof(uint64_t));

    close(fd);
    return std::make_tuple(flushed_size, num_internal_nodes, filter.getTotalBits(), min, max);
}

}  // namespace SST


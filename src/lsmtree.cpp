#include "../include/lsmtree.hpp"

/**
 * @brief Constructor for the LSMTree class
 */

LSMTree::LSMTree(int numLevelsValue, int scaleFactorValue) : numLevels(numLevelsValue), scaleFactor(scaleFactorValue) {}

/**
 * @brief Destructor for the LSMTree class
 */

LSMTree::~LSMTree() {}

int compaction(int sstNum1, int sstNum2) {}

void LSMTree::multiwayMergeSort(int sstNum1, int sstNum2) {
    
}
#pragma once

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "bloomfilter.hpp"
#include "constants.hpp"
#include "sst.hpp"

class LSMTree {
    public:
        LSMTree(int numLevelsValue, int scaleFactorValue);
        ~LSMTree();
        int numLevels;
        int scaleFactor; // basically M (needs to be a constant after)
    
    int compaction(int sstNum1, int sstNum2);
    void multiwayMergeSort(int sstNum1, int sstNum2);
    void constructInternalNodes();
    void constructLayer();

};
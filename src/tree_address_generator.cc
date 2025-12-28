#include "cache.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <fmt/core.h>

#include "bandwidth.h"
#include "champsim.h"
#include "chrono.h"
#include "deadlock.h"
#include "instruction.h"
#include "util/algorithm.h"
#include "util/bits.h"
#include "util/span.h"
#include "tree_config.h"
#include "environment.h"



std::vector<uint64_t> generate_tree_addresses(champsim::address address_in){

    //fmt::print("data memory size {} \n",tree::DATA_MEM_SIZE);
    fmt::print("Generating tree addresses \n");
    uint64_t address = address_in.to<uint64_t>();
    uint64_t root_index = (tree::IND_TREE_SIZE != 0) ? ((address - tree::MEMORY_START_ADDR) / tree::IND_TREE_SIZE) : ((address - tree::MEMORY_START_ADDR)) ;
    uint64_t tree_offset = root_index * tree::IND_TREE_SIZE;
    uint64_t dataNodeNum = (address - tree::MEMORY_START_ADDR -tree_offset)/tree::BLOCK_SIZE;
    uint64_t treeNodeAddr = 0;
    uint64_t tree_level_address = 0;
    uint64_t shift = 0;
    uint64_t node_index = 0;
    uint64_t TreeNodeOffset;
    std::vector<uint64_t> counter_addresses(tree::MAX_LEVEL);

    for (int current_level = tree::MAX_LEVEL-1; current_level>=0; current_level-- ){
        if(current_level == 0){
            treeNodeAddr = tree::TREE_START_ADDRESSES[root_index];
            tree_level_address = tree::TREE_START_ADDRESSES[root_index];
        }
        else{
            shift = tree::TREE_ARITY_BITS * (tree::MAX_LEVEL - 1 - current_level);
            node_index = dataNodeNum >> shift;
            TreeNodeOffset = node_index * tree::TREE_DATA_SIZE;
            tree_level_address = tree_level_address + (tree::TREE_DATA_SIZE<<(tree::TREE_ARITY_BITS*current_level));
            treeNodeAddr = tree_level_address + TreeNodeOffset;
        }
        counter_addresses[current_level] = treeNodeAddr;
    }
    return counter_addresses;
}
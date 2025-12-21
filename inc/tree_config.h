#ifndef TREE_CONFIG_H
#define TREE_CONFIG_H
#include <bit>
#include <cmath>
#include <environment.h>
#include "champsim.h"
#include "cache.h"
#include <fmt/core.h>

extern champsim::environment* global_environment;

namespace tree{
    //Set Parameters
    const int TREE_ARITY = 8;
    const int TREE_ROOTS = 64;
    const int MEMORY_START_ADDR = 0;
    const int TREE_DATA_SIZE = 8; //Bytes
    
    const int BLOCK_SIZE = 64;

    //To be calculate parameters
    inline uint64_t DATA_MEM_SIZE = 0;
    inline MEMORY_CONTROLLER* dram_ptr;
    inline uint8_t TREE_ARITY_BITS = 0;
    inline uint64_t NUM_DATA_BLOCKS = 0;
    inline uint8_t MAX_LEVEL = 0;
    inline uint64_t TREE_SIZE = 0;
    inline uint64_t IND_TREE_SIZE = 0;
    inline std::vector<uint64_t> TREE_START_ADDRESSES;
    inline uint64_t TREE_SIZE_PART;
    inline std::vector<uint64_t> init_tree_addresses(uint64_t tree_size) {
            std::vector<uint64_t> addresses(TREE_ROOTS);
            addresses[0] = MEMORY_START_ADDR + DATA_MEM_SIZE;
            uint64_t vLastStart = addresses[0];
            
            for (int x = 1; x < TREE_ROOTS; x++) {
                vLastStart = vLastStart + tree_size;
                addresses[x] = vLastStart;
            }
            
            return addresses;
        }
    inline void initialize(){
        dram_ptr = &global_environment->dram_view();
        auto& mapping = dram_ptr->channels[0].address_mapping;
        auto rows = mapping.rows();
        auto columns = mapping.columns();
        auto banks = mapping.banks();
        auto bank_groups = mapping.bankgroups();
        auto ranks = mapping.ranks();
        auto channels = mapping.channels();
        uint64_t channel_width = static_cast<uint64_t>(dram_ptr->channels[0].channel_width.count());
        DATA_MEM_SIZE = rows * columns * banks * bank_groups * ranks * channels * channel_width;
        fmt::print("DATA_MEM_SIZE: {} \n", DATA_MEM_SIZE);
        //Calculated Parameters
        TREE_ARITY_BITS = static_cast<uint8_t>(std::floor(std::log2((TREE_ARITY))));
        fmt::print("tree arity bits: {} \n", TREE_ARITY_BITS);
        NUM_DATA_BLOCKS = DATA_MEM_SIZE / BLOCK_SIZE;
        fmt::print("NUM_DATA_BLOCKS: {} \n", NUM_DATA_BLOCKS);
        MAX_LEVEL = static_cast<uint8_t>((std::ceil(std::log2(NUM_DATA_BLOCKS / TREE_ROOTS)) + TREE_ARITY_BITS - 1) / std::ceil(std::log2(TREE_ARITY)));

        TREE_SIZE = static_cast<uint64_t>(((pow(TREE_ARITY, MAX_LEVEL) - 1) / (TREE_ARITY - 1)) * TREE_ARITY * TREE_DATA_SIZE);
        TREE_SIZE_PART = static_cast<uint64_t>(pow(TREE_ARITY, MAX_LEVEL) - 1);
        fmt::print("TREE_SIZE_PART: {} \n", TREE_SIZE_PART);
        fmt::print("MAX_LEVEL: {} \n", MAX_LEVEL);
        // Function to initialize tree addresses
        
        
        TREE_START_ADDRESSES = init_tree_addresses(TREE_SIZE);
        IND_TREE_SIZE = DATA_MEM_SIZE / TREE_ROOTS;
    }

}

#endif
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

class EvictionLog {
private:
    struct Event {
        uint64_t evicted_address;
        long set;
        uint64_t way;
        uint64_t way1;
        uint64_t way2;
        uint64_t way3;
        uint64_t way4;
        uint64_t way5;
        uint64_t way6;
        uint64_t way7;
        std::vector<int64_t> state;
        int evicted_level;
    };
    
    std::vector<Event> rereference_list;
    bool write_to_log;
    uint64_t instructions_number;
    const uint64_t INVALID = -1;
    const long NUM_WAY = 8;
public:
    EvictionLog(bool log_enabled = false) 
        : write_to_log(log_enabled), instructions_number(0) {}


    void add_event(champsim::address victim_address, long set, long way, const std::vector<int64_t>& state, CACHE* cache_ptr, int evicted_level) {
        if (!write_to_log) return;
        auto shamt = cache_ptr->OFFSET_BITS;
        auto victim_address_sliced = victim_address.slice_upper(shamt).to<uint64_t>();
        Event event;
        event.evicted_address = victim_address_sliced;
        event.set = set;
        long block_base = set * NUM_WAY;
        event.way = cache_ptr->block[block_base + way].address.slice_upper(shamt).to<uint64_t>(); 
        
        // Extract addresses from each way
        uint64_t other_ways[7] = {INVALID, INVALID, INVALID, INVALID,INVALID, INVALID, INVALID};
        int idx = 0;
        
        for (long w = 0; w < NUM_WAY; ++w) {
            if (w != way && idx < 7) {
                long block_idx = block_base + w;
                
                // Access block directly using array index
                auto& current_block = cache_ptr->block[block_idx];
                
                if (current_block.valid) {
                    other_ways[idx++] = current_block.address.slice_upper(shamt).to<uint64_t>();
                }
                else {
                    other_ways[idx++] = INVALID;
                }
            }
        }
        
        event.way1 = other_ways[0];
        event.way2 = other_ways[1];
        event.way3 = other_ways[2];
        event.way4 = other_ways[3];
        event.way5 = other_ways[4];
        event.way6 = other_ways[5];
        event.way7 = other_ways[6];
        event.state = state;
        event.evicted_level = evicted_level;
        
        rereference_list.push_back(event);
    }
    
    void resolve(champsim::address address, CACHE* cache_ptr) {
        if (!write_to_log) return;
        auto it = rereference_list.begin();
        auto shamt = cache_ptr->OFFSET_BITS;
        auto address_sliced = address.slice_upper(shamt).to<uint64_t>();
        while (it != rereference_list.end()) {
            if(address_sliced == it->way1) it->way1 = INVALID;
            if(address_sliced == it->way2) it->way2 = INVALID;
            if(address_sliced == it->way3) it->way3 = INVALID;
            if(address_sliced == it->way4) it->way4 = INVALID;
            if(address_sliced == it->way5) it->way5 = INVALID;
            if(address_sliced == it->way6) it->way6 = INVALID;
            if(address_sliced == it->way7) it->way7 = INVALID;
            if(address_sliced == it->way) it->way = INVALID;
            // Bad eviction - evicted location was rereferenced
            if (address_sliced == it->evicted_address) {
                if (write_to_log) {
                    log_bad_eviction(it->way, it->evicted_address, it->state, it->evicted_level);
                }
                    it = rereference_list.erase(it); 
                } 
                // Good eviction - all ways referenced
                else if (it->way1 == INVALID && it->way2 == INVALID && it->way3 == INVALID && 
                        it->way4 == INVALID && it->way5 == INVALID && it->way6 == INVALID && 
                        it->way7 == INVALID) {
                    if (write_to_log) {
                        log_good_eviction(it->way, it->evicted_address, it->state, it->evicted_level);
                    }
                    it = rereference_list.erase(it);  
                }
                else {
                    ++it;
                }
            }
    }
    
    void set_instruction_number(uint64_t instr_num) {
        instructions_number = instr_num;
    }
    
    void enable_logging(bool enable) {
        write_to_log = enable;
    }

private:
    void log_bad_eviction(uint64_t inserted_address, uint64_t evicted_address, const std::vector<int64_t> state, int evicted_level) {
        
        // CSV log with label 0 (bad eviction)
        log_to_csv(inserted_address, evicted_address, state, 0, evicted_level);
    }
    
    void log_good_eviction(uint64_t inserted_address, uint64_t evicted_address, const std::vector<int64_t> state, int evicted_level) {
        // CSV log with label 1 (good eviction)
        log_to_csv(inserted_address, evicted_address, state, 1, evicted_level);
    }
    
    void log_to_csv(uint64_t inserted_address, uint64_t evicted_address, const std::vector<int64_t> state, int label, int evicted_level) {
        // Opens or creates a log.csv file in append mode
        std::ofstream csv_file("log.csv", std::ios::app);
        
        // Write label as first column
        csv_file << label;
        csv_file << ",";
        csv_file << inserted_address;
        csv_file << ",";
        csv_file << evicted_address;
        csv_file << ",";
        csv_file << evicted_level;
        
        // Write state
        for (size_t i = 0; i < state.size(); ++i) {
            csv_file << "," << state[i];
        }
        // Start a new row
        csv_file << "\n";
        csv_file.close();
    }
};

// Global instance
inline EvictionLog rr_log;
inline std::vector<int64_t> levels_counter(9*64,0);
#ifndef REPLACEMENT_MODIFIED_LRU_H
#define REPLACEMENT_MODIFIED_LRU_H

#include <vector>

#include "cache.h"


class modified_lru : public champsim::modules::replacement
{
  long NUM_WAY;
  std::vector<uint64_t> last_used_cycles;
  uint64_t cycle = 0;
  std::vector<int64_t> levels_counter;
  int& get_line_level(long set, long way);
public:
  std::vector<int> line_level;
  explicit modified_lru(CACHE* cache);
  modified_lru(CACHE* cache, long sets, long ways);
  
  // void initialize_replacement();
  long find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                   champsim::address full_addr, access_type type, int8_t node_level);
  void replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                              access_type type, int8_t node_level);
  void update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                                access_type type, uint8_t hit, int8_t node_level);
  // void replacement_final_stats()
};

#endif

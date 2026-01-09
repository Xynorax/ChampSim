#include "modified_lru.h"

#include <algorithm>
#include <cassert>

modified_lru::modified_lru(CACHE* cache) : modified_lru(cache, cache->NUM_SET, cache->NUM_WAY) {} // Delegating constructor, delegates all the init to the second constructor

modified_lru::modified_lru(CACHE* cache, long sets, long ways) : replacement(cache), NUM_WAY(ways), last_used_cycles(static_cast<std::size_t>(sets * ways), 0), levels_counter(9, 0),
line_level(static_cast<std::size_t>(sets * ways), 0) {}
int& modified_lru::get_line_level(long set, long way) { return line_level.at(static_cast<std::size_t>(set * NUM_WAY + way)); }

// Called when set is full
long modified_lru::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                      champsim::address full_addr, access_type type, int8_t node_level)
{
  //fmt::print("find_victim");
  auto begin = std::next(std::begin(last_used_cycles), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);

  // Find the way whose last use cycle is most distant
  auto victim = std::min_element(begin, end);
  long victim_way = std::distance(begin, victim);
  get_line_level(set, victim_way) = node_level;
  assert(begin <= victim);
  assert(victim < end);
  return std::distance(begin, victim);
}


// Called when there is an eviction
void modified_lru::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,access_type type, int8_t node_level)
{
  //fmt::print("replacement_cache_fill called");
  std::vector<std::pair<int8_t, uint64_t>> level_rankings;
  for (int8_t i = 0; i < static_cast<int8_t>(levels_counter.size()); ++i) {
    level_rankings.push_back({i, levels_counter[i]});
  }
  auto begin = std::next(std::begin(line_level), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);

  for (auto it = begin; it != end; ++it)
    if (levels_counter[*it] < levels_counter[node_level]) {
      last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = last_used_cycles.at((std::size_t)(set * NUM_WAY + std::distance(begin, it)));
      last_used_cycles.at((std::size_t)(set * NUM_WAY + way))++;
    }

  
}

// Called on a hit
void modified_lru::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                   champsim::address victim_addr, access_type type, uint8_t hit, int8_t node_level = 8)
{
  // Mark the way as being used on the current cycle
  /*for (std::size_t i = 0; i < levels_counter.size(); i++) {
        if (i != static_cast<std::size_t>(node_level)) {
            fmt::print("{}",levels_counter[i] );
            
        }
    }*/
  
  if (hit && access_type{type} != access_type::WRITE) { // Skip this for writeback hits 
    last_used_cycles.at((std::size_t)(set * NUM_WAY + way)) = cycle++;
    levels_counter[node_level] = levels_counter[node_level] + 3;
    // Increment all levels except node_level
    for (std::size_t i = 0; i < levels_counter.size(); i++) {
        if (i != static_cast<std::size_t>(node_level)) {
          if (levels_counter[i] >0) {
            levels_counter[i] = levels_counter[i] - 1;
          }
            
        }
    }
  }
    
}

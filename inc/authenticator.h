#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include <cstdint>
#include <deque>
#include "champsim.h"
#include "chrono.h"
#include "tree_config.h"
#include "cache.h"
#include <fmt/core.h>

namespace champsim {
    constexpr int AUTHENTICATION_QUEUE_SIZE = 32;
    constexpr int AUTHENTICATION_LATENCY = 20;

class Authenticator {
public:
  

    struct tree_node {
    champsim::address address;
    bool cached = false;
    bool ready = false;
    };

    struct auth_entry {
        bool authenticated = false; 
        champsim::address llc_address;
        std::vector<tree_node> tree_levels;
        bool ready = false;
        uint8_t cached_level = tree::MAX_LEVEL;
        champsim::chrono::clock::time_point auth_compl_time;
    };
    std::deque<auth_entry> authentication_queue;
    void add_entry(champsim::address llc_address, bool authenticated = false) {
        auth_entry new_entry;
        new_entry.llc_address = llc_address;
        new_entry.authenticated = authenticated;
        new_entry.cached_level = tree::MAX_LEVEL;
        new_entry.ready = false;
        new_entry.tree_levels.resize(tree::MAX_LEVEL);
        authentication_queue.push_back(new_entry);
        fmt::print("entry added to authentication queue \n");
        fmt::print("{} \n",new_entry.cached_level);
        fmt::print("{} \n",new_entry.ready);

    };

    void add_tree_node(champsim::address llc_address, champsim::address tree_node_address, int8_t current_level, bool cached = false) {
        for (auto& entry : authentication_queue) {
            if (entry.llc_address == llc_address) {
                entry.tree_levels[current_level].address = tree_node_address;
                entry.tree_levels[current_level].cached = cached;
                fmt::print("added tree node to authentication queue entry {} \n",llc_address);
                fmt::print("{} \n",entry.cached_level);
                fmt::print("{} \n",entry.ready);
                if (cached) {
                    entry.cached_level = current_level;
                }
            }
        }
        
    }
    void update_auth_timer(champsim::chrono::clock::time_point current_time) {
       for (auto& entry : authentication_queue) {
            if (current_time >= entry.auth_compl_time) {
                entry.authenticated = true;
            }
        } 
    }
    void start_authentication(champsim::address llc_address, champsim::chrono::clock::time_point current_time, champsim::chrono::picoseconds clock_period) {
        for (auto& entry : authentication_queue) {
            if (entry.llc_address == llc_address) {
                entry.auth_compl_time = current_time + (AUTHENTICATION_LATENCY * clock_period);
            }
        }
    }

    void remove_entry(champsim::address llc_address) {
        auto it = authentication_queue.begin();
        while (it != authentication_queue.end()) {
            if (it ->llc_address == llc_address) {
                it = authentication_queue.erase(it);
                break;
            }
            else {
                ++it;
            }
        }
    }

    bool check_authenticated(champsim::address llc_address) {
        for (const auto& entry : authentication_queue) {
            if (entry.llc_address == llc_address) {
                return entry.authenticated;
            }
        }
        return false;
    }
    bool is_ready_for_authentication(champsim::address llc_address) {
        fmt::print("Checking {} if ready for authentication \n",llc_address);
        //fmt::print("Authentication queue size: {}",authentication_queue.size());
        for (const auto& entry : authentication_queue) {
            for (int j = tree::MAX_LEVEL - 1; j >= 0; j--) {
                    fmt::print("{} \n",entry.tree_levels[j].ready);
                }
            if (entry.llc_address == llc_address and entry.ready) {
                return true;
            }
        }
        return false;
    }

    void set_node_ready(champsim::address llc_address, champsim::address node_address, int8_t current_level) {
        for (auto& entry : authentication_queue) {

            if (entry.llc_address == llc_address and !entry.ready) {
                fmt::print("LLC address match in set_node_ready, input node address; {} \n", node_address);
                for (int j = 0; j < tree::MAX_LEVEL; j++) {
                    fmt::print("Node {} address: {}", j, entry.tree_levels[j].address);
                    if (entry.tree_levels[j].address == node_address) {
                        entry.tree_levels[j].ready = true;
                    }
                }
                for (int j = tree::MAX_LEVEL - 1; j >= 0; j--) {
                    fmt::print("set_node_ready function ready: {} \n",entry.tree_levels[j].ready);
                }
                for (int j = tree::MAX_LEVEL - 2; j > 0; j--) {
                    if (entry.tree_levels[j].ready == false) {
                        fmt::print("Node {} is not ready", j);
                        break;
                    }
                    fmt::print("Node {} is ready", j);
                    if (j == entry.cached_level || j == 1) {
                        fmt::print("entry {} set to ready for authentication! \n", llc_address);
                        entry.ready = true;
                    }
                }
            }
        }
        
    };
    
    
private:

};

} // namespace champsim

inline champsim::Authenticator authenticator;


#endif

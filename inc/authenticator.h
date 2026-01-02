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
    auto matches_address(champsim::address addr) const {
    return [match = addr](const auto& entry) {
        return entry.llc_address == match;
    };
    }
    void debug_print_auth_queue() {
        fmt::print("Current queue: \n");
        for (const auto& debug_entry : authentication_queue) {
            champsim::address llc_addr = debug_entry.llc_address;
            fmt::print("{:#x}\n", llc_addr.to<uint64_t>());
            fmt::print("In progress: {} ", debug_entry.authentication_in_progress);
            fmt::print("Ready for auth: {} ", debug_entry.ready);
            fmt::print("Cached level: {} ", debug_entry.cached_level);
            fmt::print("Authenticated: {}\n", debug_entry.authenticated);
        }
    }

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
        bool authentication_in_progress;
    };
    std::deque<auth_entry> authentication_queue;
    void add_entry(champsim::address llc_address,std::vector<uint64_t>tree_addresses, bool authenticated = false) {
        llc_address = champsim::address(tree::shift_address(llc_address.to<uint64_t>()));
        auto entry = std::find_if(std::begin(authentication_queue), std::end(authentication_queue), 
                                   matches_address(llc_address));
        if (entry != authentication_queue.end()) {
        fmt::print(" DUPLICATE FOUND - not adding\n");
        assert(0);
        return;  // Don't add another entry
        }
        auth_entry new_entry;
        new_entry.llc_address = llc_address;
        new_entry.authenticated = authenticated;
        new_entry.cached_level = tree::MAX_LEVEL;
        new_entry.ready = false;
        new_entry.tree_levels.resize(tree::MAX_LEVEL);
        new_entry.authentication_in_progress = false;
        for (int j = 0; j < tree::MAX_LEVEL; j++) {
            champsim::address tree_node_address = champsim::address(tree::shift_address(tree_addresses[j]));
            new_entry.tree_levels[j].address = tree_node_address;
        }
        authentication_queue.push_back(new_entry);
        fmt::print("entry added to authentication queue,  \n");
        debug_print_auth_queue();
        //fmt::print("{} \n",new_entry.cached_level);
        //fmt::print("{} \n",new_entry.ready);

    };

    void cache_tree_node(champsim::address llc_address, champsim::address tree_node_address, int8_t current_level) {
        llc_address = champsim::address(tree::shift_address(llc_address.to<uint64_t>()));
        tree_node_address = champsim::address(tree::shift_address(tree_node_address.to<uint64_t>()));
        fmt::print("Caching tree node of level: {}", current_level);
        for (auto& entry : authentication_queue) {
            fmt::print("Entry: {}", entry.llc_address );
            for (int j = tree::MAX_LEVEL-1; j>=0 ; j--) {
                fmt::print("Node {} address: {} ", j, entry.tree_levels[j].address);
                if (entry.cached_level == j) break; // Dont set another cached level if already set
                if (entry.tree_levels[j].address == tree_node_address) {
                    fmt::print("Tree node hit! \n");
                    entry.cached_level = current_level;
                    entry.tree_levels[current_level].ready = true;
                    break;
                }
                
            }
            fmt::print("Cached level: {} \n",entry.cached_level);
            fmt::print("Entry ready: {} \n",entry.ready);
            
        }

    }
        
    void update_auth_timer(champsim::chrono::clock::time_point current_time) {
       for (auto& entry : authentication_queue) {
            if (!entry.authentication_in_progress) continue;
            if (current_time >= entry.auth_compl_time) {
                entry.authenticated = true;
            }
        } 
    }
    void start_authentication(champsim::address llc_address, champsim::chrono::clock::time_point current_time, champsim::chrono::picoseconds clock_period) {
        llc_address = champsim::address(tree::shift_address(llc_address.to<uint64_t>()));
        for (auto& entry : authentication_queue) {
            if (entry.llc_address == llc_address) {
                entry.authentication_in_progress = true;
                entry.auth_compl_time = current_time + (AUTHENTICATION_LATENCY * clock_period);
            }
        }
    }

    void remove_entry(champsim::address llc_address) {
        llc_address = champsim::address(tree::shift_address(llc_address.to<uint64_t>()));
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
        llc_address = champsim::address(tree::shift_address(llc_address.to<uint64_t>()));
        for (const auto& entry : authentication_queue) {
            if (entry.llc_address == llc_address) {
                return entry.authenticated;
            }
        }
        return true;
    }
    bool is_ready_for_authentication(champsim::address llc_address) {
        llc_address = champsim::address(tree::shift_address(llc_address.to<uint64_t>()));
        //fmt::print("Authentication queue size: {}",authentication_queue.size());
        for (const auto& entry : authentication_queue) {
            for (int j = tree::MAX_LEVEL - 1; j >= 0; j--) {
                    //fmt::print("{} \n",entry.tree_levels[j].ready);
                }
            if (entry.llc_address == llc_address and entry.ready) {
                return true;
            }
        }
        return false;
    }
    bool is_authentication_in_progress(champsim::address llc_address) {
        llc_address = champsim::address(tree::shift_address(llc_address.to<uint64_t>()));
        for (const auto& entry : authentication_queue) {
            if (entry.llc_address == llc_address and entry.authentication_in_progress) {
                return true;
            }
        }
        fmt::print("Entry {} not in progress",llc_address );
        return false;

    }
    void set_node_ready(champsim::address llc_address, champsim::address node_address) {
        llc_address = champsim::address(tree::shift_address(llc_address.to<uint64_t>()));
        node_address = champsim::address(tree::shift_address(node_address.to<uint64_t>()));
        for (auto& entry : authentication_queue) {

            if (!entry.ready) {
                
                for (int j = 0; j < tree::MAX_LEVEL; j++) {
                    fmt::print("Node {} address: {} ", j, entry.tree_levels[j].address);
                    if (entry.tree_levels[j].address == node_address) {
                        fmt::print("LLC address {} match in set_node_ready, input node address; {} \n",entry.llc_address, node_address);
                        entry.tree_levels[j].ready = true;
                    }
                }
                for (int j = tree::MAX_LEVEL - 1; j >= 0; j--) {
                    fmt::print("set_node_ready function ready: {} \n",entry.tree_levels[j].ready);
                }
                for (int j = tree::MAX_LEVEL - 1; j >= 0; j--) {
                    if (entry.tree_levels[j].ready == false) {
                        fmt::print("Node {} is not ready", j);
                        break;
                    }
                    fmt::print("Node {} is ready ", j);
                    if (j == entry.cached_level || j == 0) {
                        fmt::print("entry {} set to ready for authentication! \n", entry.llc_address);
                        entry.ready = true;
                        debug_print_auth_queue();
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

#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include <cstdint>
#include <deque>
#include "champsim.h"
#include "chrono.h"
#include "tree_config.h"

namespace champsim {
    constexpr int AUTHENTICATION_QUEUE_SIZE = 32;

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
        champsim::Authenticator::auth_entry new_entry;
        new_entry.llc_address = llc_address;
        new_entry.authenticated = authenticated;
        this->authentication_queue.push_back(new_entry);

    };

    void add_tree_node(champsim::address llc_address, champsim::address tree_node_address, int8_t current_level, bool cached = false) {
        for (int idx = 0; idx < AUTHENTICATION_QUEUE_SIZE; idx++) {
            if (this->authentication_queue[idx].llc_address == llc_address) {
                this->authentication_queue[idx].tree_levels[current_level].address = tree_node_address;
                cached = cached;
                if (cached) {
                    this->authentication_queue[idx].cached_level = current_level;
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
    void start_authentication(champsim::address llc_address, champsim::chrono::clock::time_point current_time) {
        for (auto& entry : authentication_queue) {
            if (entry.llc_address == llc_address) {
                entry.auth_compl_time = current_time;
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
    bool ready_for_authentication(champsim::address llc_address) {
        for (const auto& entry : authentication_queue) {
            if (entry.llc_address == llc_address and entry.ready) {
                return true;
            }
        }
        return false;
    }

    void set_node_ready(champsim::address llc_address, champsim::address node_address) {
        for (int idx = 0; idx < AUTHENTICATION_QUEUE_SIZE; idx++) {
            if (this->authentication_queue[idx].llc_address == llc_address and this->authentication_queue[idx].ready) {
                for (int j = 0; j < tree::MAX_LEVEL; j++) {
                    if (this->authentication_queue[idx].tree_levels[j].address == node_address) {
                        this->authentication_queue[idx].tree_levels[j].ready = true;
                    }
                }
                for (int j = tree::MAX_LEVEL; j >= this->authentication_queue[idx].cached_level; j--) {
                    if (this->authentication_queue[idx].tree_levels[j].ready == false) {
                        break;
                    }
                    if (j == this->authentication_queue[idx].cached_level) {
                        this->authentication_queue[idx].ready = true;
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

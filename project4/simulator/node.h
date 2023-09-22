#ifndef NODE_H
#define NODE_H

#include <list>

#include "Directory_Controller.h"
#include "cache.h"
#include "cpu.h"
#include "network.h"
#include "request.h"
#include "settings.h"
#include "sim.h"
#include "types.h"

class CPU;
class Cache;
class Network;
class DirectoryController;

class Node {
   public:
    Node(bool isDir, nodeID_t id, Network* ntwk);  // if its a directory, don't initialize the cpu/cache
    ~Node();

    nodeID_t id;
    bool isDir;
    CPU* cpu = NULL;  // needed to tick the cpu
    Cache* cache = NULL;
    Network* ntwk = NULL;
    DirectoryController* dir = NULL;
    bool done = false;

    std::list<Request*> ntwk_out_next;  // how the agents communicate with ntwk
                                        // tick: cache sets this other nodes do too
                                        // tock: move all to ntwk_out_queue

    // dir_tick: directory sets this, other nodes do too
    // dir_tock: move all to ntwk_out_queue

    std::list<Request*> ntwk_out_queue;  // ntwk movement infrastructure
                                         // tick: move 1 towards its dest.
                                         // if next is dest, place in ntwk_in_next
                                         // else place in ntwk_out_next
    // tock: set by out_next

    // dir_tick: move 1 towards its dest
    // dir_tock: set by out_next

    std::list<Request*> ntwk_in_queue;  // how the ntwk communicates with agent
                                        // tick: move 1 into cache->ntwk_in_next
                                        // tock: set by in_next

    // dir_tick: move 1 into dir->request_next
    // dir_tock: set by in_next

    std::list<Request*> ntwk_in_next;  // how to network talks to this node
                                       // tick: set by other nodes
                                       // tock: move all to ntwk_in_queue

    // dir_tick: set by other nodes
    // dir_tock: move all to ntwk_in_queue

    bool isDone();  // is the cpu done?
    void tick();    // progress cpu, cache, then self
    void tock();    // progress cpu, cache, then self
};

#endif  // NODE_H

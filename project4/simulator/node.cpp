#include "node.h"

Node::Node(bool isDir, nodeID_t id, Network *ntwk) {
    this->id = id;
    this->isDir = isDir;
    if (isDir) {
        dir = new DirectoryController(id);
        dir->my_node = this;
    } else {
        switch (settings.cputype) {
            case FICI_CPU:
                cpu = new CPU(id);
                break;
            default:
                cpu = NULL;
        }
        if (cpu == NULL) {
            fatal_error("Invalid CPU Type: %s\n", cputype_str[settings.cputype]);
        }
        cache = new Cache(id);
        cpu->my_cache = cache;
        cache->my_cpu = cpu;
        cache->my_node = this;
    }
    this->ntwk = ntwk;
}

Node::~Node() {
    if (cpu) delete cpu;
    if (cache) delete cache;
    if (dir) delete dir;
}

bool Node::isDone() {
    done = cpu ? cpu->isDone() : true;
    return done;
}

void Node::tick() {
    if (!isDone()) debug_printf("Tick for node %lu\n", id);
    if (!isDir) {
        cpu->tick();
        cache->tick();
    } else {
        dir->tick();
    }

    // ntwk out queue processes one item per cycle
    if (!ntwk_out_queue.empty()) {
        Request *r = ntwk_out_queue.front();
        ntwk_out_queue.pop_front();
        // RING topology hard coded
        if (settings.nettype == RING_TOP) {
            uint64_t distance = (r->destID + ntwk->num_nodes - id) % ntwk->num_nodes;
            nodeID_t target =
                (((distance > ntwk->num_nodes / 2) ? id - 1 : id + 1) + ntwk->num_nodes) % ntwk->num_nodes;
            if (target == r->destID) {
                // place in ntwk_in_next
                ntwk->nodes[target]->ntwk_in_next.push_back(r);
            } else {
                // place in ntwk_out_next
                ntwk->nodes[target]->ntwk_out_next.push_back(r);
            }
        } else {
            fatal_error("Unsupported Network Type\n");
        }
    }

    // ntwk in queue
    if (!ntwk_in_queue.empty()) {
        if (isDir) {
            Request *r = ntwk_in_queue.front();
            ntwk_in_queue.pop_front();
            dir->request_next.push_back(r);
        } else {
            if (cache->ntwk_in_next == NULL) {
                Request *r = ntwk_in_queue.front();
                ntwk_in_queue.pop_front();
                cache->ntwk_in_next = r;
            }
        }
    }
}

void Node::tock() {
    if (!isDir) {
        cpu->tock();
        cache->tock();
    } else {
        dir->tock();
    }

    // ntwk out next
    while (!ntwk_out_next.empty()) {
        Request *r = ntwk_out_next.front();
        ntwk_out_next.pop_front();
        ntwk_out_queue.push_back(r);
    }

    // ntwk in next
    while (!ntwk_in_next.empty()) {
        Request *r = ntwk_in_next.front();
        ntwk_in_next.pop_front();
        ntwk_in_queue.push_back(r);
    }
}

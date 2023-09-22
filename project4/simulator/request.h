#ifndef REQUEST_H
#define REQUEST_H

#include "messages.h"
#include "types.h"

class Request {
   public:
    Request(nodeID_t src, nodeID_t dest, blockID_t _block, message_t type);
    ~Request();

    nodeID_t srcID;
    nodeID_t destID;

    blockID_t block;

    message_t msg;
    int invack_count;

    timestamp_t req_time;

    void print_state();
};

#endif

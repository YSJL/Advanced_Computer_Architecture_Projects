#include "request.h"

Request::Request(nodeID_t src, nodeID_t dest, blockID_t _block, message_t type) {
    srcID = src;
    destID = dest;
    block = _block;
    msg = type;
}

Request::~Request() {}

void Request::print_state() {
    debug_printf("Request {src: %lu, dest: %lu, block: 0x%lx, msg: %s}\n", srcID, destID, block, message_t_str[msg]);
}

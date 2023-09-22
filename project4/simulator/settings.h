/**
 * Author: Pulkit Gupta
 * Created: 12/22/2022
 *
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stddef.h>

#include "types.h"

#define MAX_SHARERS 16

#include "protocols.h"

typedef struct settings_struct {
    protocol_t protocol;
    nodeID_t num_procs;
    cputype_t cputype;
    nettype_t nettype;
    int link_latency;
    int mem_latency;
    size_t block_size;  // log2 form

    char *trace_dir;

    settings_struct() : block_size(6) {}
} settings_t;

extern settings_t settings;

#endif  // SETTINGS_H

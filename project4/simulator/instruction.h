#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "types.h"

typedef struct instr_struct {
    addr_t address;
    char action;  // r: read, w: write
    instr_struct() : address(0), action('r') {}
} inst_t;

#endif

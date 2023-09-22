#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <cstring>
#include <list>
#include <vector>

#include "procsim.hpp"



// TODO: Scoreboard, use dynamic instruction count, use size() for busy except MUL, pipeline to spit it out
// TODO: Update RegFile, each entry is 2 bit "Free/Ready"
// Ready, Value, FU Where to store
// Variables
    std::vector<uint64_t> StoreBuffer;
    std::vector<inst_t> InfDispatchQ;
    std::vector<uint64_t> RAT;
    std::vector<int8_t> RegFile;
    std::vector<inst_t> SchedQ;
    std::vector<inst_t> ROB;
    //FU Scoreboard
    //Scoreboard[num_FUs][pipeline] for MUL
    std::vector<inst_t> ALUScoreboard;
    std::vector<std::vector<inst_t>> MULScoreboard;
    std::vector<inst_t> LSUScoreboard;
    std::vector<inst_t> FUResults;

    procsim_conf_t def = {};

    //Max SchedQ Size
    uint64_t MAX_SCHEDQ;

    int prev_retired_stored = 0;

// The helper functions in this#ifdef are optional and included here for your
// convenience so you can spend more time writing your simulator logic and less
// time trying to match debug trace formatting! (If you choose to use them)
#ifdef DEBUG
// TODO: Fix the debug outs
static void print_operand(int8_t rx) {
    if (rx < 0) {
        printf("(none)"); //  PROVIDED
    } else {
        printf("R%" PRId8, rx); //  PROVIDED
    }
}

// Useful in the fetch and dispatch stages
static void print_instruction(const inst_t *inst) {
    if (!inst) return;
    static const char *opcode_names[] = {NULL, NULL, "ADD", "MUL", "LOAD", "STORE", "BRANCH"};

    printf("opcode=%s, dest=", opcode_names[inst->opcode]); //  PROVIDED
    print_operand(inst->dest); //  PROVIDED
    printf(", src1="); //  PROVIDED
    print_operand(inst->src1); //  PROVIDED
    printf(", src2="); //  PROVIDED
    print_operand(inst->src2); //  PROVIDED
    printf(", dyncount=%lu", inst->dyn_instruction_count); //  PROVIDED
}

// This will print out the state of the RAT
static void print_rat(void) {
    for (uint64_t regno = 0; regno < NUM_REGS; regno++) {
        if (regno == 0) {
            printf("    { R%02" PRIu64 ": P%03" PRIu64 " }", regno, (uint64_t) RAT[regno]); // TODO: fix me
        } else if (!(regno & 0x3)) {
            printf("\n    { R%02" PRIu64 ": P%03" PRIu64 " }", regno, (uint64_t) RAT[regno]); //  TODO: fix me
        } else {
            printf(", { R%02" PRIu64 ": P%03" PRIu64 " }", regno, (uint64_t) RAT[regno]); //  TODO: fix me
        }
    }
    printf("\n"); //  PROVIDED
}

// This will print out the state of the register file, where P0-P31 are architectural registers 
// and P32 is the first PREG 
static void print_prf(void) {
    for (uint64_t regno = 0; regno < RegFile.size(); regno++) { // TODO: fix me
        if (regno == 0) {
            printf("    { P%03" PRIu64 ": Ready: %d, Free: %d }", regno, RegFile[regno]&1, (RegFile[regno]&2)>>1); // TODO: fix me
        } else if (!(regno & 0x3)) {
            printf("\n    { P%03" PRIu64 ": Ready: %d, Free: %d }", regno, RegFile[regno]&1, (RegFile[regno]&2)>>1); // TODO: fix me
        } else {
            printf(", { P%03" PRIu64 ": Ready: %d, Free: %d }", regno, RegFile[regno]&1, (RegFile[regno]&2)>>1); // TODO: fix me
        }
    }
    printf("\n"); //  PROVIDED
}

// This will print the state of the ROB where instructions are identified by their dyn_instruction_count
static void print_rob(void) {
    size_t printed_idx = 0;
    printf("\tAllocated Entries in ROB: %lu\n", ROB.size()); //
    for (uint64_t i = 0; i < ROB.size(); i++) { //
        if (printed_idx == 0) {
            printf("    { dyncount=%05" PRIu64 ", completed: %d, mispredict: %d, prev_preg: %d }", ROB[i].dyn_instruction_count, ROB[i].src2, (int) ROB[i].mispredict, ROB[i].src1);
        } else if (!(printed_idx & 0x3)) {
            printf("\n    { dyncount=%05" PRIu64 ", completed: %d, mispredict: %d, prev_preg: %d  }", ROB[i].dyn_instruction_count, ROB[i].src2, (int) ROB[i].mispredict, ROB[i].src1); // TODO: Fix Me
        } else {
            printf(", { dyncount=%05" PRIu64 " completed: %d, mispredict: %d, prev_preg: %d  }", ROB[i].dyn_instruction_count, ROB[i].src2, (int) ROB[i].mispredict, ROB[i].src1); // TODO: Fix Me
        }
        printed_idx++;
    }
    if (!printed_idx) {
        printf("    (ROB empty)"); //  PROVIDED
    }
    printf("\n"); //  PROVIDED
}
#endif




// Optional helper function which pops previously retired store buffer entries
// and pops instructions from the head of the ROB. (In a real system, the
// destination register value from the ROB would be written to the
// architectural registers, but we have no register values in this
// simulation.) This function returns the number of instructions retired.
// Immediately after retiring a mispredicting branch, this function will set
// *retired_mispredict_out = true and will not retire any more instructions. 
// Note that in this case, the mispredict must be counted as one of the retired instructions.
static uint64_t stage_state_update(procsim_stats_t *stats,
                                   bool *retired_mispredict_out) {
    // TODO: fill me in
#ifdef DEBUG
    printf("Stage Retire: \n"); //  PROVIDED
#endif
    uint64_t ret = 0;
    #ifdef DEBUG
        printf("	Popping %d store buffer entries that retired last cycle\n", prev_retired_stored); //  PROVIDED
    #endif
    while (prev_retired_stored != 0) {
        #ifdef DEBUG
            printf("		Popping back of store buffer: %#012lx\n", StoreBuffer.front()); //  PROVIDED
        #endif
        StoreBuffer.erase(StoreBuffer.begin());
        prev_retired_stored--;
    }
    #ifdef DEBUG
        printf("Checking ROB:\n"); //  PROVIDED
    #endif
    while (ROB.size() > 0) {
        if (ROB.front().src2 == 1) {
            #ifdef DEBUG
                printf("	Retiring: "); //  PROVIDED
                print_instruction(&ROB.front());
                printf("\n");
            #endif
            if (ROB.front().opcode == OPCODE_STORE) {
                #ifdef DEBUG
                    printf("		Retiring store, need to pop back of store buffer next cycle\n"); //  PROVIDED
                #endif
                prev_retired_stored++;
            }
            if (ROB.front().src1 >= NUM_REGS) {
                //RegFile is free
                #ifdef DEBUG
                    printf("		Freeing: P%d for areg: R%d\n", ROB.front().src1, ROB.front().dest); //  PROVIDED
                #endif
                RegFile[ROB.front().src1] = 2;
                //printf("ROB_front_prev_preg: %d, Reg[Prev_p] (after): %d\n", ROB.front().src1, RegFile[ROB.front().src1]);
            }
            ret++;
            if (ROB.front().mispredict == true) {
                *retired_mispredict_out = true;
                ROB.erase(ROB.begin());
                break;
            }
            ROB.erase(ROB.begin());
        } else {
            #ifdef DEBUG
                printf("	ROB entry %lu still in flight: dyncount=%lu\n", ret, ROB.front().dyn_instruction_count); //  PROVIDED
            #endif
            break;
        }
    }
    stats->instructions_retired += ret;
    return ret;
}

// Optional helper function which is responsible for moving instructions
// through pipelined Function Units and then when instructions complete (that
// is, when instructions are in the final pipeline stage of an FU and aren't
// stalled there), setting the ready bits in the register file. This function 
// should remove an instruction from the scheduling queue when it has completed.
static void stage_exec(procsim_stats_t *stats) {
    // TODO: fill me in
#ifdef DEBUG
    printf("Stage Exec: \n"); //  PROVIDED
#endif

    // Progress ALUs
#ifdef DEBUG
    printf("Progressing ALU units\n");  // PROVIDED
#endif
    for (uint64_t i = 0; i < ALUScoreboard.size(); i++) {
        #ifdef DEBUG
            printf("	Completing ALU: %lu, for dyncount=%lu\n", i, ALUScoreboard[i].dyn_instruction_count);
        #endif
        //printf("ALU[%lu] dyn: %lu\n",i, ALUScoreboard[i].dyn_instruction_count);
        if (++ALUScoreboard[i].src1 >= 1) {
            FUResults.push_back(ALUScoreboard[i]);
            ALUScoreboard.erase(ALUScoreboard.begin() + i);
            //printf("ALU size after erase: %lu\n", ALUScoreboard.size());
            //Erase Shifts one left
            i--;
            if (ALUScoreboard.size() == 0) {
                //printf("ALUSC Size 0 Break\n");
                break;
            }
        }

    }


    // Progress MULs
#ifdef DEBUG
    printf("Progressing MUL units\n");  // PROVIDED
#endif
    for (uint64_t i = 0; i < MULScoreboard.size(); i++) {
        for (uint64_t j = 0; j < MULScoreboard[i].size(); j++) {
            #ifdef DEBUG
                printf("	Completing MUL: %lu, for dyncount=%lu\n", i, MULScoreboard[i][j].dyn_instruction_count);
            #endif
            //printf("MUL[%lu][%lu] dyn: %lu\n",i, j, MULScoreboard[i][j].dyn_instruction_count);
            if (++MULScoreboard[i][j].src1 >= 3) {
                FUResults.push_back(MULScoreboard[i][j]);
                MULScoreboard[i].erase(MULScoreboard[i].begin() + j);
                //Erase Shifts one left
                j--;
                if (MULScoreboard[i].size() == 0) {
                    break;
                }
            }
        }
    }
    //printf("\n");

    // Progress LSU loads
#ifdef DEBUG
    printf("Progressing LSU units for loads\n");  // PROVIDED
#endif
    for (uint64_t i = 0; i < LSUScoreboard.size(); i++) {
        //printf("LSU[%lu] dyn: %lu\n",i, LSUScoreboard[i].dyn_instruction_count);
        if(LSUScoreboard[i].opcode == OPCODE_LOAD) {
            int StoreBufferIndex = -1;
            for (uint64_t j = 0; j < StoreBuffer.size(); j++) {
                if (StoreBuffer[j] == LSUScoreboard[i].load_store_addr) {
                    StoreBufferIndex = j;
                    break;
                }
            }
            LSUScoreboard[i].src1++;
            if ((LSUScoreboard[i].src1 == 1 && StoreBufferIndex != -1)
                || (LSUScoreboard[i].src1 == 2 && LSUScoreboard[i].dcache_miss == 0)
                || (LSUScoreboard[i].src1 >= 12)) {
                #ifdef DEBUG
                    printf("	Completing LSU: %lu, for dyncount=%lu,", i, LSUScoreboard[i].dyn_instruction_count);
                #endif
                if (LSUScoreboard[i].src1 == 1 && StoreBufferIndex != -1) {
                    stats->store_buffer_read_hits++;
                    #ifdef DEBUG
                        printf("Store Buffer Hit\n");
                    #endif
                } else if (LSUScoreboard[i].src1 == 2 && LSUScoreboard[i].dcache_miss == 0) {
                    stats->dcache_reads++;
                    stats->dcache_read_hits++;
                    #ifdef DEBUG
                        printf("Cache Hit\n");
                    #endif
                } else {
                    stats->dcache_reads++;
                    stats->dcache_read_misses++;
                    #ifdef DEBUG
                        printf("Cache Miss\n");
                    #endif
                }
                stats->reads++;
                FUResults.push_back(LSUScoreboard[i]);
                LSUScoreboard.erase(LSUScoreboard.begin() + i);
                //Erase Shifts one left
                i--;
                if (LSUScoreboard.size() == 0) {
                    break;
                }
            }
        }
    }

    // Progress LSU stores
#ifdef DEBUG
    printf("Progressing LSU units for stores\n");  // PROVIDED
#endif
    for (uint64_t i = 0; i < LSUScoreboard.size(); i++) {
        if (LSUScoreboard[i].opcode == OPCODE_STORE) {
            #ifdef DEBUG
                printf("	Completing LSU: %lu, for dyncount=%lu, adding %#012lx to Store Buffer\n", i
                  , LSUScoreboard[i].dyn_instruction_count, LSUScoreboard[i].load_store_addr);
            #endif
            if (++LSUScoreboard[i].src1 >= 1) {
                FUResults.push_back(LSUScoreboard[i]);
                StoreBuffer.push_back(LSUScoreboard[i].load_store_addr);
                LSUScoreboard.erase(LSUScoreboard.begin() + i);
                //Erase Shifts one left
                i--;
            }
        }
    }

    // Apply Result Busses
#ifdef DEBUG
    printf("Processing Result Busses\n"); // PROVIDED
#endif

    for (uint64_t j = 0; j < FUResults.size(); j++) {
        #ifdef DEBUG
            printf("	Processing Result Bus for: "); //  PROVIDED
            print_instruction(&FUResults[j]);
            printf("\n");
        #endif
        for (uint64_t i = 0; i < ROB.size(); i ++) {
            if (FUResults[j].dyn_instruction_count == ROB[i].dyn_instruction_count) {
                //printf("Result Bus = ROB[i]\n");
                ROB[i].src2 = 1;
            }
        }
        for (uint64_t i = 0; i < SchedQ.size(); i++) {
            //FUR.back() may cause problems
            if (FUResults.size() < j) {
                break;
            }
            if (FUResults[j].dyn_instruction_count == SchedQ[i].dyn_instruction_count) {
                //Ready bit of Regfile is 1
                if (SchedQ[i].dest > 0){
                    #ifdef DEBUG
                        printf("		Marking preg ready: %d\n",SchedQ[i].dest); //  PROVIDED
                    #endif
                    RegFile[SchedQ[i].dest] |= 1;
                }
                SchedQ.erase(SchedQ.begin() + i);
                FUResults.erase(FUResults.begin() + j);
                i--;
                j--;
            }
        }
    }
}

// Optional helper function which is responsible for looking through the
// scheduling queue and firing instructions that have their source pregs
// marked as ready. Note that when multiple instructions are ready to fire
// in a given cycle, they must be fired in program order. 
// Also, load and store instructions must be fired according to the 
// memory disambiguation algorithm described in the assignment PDF. Finally,
// instructions stay in their reservation station in the scheduling queue until
// they complete (at which point stage_exec() above should free their RS).
static void stage_schedule(procsim_stats_t *stats) {
    // TODO: SC size
#ifdef DEBUG
    printf("Stage Schedule: \n"); //  PROVIDED
#endif
    bool fired_store = 0;
    bool fired_any = 0;
    for (uint64_t i = 0; i < SchedQ.size(); i++) {
        if (check_src_regs(SchedQ[i].src1, SchedQ[i].src2) || in_FU(&SchedQ[i]) ) {
            continue;
        }
        #ifdef DEBUG
            printf("	Attempting to fire instruction: "); //  PROVIDED
            print_instruction(&SchedQ[i]);
            printf("\n");
        #endif
        #ifdef DEBUG
            printf("		Src0: %d, 1; Src1: %d, 1\n", SchedQ[i].src1, SchedQ[i].src2);
        #endif
        if (SchedQ[i].opcode == OPCODE_LOAD) {
            bool canFire = 1;
            for (uint64_t j = 0; j < i; j++) {
//            for (uint64_t j = 0; j < SchedQ.size(); j++) {
//                if (j + 1 == i) {
//                    if (j >= SchedQ.size()) break;
//                }
                if (SchedQ[j].opcode == OPCODE_STORE
                    && SchedQ[j].dyn_instruction_count < SchedQ[i].dyn_instruction_count) {
                    canFire = 0;
                }
            }
            if (LSUScoreboard.size() == def.num_lsu_fus) {
                canFire = 0;
            }
            if (canFire) {
                if (free_FU(SchedQ[i].opcode)) {
//                    Scoreboard[find_numFU(SchedQ[i].opcode)].push_back(SchedQ[i]);
                    LSUScoreboard.push_back(SchedQ[i]);
                    LSUScoreboard.back().src1 = 0;
                    fired_any = 1;
                    #ifdef DEBUG
                        printf("		Fired to LSU: %lu\n", LSUScoreboard.size() - 1);
                    #endif
                }
            }
        } else if (SchedQ[i].opcode == OPCODE_STORE) {
            bool canFire = 1;
            for (uint64_t j = 0; j < i; j++) {
//            for (uint64_t j = 0; j < SchedQ.size(); j++) {
//                if (j + 1 == i) {
//                    if (j >= SchedQ.size()) break;
//                }
                if (((SchedQ[j].opcode == OPCODE_LOAD || SchedQ[j].opcode == OPCODE_STORE)
                    && SchedQ[j].dyn_instruction_count < SchedQ[i].dyn_instruction_count) || fired_store) {
                    canFire = 0;
                }
            }
            if (LSUScoreboard.size() == def.num_lsu_fus) {
                canFire = 0;
            }
            if (canFire) {
                //LSU
                if (free_FU(SchedQ[i].opcode)) {
//                    Scoreboard[find_numFU(SchedQ[i].opcode)].push_back(SchedQ[i]);
                    LSUScoreboard.push_back(SchedQ[i]);
                    LSUScoreboard.back().src1 = 0;
                    fired_store = 1;
                    fired_any = 1;
                    #ifdef DEBUG
                        printf("		Fired to LSU: %lu\n", LSUScoreboard.size() - 1);
                    #endif
                }
            }
        } else {
            int j = free_FU(SchedQ[i].opcode);
//            if (SchedQ[i].dyn_instruction_count == 134) {
//                printf("D: is %d", j);
//            }
            if (j >= 0) {
//                Scoreboard[find_numFU(SchedQ[i].opcode)].push_back(SchedQ[i]);
                if (SchedQ[i].opcode == OPCODE_MUL) {
                    //printf("Pushed MUL[%d]: dyn - %lu\n",j,SchedQ[i].dyn_instruction_count);
                    MULScoreboard[j].push_back(SchedQ[i]);
                    MULScoreboard[j].back().src1 = 0;
                    fired_any = 1;
                    #ifdef DEBUG
                        printf("		Fired to MLU: %d\n", j);
                    #endif
                } else {
                    ALUScoreboard.push_back(SchedQ[i]);
                    ALUScoreboard.back().src1 = 0;
                    fired_any = 1;
                    #ifdef DEBUG
                        printf("		Fired to ALU: %lu\n", ALUScoreboard.size() - 1);
                    #endif
                }
            }
        }
    }
    if (!fired_any) {
        #ifdef DEBUG
            printf("	Could not find scheduling queue entry to fire this cycle\n"); //  PROVIDED
        #endif
        stats->no_fire_cycles++;
    }
}

bool in_FU(inst_t *inst) {
    if (inst->opcode == OPCODE_ADD || inst->opcode == OPCODE_BRANCH) {
        for (uint64_t i = 0; i < ALUScoreboard.size(); i++) {
            if (inst->dyn_instruction_count == ALUScoreboard[i].dyn_instruction_count) {
                return true;
            }
        }
    } else if (inst->opcode == OPCODE_MUL) {
        for (uint64_t i = 0; i < MULScoreboard.size(); i++) {
            for (uint64_t j = 0; j < MULScoreboard[i].size(); j++) {
                if (inst->dyn_instruction_count == MULScoreboard[i][j].dyn_instruction_count) {
                    return true;
                }
            }
        }
    } else if (inst->opcode == OPCODE_LOAD || inst->opcode == OPCODE_STORE) {
        for (uint64_t i = 0; i < LSUScoreboard.size(); i++) {
            if (inst->dyn_instruction_count == LSUScoreboard[i].dyn_instruction_count) {
                return true;
            }
        }
    }
    for (uint64_t i = 0; i < FUResults.size(); i++) {
        if (inst->dyn_instruction_count == FUResults[i].dyn_instruction_count) {
            return true;
        }
    }
    return false;
}

bool check_src_regs (int8_t src1, int8_t src2) {
    if (src1 < 0 && src2 < 0) {
        return false;
    } else if (src1 < 0) {
        return (RegFile[src2] & 1) == 0;
    } else if (src2 < 0) {
        return (RegFile[src1] & 1) == 0;
    }
    return (RegFile[src1] & 1) == 0 || (RegFile[src2] & 1) == 0;
}

int find_SCIndex(opcode_t opcode) {
    int SCIndex = -1;
    if (opcode == OPCODE_ADD || opcode == OPCODE_BRANCH) {
        SCIndex = 0;
    } else if (opcode == OPCODE_MUL) {
        SCIndex = 1;
    } else if (opcode == OPCODE_LOAD || opcode == OPCODE_STORE) {
        SCIndex = 2;
    }
    return SCIndex;
}
size_t find_numFU(opcode_t opcode) {
    size_t num_FU = -1;
    if (opcode == OPCODE_ADD || opcode == OPCODE_BRANCH) {
        num_FU = def.num_alu_fus;
    } else if (opcode == OPCODE_MUL) {
        num_FU = def.num_mul_fus;
    } else if (opcode == OPCODE_LOAD || opcode == OPCODE_STORE) {
        num_FU = def.num_lsu_fus;
    }
    return num_FU;
}

int free_FU(opcode_t opcode) {
    int SCIndex = find_SCIndex(opcode);
    if (SCIndex == 0) {
        if ((size_t) ALUScoreboard.size() < def.num_alu_fus) {
            return 1;
        }
    } else if (SCIndex == 1) {
        for (uint64_t i = 0; i < MULScoreboard.size(); i++) {
            if (MULScoreboard[i].size() < 3) {
                return i;
            }
        }
    } else if (SCIndex == 2){
        if ((size_t) LSUScoreboard.size() < def.num_lsu_fus) {
            return 1;
        }
    }
    return -1;
}


// Optional helper function which looks through the dispatch queue, decodes
// instructions, and inserts them into the scheduling queue. Dispatch should
// not add an instruction to the scheduling queue unless there is space for it
// in the scheduling queue and the ROB and a free preg exists if necessary; 
// effectively, dispatch allocates pregs, reservation stations and ROB space for 
// each instruction dispatched and stalls if there any are unavailable. 
// You will also need to update the RAT if need be.
// Note the scheduling queue has a configurable size and the ROB has P+32 entries.
// The PDF has details.
static void stage_dispatch(procsim_stats_t *stats) {
    // TODO: Check what to do when dest = -1
#ifdef DEBUG
    printf("Stage Dispatch: \n"); //  PROVIDED
#endif
    for (uint64_t i = 0; i < InfDispatchQ.size(); i++) {
        bool free_SchedQ = find_free_schedQ();
        int pregi = find_free_preg();
        if (free_SchedQ && pregi != -1 && ROB.size() < def.num_rob_entries) {
            #ifdef DEBUG
                printf("	Attempting Dispatch for: "); //  PROVIDED
                print_instruction(&InfDispatchQ[i]);
                printf("\n");
            #endif
            SchedQ.push_back(InfDispatchQ[i]);
            inst_t temp = InfDispatchQ[i];
//            #ifdef DEBUG
//                printf("TEMP B/Erase: %02" PRIu64 ", OP: %d, dest: %d, src1: %d, src2: %d\n"
//                  , temp.dyn_instruction_count, temp.opcode, temp.dest, temp.src1, temp.src2 );
//            #endif
            //TEMP might be erased here, double check
            InfDispatchQ.erase(InfDispatchQ.begin() + i);
            i--;
//            #ifdef DEBUG
//                printf("TEMP A/Erase: %02" PRIu64 ", OP: %d, dest: %d, src1: %d, src2: %d\n"
//                  , temp.dyn_instruction_count, temp.opcode, temp.dest, temp.src1, temp.src2 );
//            #endif
            if (temp.src1 >= 0) {
                #ifdef DEBUG
                    printf("		Using preg: P%lu for src1: R%d\n",RAT[temp.src1],SchedQ.back().src1);
                #endif
                SchedQ.back().src1 = RAT[temp.src1];
            }
            if (temp.src2 >= 0) {
                #ifdef DEBUG
                    printf("		Using preg: P%lu for src2: R%d\n",RAT[temp.src2],SchedQ.back().src2);
                #endif
                SchedQ.back().src2 = RAT[temp.src2];
            }
            inst_t rob_temp = temp;
            //rob_temp.dest = temp.dest;
            //Rob Preg = src1
            if (temp.dest > 0) {
                rob_temp.src1 = RAT[temp.dest];
                RAT[temp.dest] = pregi;
                #ifdef DEBUG
                    printf("		Allocating preg: %d for areg: %d and updating RAT\n", pregi, temp.dest); //  PROVIDED
                    printf("		Preg to Free: %d\n", rob_temp.src1);
                #endif
                SchedQ.back().dest = RAT[temp.dest];
                //Regfile[i] = not ready & not free
                RegFile[RAT[temp.dest]] = 0;
            }
            #ifdef DEBUG
                printf("		Dispatching instruction\n");
            #endif
            //Rob ready = src2
            rob_temp.src2 = 0;
            ROB.push_back(rob_temp);
        } else {
            if (pregi == -1) {
                stats->no_dispatch_pregs_cycles++;
            } else if (ROB.size() == def.num_rob_entries) {
                stats->rob_stall_cycles++;
            }
            break;
        }
    }
}

// Method which checks if there is a free spot in Scheduler Queue
bool find_free_schedQ() {
    if (SchedQ.size() < MAX_SCHEDQ) {
        return 1;
    }
    return 0;
}

// Method which returns a free PREG index
int find_free_preg() {
    for (uint64_t i = NUM_REGS; i < RegFile.size(); i++) {
        if (RegFile[i] > 1) {;
            return i;
        }
    }
    return -1;
}

// Optional helper function which fetches instructions from the instruction
// cache using the provided procsim_driver_read_inst() function implemented
// in the driver and appends them to the dispatch queue. To simplify the
// project, the dispatch queue is infinite in size.
static void stage_fetch(procsim_stats_t *stats) {
    // TODO: fill me in
#ifdef DEBUG
    printf("Stage Fetch\n"); //  PROVIDED
#endif
    for (size_t i = 0; i < def.fetch_width; i++) {
        const inst_t *temp = procsim_driver_read_inst();
        #ifdef DEBUG
            printf("	Fetched Instruction: ");
            print_instruction(temp);
            printf("\n"); //  PROVIDED
        #endif
        //printf("Temp: %02" PRIu64 "\n", temp->dyn_instruction_count);
        if (temp != NULL) {
            if (temp->icache_miss == 1) {
                #ifdef DEBUG
                    printf("		I-Cache miss repaired by driver\n");
                #endif
                stats->icache_misses++;
            }
            if (temp->mispredict == 1) {
                #ifdef DEBUG
                    printf("		Branch Misprediction will be handled by driver\n");
                #endif
            }
            InfDispatchQ.push_back(*temp);
//            #ifdef DEBUG
//                printf("Dispatch_Back: %02" PRIu64 ", OP: %d, dest: %d, src1: %d, src2: %d\n"
//                  , InfDispatchQ.back().dyn_instruction_count, InfDispatchQ.back().opcode, InfDispatchQ.back().dest, InfDispatchQ.back().src1, InfDispatchQ.back().src2 );
//            #endif
            //Nulls might need to be added
            stats->instructions_fetched++;
        }
    }
}

// Use this function to initialize all your data structures, simulator
// state, and statistics.
void procsim_init(const procsim_conf_t *sim_conf, procsim_stats_t *stats) {
    // TODO: for MULSC, make blank vector for all num, REGFILE
    def = *sim_conf;
    //Check Max SchedQ size
    MAX_SCHEDQ = def.num_schedq_entries_per_fu * (def.num_alu_fus + def.num_lsu_fus + def.num_mul_fus);

    //Basic setup for MULSC
    for (uint64_t i = 0; i < def.num_mul_fus; i++) {
        std::vector<inst_t> temp;
        MULScoreboard.push_back(temp);
    }

    //Basic Setup for RAT
    for (uint64_t i = 0; i < NUM_REGS; i++) {
        RAT.push_back((uint64_t)i);
    }

    //Basic setup for Regfile
    for (uint64_t i = 0; i < def.num_pregs + 32; i++) {
        if (i < NUM_REGS) {
            RegFile.push_back(1);
        } else {
            RegFile.push_back(2);
        }
    }


#ifdef DEBUG
    printf("\nScheduling queue capacity: %lu instructions\n", MAX_SCHEDQ); // TODO: Fix ME
    printf("Initial RAT state:\n"); //  PROVIDED
    print_rat();
    printf("\n"); //  PROVIDED
#endif
}

// To avoid confusion, we have provided this function for you. Notice that this
// calls the stage functions above in reverse order! This is intentional and
// allows you to avoid having to manage pipeline registers between stages by
// hand. This function returns the number of instructions retired, and also
// returns if a mispredict was retired by assigning true or false to
// *retired_mispredict_out, an output parameter.
uint64_t procsim_do_cycle(procsim_stats_t *stats,
                          bool *retired_mispredict_out) {
#ifdef DEBUG
    printf("================================ Begin cycle %" PRIu64 " ================================\n", stats->cycles); //  PROVIDED
#endif

    // stage_state_update() should set *retired_mispredict_out for us
    uint64_t retired_this_cycle = stage_state_update(stats, retired_mispredict_out);

    if (*retired_mispredict_out) {
#ifdef DEBUG
        printf("%" PRIu64 " instructions retired. Retired mispredict, so notifying driver to fetch correctly!\n", retired_this_cycle); //  PROVIDED
#endif

        // After we retire a misprediction, the other stages don't need to run
        stats->branch_mispredictions++;
    } else {
#ifdef DEBUG
        printf("%" PRIu64 " instructions retired. Did not retire mispredict, so proceeding with other pipeline stages.\n", retired_this_cycle); //  PROVIDED
#endif

        // If we didn't retire an interupt, then continue simulating the other
        // pipeline stages
        stage_exec(stats);
        stage_schedule(stats);
        stage_dispatch(stats);
        stage_fetch(stats);
    }

#ifdef DEBUG
    printf("End-of-cycle dispatch queue usage: %lu\n", InfDispatchQ.size());
    printf("End-of-cycle sched queue usage: %lu\n", SchedQ.size());
    printf("End-of-cycle ROB usage: %lu\n", ROB.size());
    printf("End-of-cycle RAT state:\n"); //  PROVIDED
    print_rat();
    printf("End-of-cycle Physical Register File state:\n"); //  PROVIDED
    print_prf();
    printf("End-of-cycle ROB state:\n"); //  PROVIDED
    print_rob();
    printf("================================ End cycle %" PRIu64 " ================================\n", stats->cycles); //  PROVIDED
    print_instruction(NULL); // this makes the compiler happy, ignore it
#endif

    // TODO: Increment max_usages and avg_usages in stats here!
    stats->cycles++;
    if (stats->dispq_max_size < InfDispatchQ.size()) {
        stats->dispq_max_size = InfDispatchQ.size();
    }
    if (stats->schedq_max_size < SchedQ.size()) {
        stats->schedq_max_size = SchedQ.size();
    }
    if (stats->rob_max_size < ROB.size()) {
        stats->rob_max_size = ROB.size();
    }
    stats->dispq_avg_size += InfDispatchQ.size();
    stats->schedq_avg_size += SchedQ.size();
    stats->rob_avg_size += ROB.size();


    // Return the number of instructions we retired this cycle (including the
    // interrupt we retired, if there was one!)
    return retired_this_cycle;
}

// Use this function to free any memory allocated for your simulator and to
// calculate some final statistics.
void procsim_finish(procsim_stats_t *stats) {
    // TODO: fill me in
    stats->store_buffer_hit_ratio = (double)stats->store_buffer_read_hits / (double)stats->reads;
    stats->dcache_read_miss_ratio = (double)stats->dcache_read_misses / (double)stats->dcache_reads;
    stats->dcache_ratio = (double)stats->dcache_reads / (double) stats->reads;
    stats->dcache_read_aat = L1_HIT_TIME + (double)stats->dcache_read_miss_ratio * (double)L1_MISS_PENALTY;
    stats->read_aat = (double) stats->store_buffer_hit_ratio + (double) stats->dcache_ratio * (double) stats->dcache_read_aat;
    stats->dispq_avg_size = stats->dispq_avg_size / (double)stats->cycles;
    stats->schedq_avg_size = stats->schedq_avg_size / (double)stats->cycles;
    stats->rob_avg_size = stats->rob_avg_size / (double)stats->cycles;
    stats->ipc = (double)stats->instructions_retired / (double)stats->cycles;
}

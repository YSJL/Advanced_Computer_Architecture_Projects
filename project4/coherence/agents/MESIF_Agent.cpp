#include "MESIF_Agent.h"

#include "agent.h"

MESIF_Agent::MESIF_Agent(nodeID_t id, Cache *cache, blockID_t tag) : Agent(id, cache, tag) { state = MESIF_CACHE_I; }

MESIF_Agent::~MESIF_Agent() {}

void MESIF_Agent::process_proc_request(Request *request) {
    debug_printf("\t\tHandling proc request: ");
    request->print_state();
    debug_printf("\t\t\tCurrent state: ");
    print_state();
    switch (state) {
        case MESIF_CACHE_I:
            do_proc_I(request);
            break;
        case MESIF_CACHE_M:
            do_proc_M(request);
            break;
        case MESIF_CACHE_S:
            do_proc_S(request);
            break;
        case MESIF_CACHE_E:
            do_proc_E(request);
            break;
        case MESIF_CACHE_F:
            do_proc_F(request);
            break;
        case MESIF_CACHE_IS:
        case MESIF_CACHE_IM:
        case MESIF_CACHE_SM:
        case MESIF_CACHE_EM:
        case MESIF_CACHE_FM:
            do_proc_in_transient(request);
            break;

        default:
            print_state();
            fatal_error("Invalid Cache State for MESIF Protocol\n");
    }
    debug_printf("\t\t\tNew state: ");
    print_state();
}

void MESIF_Agent::process_ntwk_request(Request *request) {
    debug_printf("\t\tHandling ntwk request: ");
    request->print_state();
    debug_printf("\t\t\tCurrent state: ");
    print_state();
    switch (state) {
        case MESIF_CACHE_I:
            do_ntwk_I(request);
            break;
        case MESIF_CACHE_M:
            do_ntwk_M(request);
            break;
        case MESIF_CACHE_S:
            do_ntwk_S(request);
            break;
        case MESIF_CACHE_E:
            do_ntwk_E(request);
            break;
        case MESIF_CACHE_F:
            do_ntwk_F(request);
            break;
        case MESIF_CACHE_IS:
            do_ntwk_IS(request);
            break;
        case MESIF_CACHE_IM:
            do_ntwk_IM(request);
            break;
        case MESIF_CACHE_SM:
            do_ntwk_SM(request);
            break;
        case MESIF_CACHE_EM:
            do_ntwk_EM(request);
            break;
        case MESIF_CACHE_FM:
            do_ntwk_FM(request);
            break;
        default:
            print_state();
            fatal_error("Invalid Cache State for MESIF Protocol\n");
    }
    debug_printf("\t\t\tNew state: ");
    print_state();
}

void MESIF_Agent::print_state() {
    const char *block_states[11] = {"X", "I", "S", "E", "M", "F", "IS", "IM", "SM", "EM", "FM"};
    debug_printf("MESIF_Agent - state: %s\n", block_states[state]);
}

void MESIF_Agent::do_proc_I(Request *request) {
    switch (request->msg) {
        // If we get a request from the processor we need to get the data
        case LOAD:
            send_GETS(request->block);
            /* The IS state means that we have sent the GETS message and we are now waiting
             * on DATA
             */
            state = MESIF_CACHE_IS;
            /* This is a cache miss */
            sim->cache_misses++;
            break;
        case STORE:
            send_GETM(request->block);
            /* The IM state means that we have sent the GETM message and we are now waiting
             * on DATA
             */
            state = MESIF_CACHE_IM;
            /* This is a cache miss */
            sim->cache_misses++;
            break;
        default:
            fatal_error("MESIF_Agent: I state shouldn't see this message from proc: %s\n", message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_proc_S(Request *request) {
    switch (request->msg) {
        // If we get a request from the processor we need to get the data
        case LOAD:
            // This is how you respond to the processor
            send_DATA_proc(request->block);
            break;
        case STORE:
            send_GETM(request->block);
            /* The SM state means that we have sent the GETM message and we are now waiting
             * for the directory to tell us no one else has the data and we can go to M
             */
            state = MESIF_CACHE_SM;
            break;
        default:
            fatal_error("MESIF_Agent: S state shouldn't see this message from proc: %s\n", message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_proc_M(Request *request) {
    switch (request->msg) {
        // If we get a request from the processor we need to get the data
        case LOAD:
        case STORE:
            // This is how you respond to the processor
            send_DATA_proc(request->block);
            break;
        default:
            fatal_error("MESIF_Agent: M state shouldn't see this message from proc: %s\n", message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_proc_E(Request *request) {
    switch (request->msg) {
        // If we get a request from the processor we need to get the data
        case LOAD:
            send_DATA_proc(request->block);
            break;
        case STORE:
            // Request directory to give you upgrade permissions
            send_GETX(request->block);
            state = MESIF_CACHE_EM;
            break;
        default:
            fatal_error("MESIF_Agent: E state shouldn't see this message from proc: %s\n", message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_proc_F(Request *request) {
    switch (request->msg) {
        // If we get a request from the processor we need to get the data
        case LOAD:
            send_DATA_proc(request->block);
            break;
        case STORE:
            // Request directory to give you write permissions
            send_GETM(request->block);
            state = MESIF_CACHE_FM;
            break;
        default:
            fatal_error("MESIF_Agent: F state shouldn't see this message from proc: %s\n", message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_proc_in_transient(Request *request) {
    switch (request->msg) {
        /* If the block is in a transient state that means it sent out a GET message
         * and is waiting on DATA. Therefore the processor should be waiting
         * on a pending request. Therefore we should not be getting any requests from
         * the processor.
         */
        case LOAD:
        case STORE:
            fatal_error("Should only have one outstanding request per processor!");
        default:
            fatal_error("MESIF_Agent: Transient state shouldn't see this message\n");
    }
}

void MESIF_Agent::do_ntwk_I(Request *request) {
    switch (request->msg) {
        default:
            fatal_error("MESIF_Agent: I state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}
void MESIF_Agent::do_ntwk_S(Request *request) {
    switch (request->msg) {
        case REQ_INVALID:
            send_INVACK(request->block);
            state = MESIF_CACHE_I;
            break;
        default:
            fatal_error("MESIF_Agent: S state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}
void MESIF_Agent::do_ntwk_M(Request *request) {
    switch (request->msg) {
        case RECALL_GOTO_I:
            send_DATA_dir(request->block);
            state = MESIF_CACHE_I;
            break;
        case RECALL_GOTO_S:
            send_DATA_dir(request->block);
            state = MESIF_CACHE_S;
            break;
        default:
            fatal_error("MESIF_Agent: M state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_ntwk_E(Request *request) {
    switch (request->msg) {
        case RECALL_GOTO_I:
            send_DATA_dir(request->block);
            state = MESIF_CACHE_I;
            break;
        case RECALL_GOTO_S:
            send_DATA_dir(request->block);
            state = MESIF_CACHE_F;
            break;
        default:
            fatal_error("MESIF_Agent: E state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_ntwk_IM(Request *request) {
    switch (request->msg) {
        case DATA:
            send_DATA_proc(request->block);
            state = MESIF_CACHE_M;
            break;
        default:
            fatal_error("MESIF_Agent: IM state shouldn't see this message from ntwk: %s\n",
                        message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_ntwk_IS(Request *request) {
    switch (request->msg) {
        case DATA:
            send_DATA_proc(request->block);
            state = MESIF_CACHE_S;
            break;
        case DATA_E:
            send_DATA_proc(request->block);
            state = MESIF_CACHE_E;
            break;
        default:
            fatal_error("MESIF_Agent: IS state shouldn't see this message from ntwk: %s\n",
                        message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_ntwk_SM(Request *request) {
    switch (request->msg) {
        case REQ_INVALID:
            send_INVACK(request->block);
            state = MESIF_CACHE_IM;
            break;
        case ACK:
            send_DATA_proc(request->block);
            state = MESIF_CACHE_M;
            break;
        default:
            fatal_error("MESIF_Agent: SM state shouldn't see this message from ntwk: %s\n",
                        message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_ntwk_EM(Request *request) {
    switch (request->msg) {
        case RECALL_GOTO_I:
            send_DATA_dir(request->block);
            state = MESIF_CACHE_IM;
            break;
        case RECALL_GOTO_S:
            send_DATA_dir(request->block);
            state = MESIF_CACHE_FM;
            break;
        case ACK:
            send_DATA_proc(request->block);
            state = MESIF_CACHE_M;
            break;
        default:
            fatal_error("MESIF_Agent: EM state shouldn't see this message from ntwk: %s\n",
                        message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_ntwk_F(Request *request) {
    switch (request->msg) {
        case RECALL_GOTO_I:
            send_DATA_dir(request->block);
            state = MESIF_CACHE_I;
            break;
        case RECALL_GOTO_S:
            send_DATA_dir(request->block);
            state = MESIF_CACHE_F;
            break;
        default:
            fatal_error("MESIF_Agent: F state shouldn't see this message from ntwk: %s\n", message_t_str[request->msg]);
    }
}

void MESIF_Agent::do_ntwk_FM(Request *request) {
    switch (request->msg) {
        case RECALL_GOTO_I:
            send_DATA_dir(request->block);
            state = MESIF_CACHE_IM;
            break;
        case RECALL_GOTO_S:
            send_DATA_dir(request->block);
            state = MESIF_CACHE_FM;
            break;
        case ACK:
            send_DATA_proc(request->block);
            state = MESIF_CACHE_M;
            break;
        default:
            fatal_error("MESIF_Agent: FM state shouldn't see this message from ntwk: %s\n",
                        message_t_str[request->msg]);
    }
}

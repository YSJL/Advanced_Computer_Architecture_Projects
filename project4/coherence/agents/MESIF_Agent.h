#ifndef MESIF_AGENT_H
#define MESIF_AGENT_H

#include "agent.h"

/** Cache states.  */
typedef enum {
    MESIF_CACHE_I = 1,
    MESIF_CACHE_S,
    MESIF_CACHE_E,
    MESIF_CACHE_M,
    MESIF_CACHE_F,
    MESIF_CACHE_IS,
    MESIF_CACHE_IM,
    MESIF_CACHE_SM,
    MESIF_CACHE_EM,
    MESIF_CACHE_FM
} MESIF_cache_state_t;

class MESIF_Agent : public Agent {
   public:
    // Cache state for this cache block
    MESIF_cache_state_t state;

    MESIF_Agent(nodeID_t id, Cache *cache, blockID_t tag);
    ~MESIF_Agent();

    // sim will handle deletion/free
    void process_proc_request(Request *request);

    // sim will handle deletion/free
    void process_ntwk_request(Request *request);

    void print_state();

    void do_proc_I(Request *request);
    void do_proc_S(Request *request);
    void do_proc_E(Request *request);
    void do_proc_M(Request *request);
    void do_proc_F(Request *request);
    void do_proc_in_transient(Request *request);
    void do_ntwk_I(Request *request);
    void do_ntwk_S(Request *request);
    void do_ntwk_E(Request *request);
    void do_ntwk_M(Request *request);
    void do_ntwk_F(Request *request);
    void do_ntwk_IM(Request *request);
    void do_ntwk_IS(Request *request);
    void do_ntwk_SM(Request *request);
    void do_ntwk_EM(Request *request);
    void do_ntwk_FM(Request *request);
};

#endif

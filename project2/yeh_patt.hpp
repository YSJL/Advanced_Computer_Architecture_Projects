#ifndef YEHPATT_H
#define YEHPATT_H

#include "branchsim.hpp"
#include "Counter.hpp"
#include "inttypes.h"

// Yeh-Patt predictor definition
class yeh_patt : public branch_predictor_base
{

private:

    // TODO: add your fields here
    uint64_t* HT;
    uint* PT;
    uint64_t index_mask;
    uint64_t p_mask;

public:
    // create optional helper functions here

    void init_predictor(branchsim_conf *sim_conf);

    // Return the prediction
    bool predict(branch *branch);

    // Update the branch predictor state
    void update_predictor(branch *branch);

    // Cleanup any allocated memory here
    ~yeh_patt();
};

#endif

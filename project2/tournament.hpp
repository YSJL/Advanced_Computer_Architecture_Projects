#ifndef TOURNAMENT_H
#define TOURNAMENT_H

#include "branchsim.hpp"
#include "Counter.hpp"
#include "perceptron.hpp"
#include "yeh_patt.hpp"

// Tournament predictor definition
class tournament : public branch_predictor_base
{

private:

    uint index_mask;
    uint* counters;
    yeh_patt *yeh_patt_predictor;
    perceptron *perceptron_predictor;
    bool y_prediction;
    bool p_prediction;


public:
    // create optional helper functions here

    void init_predictor(branchsim_conf *sim_conf);

    // Return the prediction
    bool predict(branch *branch);

    // Update the branch predictor state
    void update_predictor(branch *branch);

    // Cleanup any allocated memory here
    ~tournament();
};

#endif

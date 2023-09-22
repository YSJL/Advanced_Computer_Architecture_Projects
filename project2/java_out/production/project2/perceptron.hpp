#ifndef PERCEPTRON_H
#define PERCEPTRON_H

#include "branchsim.hpp"

// Perceptron predictor definition
class perceptron : public branch_predictor_base
{

private:

    uint* GHR;
    uint64_t index_mask;
    int theta;
    //struct weights {int* weight;};
    std::vector<std::vector<int>> perceptron;
    int per_y;
    uint per_g;




public:
    // create optional helper functions here

    void init_predictor(branchsim_conf *sim_conf);

    // Return the prediction
    bool predict(branch *branch);

    // Update the branch predictor state
    void update_predictor(branch *branch);

    // Cleanup any allocated memory here
    ~perceptron();
};

#endif

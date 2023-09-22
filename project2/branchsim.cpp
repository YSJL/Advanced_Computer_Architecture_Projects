#include <iostream>
#include <cmath>
#include <iterator>
#include <algorithm>
#include <cmath>
#include <cstring>

#include "branchsim.hpp"
#include "yeh_patt.hpp"
#include "Counter.hpp"
#include "tournament.hpp"
#include "perceptron.hpp"

// Don't modify this line -- Its to make the compiler happy
branch_predictor_base::~branch_predictor_base() {}


// ******* Student Code starts here *******

// Yeh-Patt Branch Predictor

void yeh_patt::init_predictor(branchsim_conf *sim_conf)
{
    // Set size of History Table using L
    uint64_t size_HT = pow(2,sim_conf->L);
    // Set size of Pattern Table using P
    uint64_t size_PT = pow(2,sim_conf->P);
    // Set size of Hit Table, array of uint, to size_HT
    HT = new uint64_t[size_HT];
    // Set all uint values of Hit Table to 0 (May be wrong)
    std::fill_n(HT, size_HT, 0);
    // Set size of Pattern Table, array of uint, to size_PT
    PT = new uint[size_PT];
    // Set all uint values of Pattern Table to 0b01, the Weakly-NOT-taken state
    std::fill_n(PT, size_PT, 1);
    // Bit mask of (2+L-1) for getting the index of PC Address
    index_mask = ~(~0 << (2 + (sim_conf->L)));
    // Bit mask for one entry of HT. Leaves two bits at the end so it needs to be shifted
    p_mask = ~(~0 << (sim_conf->P));

}

bool yeh_patt::predict(branch *branch)
{
    // Return boolean ret is false for default for branch is Not Taken
    bool ret = false;
    // Calculate the Index of HT
    uint64_t index = (branch->ip & index_mask) >> 2;
    //Return true if PT value at Index is bigger or equal to 0b10 since branch is Taken
    if (PT[HT[index]] >= 2) {
      ret = true;
    }
    // Return ret
    return ret;
}

void yeh_patt::update_predictor(branch *branch)
{
    // Calculate the Index of HT
    uint64_t index = (branch->ip & index_mask) >> 2;
    // Find the History value of Index
    uint64_t history = HT[index];

    // If actual branch is Taken
    if (branch->is_taken) {
      //Update PT value to +1 unless 0b11
      if (PT[history] < 3) {
        PT[history]++;
      }
    } else {
      //Update PT value to -1 unless 0b00
      if (PT[history] > 0) {
        PT[history]--;
      }
    }

    // Update new History value
    uint64_t branchVal = 0;
    if (branch->is_taken) {
        branchVal = 1;
    }
    HT[index] = ((history << 1) & p_mask) | branchVal;
}

yeh_patt::~yeh_patt()
{
    //Free all memory for yeh_patt
    delete[] HT;
    delete[] PT;
}


// Perceptron Branch Predictor

void perceptron::init_predictor(branchsim_conf *sim_conf)
{
    //Calculate theta
    theta = static_cast<int> (std::floor(((float)1.93 * ((float)sim_conf->G)) + 14));

    //Perceptron Table size
    uint64_t size_PT = pow(2,sim_conf->N);
    //Set up Perceptron Table
    perceptron.resize(size_PT, std::vector<int>((sim_conf->G + 1), 0));

    //Setup GHR
    per_g = sim_conf->G;
    GHR = new uint[sim_conf->G];
    //Fills all values to -1
    std::fill_n(GHR, per_g, -1);

    //Bit mask of (2+N-1) for getting the index of PC Address
    index_mask = ~(~0 << (2 + sim_conf->N));
}

bool perceptron::predict(branch *branch)
{
    //Basic return boolean value
    bool ret = false;
    //Calculate PC Index
    uint64_t index = (branch->ip & index_mask) >> 2;

    //x[0] value is 1, start with w[0]
    per_y = perceptron[index][0];

    //Calculate y
    for (uint i = 0; i < per_g; i++) {
      per_y += GHR[i] * perceptron[index][i+1];
    }

    //if y > 0, return true, else return false
    if (per_y > 0) {
      ret = true;
    }
    return ret;
}

void perceptron::update_predictor(branch *branch)
{
    //Value for the actual value of branch T or NT
    int actualBranch = -1;
    if (branch->is_taken) {
        actualBranch = 1;
    }

    //Calculate PC Index to update predictron
    uint64_t index = (branch->ip & index_mask) >> 2;
    //Get predicted boolean value
    bool prediction = false;
    if (per_y > 0) {
        prediction = true;
    }
    //Update perceptron table
    if ((prediction != branch->is_taken) || abs(per_y) < theta) {
        //Update first weight with x[0] as 1
        perceptron[index][0] += actualBranch;
        //Check if weight is out of bounds
        if (perceptron[index][0] > theta) {
            perceptron[index][0] = theta;
        } else if (perceptron[index][0] < (-1 * theta)) {
            perceptron[index][0] = -1 * theta;
        }
        for (uint i = 0; i < per_g; i++) {
            //Update rest of the weights with the GHR values
            perceptron[index][i + 1] += actualBranch * GHR[i];
            //Check if weight is out of bounds
            if (perceptron[index][i + 1] > theta) {
                perceptron[index][i + 1] = theta;
            } else if (perceptron[index][i + 1] < (-1 * theta)) {
                perceptron[index][i + 1] = -1 * theta;
            }
        }
    }
    //Update GHR after perceptron training
    for (uint i = per_g - 1; i > 0; i--) {
        //Shifting GHR
        GHR[i] = GHR[i - 1];
    }
    //Updating the most recent GHR value
    GHR[0] = actualBranch;

    return;
}

perceptron::~perceptron()
{
    //Free all memory for perceptron
    delete[] GHR;
}

// Tournament Branch Predictor

void tournament::init_predictor(branchsim_conf *sim_conf)
{
    //Bit mask of 13 for getting the index of PC Address
    index_mask = ~(~0 << (14));

    //Initialize counter array
    uint64_t size_counters = pow(2,12);
    counters = new uint[size_counters];
    //Default counter val
    uint def_counter = 0;
    if (sim_conf->C == 0) {
        def_counter = 0;
    } else if (sim_conf->C == 1) {
        def_counter = 7;
    } else if (sim_conf->C == 2) {
        def_counter = 8;
    } else {
        def_counter = 15;
    }
    //Fill counter array with default counter val
    std::fill_n(counters, size_counters, def_counter);

    //Initialize both perceptron and yeh_patt
    perceptron_predictor = new perceptron();
    perceptron_predictor->init_predictor(sim_conf);
    yeh_patt_predictor = new yeh_patt();
    yeh_patt_predictor->init_predictor(sim_conf);

}

bool tournament::predict(branch *branch)
{
    //Calculate PC Index to check counter array
    uint64_t index = (branch->ip & index_mask) >> 2;

    //Prediction result
    bool ret = false;

    //If counter value at index is lower than 1000, use Yeh-Patt, otherwise use Perceptron
    if (counters[index] < 8) {
        ret = yeh_patt_predictor->predict(branch);
        y_prediction = ret;
        p_prediction = perceptron_predictor->predict(branch);
    } else {
        ret = perceptron_predictor->predict(branch);
        p_prediction = ret;
        y_prediction = yeh_patt_predictor->predict(branch);
    }
    //Return value from either Yeh-Patt or Perceptron
    return ret;
}

void tournament::update_predictor(branch *branch)
{
    //Calculate PC Index to check counter array
    uint64_t index = (branch->ip & index_mask) >> 2;

    //Update yeh_patt and perceptron
    yeh_patt_predictor->update_predictor(branch);
    perceptron_predictor->update_predictor(branch);

    //Update counters
    if (p_prediction != y_prediction) {
        // If Perceptron was correct
        if (p_prediction == branch->is_taken) {
            //Add 1 unless counter value is already 15
            if (++counters[index] > 15) {
                counters[index] = 15;
            }
        // If Yeh-Patt was correct
        } else {
            //Neg 1 unless counter value is already 0;
            if (counters[index] == 0) {
                counters[index] = 0;
            } else {
                counters[index]--;
            }
        }
    }
}

tournament::~tournament()
{
    //Free all memory for tournament
    delete[] counters;
    delete perceptron_predictor;
    delete yeh_patt_predictor;
}


// Common Functions to update statistics and final computations, etc.

/**
 *  Function to update the branchsim stats per prediction. Here we now know the
 *  actual outcome of the branch, so you can update misprediction counters etc.
 *
 *  @param prediction The prediction returned from the predictor's predict function
 *  @param branch Pointer to the branch that is being predicted -- contains actual behavior
 *  @param stats Pointer to the simulation statistics -- update in this function
*/
void branchsim_update_stats(bool prediction, branch *branch, branchsim_stats *sim_stats) {

    sim_stats->total_instructions = branch->inst_num;
    //Increment number of branch instructions
    sim_stats->num_branch_instructions++;
    //Check if prediction is taken
    if (prediction == branch->is_taken) {
      //Increment correctly predictd
      sim_stats->num_branches_correctly_predicted++;
    } else {
      //Increment mispredicted
      sim_stats->num_branches_mispredicted++;
    }
    //Calculate misses per kilo instructions
    sim_stats->misses_per_kilo_instructions = static_cast<uint64_t> (sim_stats->num_branches_mispredicted * 1000 / sim_stats->total_instructions);
}

/**
 *  Function to finish branchsim statistic computations such as prediction rate, etc.
 *
 *  @param stats Pointer to the simulation statistics -- update in this function
*/
void branchsim_finish_stats(branchsim_stats *sim_stats) {
    //Calculate fraction of branch instructions
    sim_stats->fraction_branch_instructions = ((double) sim_stats->num_branch_instructions / (double) sim_stats->total_instructions);
    //Calculate prediction accuracy
    sim_stats->prediction_accuracy = ((double) sim_stats->num_branches_correctly_predicted / (double) sim_stats->num_branch_instructions);
}

#ifndef BRANCHSIM_H
#define BRANCHSIM_H

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

const bool TAKEN = true;
const bool NOT_TAKEN = false;

typedef struct branch_t {
    uint64_t ip; // ip = instruction pointer aka. address of the branch instruction or PC
    uint64_t inst_num; // Instruction count of the branch

    bool is_taken;  // Is the branch actually taken

    branch_t() { }

} branch;

// Structure to hold configuration details for a branchsim simulation run
typedef struct branchsim_conf_t {

    enum PREDICTOR {YEHPATT = 1, PERCEPTRON, TOURNAMENT};
    
    // Global Parameters:
    enum PREDICTOR predictor; // The predictor being used 1: Yeh-Patt, 2: Perceptron, 3: Tournament between the two

    // Yeh Patt Parameters:
    uint64_t P; // log2 Height of the Pattern Table
    uint64_t L; // log2 Height of the Local History Table

    // Perceptron Parameters:
    uint64_t N; // log2 Height of Perceptron Table
    uint64_t G; // length of global history used

    // Tournament Parameters:
    uint64_t C; // initial state of the counters in the tournament only.
        // 0: Strongly not taken (0000)
        // 1: Weakly not taken   (0111)
        // 2: Weakly taken       (1000)
        // 3: Strongly taken     (1111)

    // Default constructor
    branchsim_conf_t() : predictor(PREDICTOR::YEHPATT), 
        P(13), L(12), N(9), G(32), C(1) {}

} branchsim_conf;

// static map to get the string for a predictor
static const std::map<branchsim_conf_t::PREDICTOR, const char *> pred_to_string = {
    {branchsim_conf::YEHPATT, "Yeh-Patt"},
    {branchsim_conf::PERCEPTRON, "PERCEPTRON"},
    {branchsim_conf::TOURNAMENT, "TOURNAMENT"},
};

// Structure to hold statistics for a branchsim simulation run
typedef struct branchsim_stats_t {
    uint64_t total_instructions;            // Use the inst num field of the final branch
    uint64_t num_branch_instructions;
    uint64_t num_branches_correctly_predicted;
    uint64_t num_branches_mispredicted;
    uint64_t misses_per_kilo_instructions;  // Yes this is indeed being recorded as a uint64_t
                                            // Allows tracking various phases of program execution
                                            // and is a common metric in branch prediction papers

    double fraction_branch_instructions;    // fraction of the entire program that is branches
    double prediction_accuracy;
} branchsim_stats;

// Base class for a branch predictor
class branch_predictor_base {

public:
    // Initialize the predictor state including any data structures you might need
    virtual void init_predictor(branchsim_conf *sim_conf) = 0;

    // Return the prediction ({taken/not-taken}, target-address)
    virtual bool predict(branch *branch) = 0;

    // Update the branch predictor state
    virtual void update_predictor(branch *branch) = 0;

    // Cleanup any allocated memory here
    virtual ~branch_predictor_base() = 0;
};


// Function to update branch prediction statistics after the prediction
void branchsim_update_stats(bool prediction, branch *branch, branchsim_stats *sim_stats);

// Function to perform cleanup and final statistics calculations, etc
void branchsim_finish_stats(branchsim_stats *sim_stats);

#endif

/**
 * @file branchsim_driver.cpp
 * @brief Trace reader and driver for a branch prediction project assigned in
 * CS{4/6}290 and ECE{4/6}100 for the Spring 2022 semester
 *
 * DON"T MODIFY ANY CODE IN THIS FILE!
 *
 * @author Anirudh Jain and Pulkit Gupta
 */

#include <getopt.h>
#include <unistd.h>


#include <cstdarg>
#include <cinttypes>
#include <cstdio>
#include <cstdbool>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include "branchsim.hpp"
#include "yeh_patt.hpp"
#include "tournament.hpp"
#include "perceptron.hpp"

// Print message and exit from the application
static inline void print_error_exit(const char *msg, ...)
{
    va_list argptr;
    va_start(argptr, msg);
    fprintf(stderr, msg, argptr);
    va_end(argptr);
    exit(EXIT_FAILURE);
}


// Print error usage
static void print_err_usage(const char *err)
{
    // TODO: update this
    fprintf(stderr, "%s\n", err);
    fprintf(stderr, "./branchsim -i <trace file> [Options]\n");
    fprintf(stderr, "-M [Select the prediction scheme to use]\n");
    fprintf(stderr, "\t\t1: Yeh-Patt\n");
    fprintf(stderr, "\t\t2: Perceptron\n");
    fprintf(stderr, "\t\t3: Tournament\n");
    fprintf(stderr, "-P [Log2 of the height of pattern table in Yeh-Patt Predictor]\n");
    fprintf(stderr, "-L [Log2 of the height of history table in Yeh-Patt Predictor]\n");
    fprintf(stderr, "-N [Log2 of the height of the Perceptron Table]\n");
    fprintf(stderr, "-G [Width of Global History Register in bits]\n");
    fprintf(stderr, "-C [Initial state of tournament counters]\n");
    fprintf(stderr, "\t\t0: Strongly Not Taken\n");
    fprintf(stderr, "\t\t1: Weakly Not Taken\n");
    fprintf(stderr, "\t\t2: Weakly Taken\n");
    fprintf(stderr, "\t\t3: Strongly Taken\n");
    fprintf(stderr, "-H <Print this message>\n");

    exit(EXIT_FAILURE);
}

// Function to print the run configuration
static void print_sim_config(branchsim_conf *sim_conf)
{
    // TODO: Set This based on branchsim hpp
    printf("SIMULATION CONFIGURATION\n");
    printf("P:          %" PRIu64 "\n", sim_conf->P);
    printf("L:          %" PRIu64 "\n", sim_conf->L);
    printf("N:          %" PRIu64 "\n", sim_conf->N);
    printf("G:          %" PRIu64 "\n", sim_conf->G);
    printf("C:          %" PRIu64 "\n", sim_conf->C);
    printf("Predictor (M):  %s\n", pred_to_string.at(sim_conf->predictor));
}

// Function to print the simulation output
static void print_sim_output(branchsim_stats *sim_stats)
{
    printf("\nSIMULATION OUTPUT\n");
    printf("Total Instructions:                 %" PRIu64 "\n", sim_stats->total_instructions);
    printf("Total Branch Instructions:          %" PRIu64 "\n", sim_stats->num_branch_instructions);
    printf("Branches Correctly Predicted:       %" PRIu64 "\n", sim_stats->num_branches_correctly_predicted);
    printf("Branches Mispredicted:              %" PRIu64 "\n", sim_stats->num_branches_mispredicted);
    printf("Misses Per Kilo Instructions:       %" PRIu64 "\n", sim_stats->misses_per_kilo_instructions);

    printf("Fraction Branch Instructions:       %.8f\n", sim_stats->fraction_branch_instructions);
    printf("Branch Prediction accuracy:         %.8f\n", sim_stats->prediction_accuracy);
}


// Main function for the simulator driver
int main(int argc, char *const argv[])
{
    // Trace file
    FILE *trace = NULL;

    // FILE *temp = fopen("temp.trace", "w+");

    branchsim_stats sim_stats;
    memset(&sim_stats, 0, sizeof(branchsim_stats));

    branchsim_conf sim_conf = branchsim_conf();

    int opt;
    int pred_num = -1;

    // TODO: update this
    while (-1 != (opt = getopt(argc, argv, "i:I:m:M:p:P:l:L:n:N:g:G:c:C:hH"))) {
        switch (opt) {
            case 'm':
            case 'M':
                pred_num = std::atoi(optarg);
                if (pred_num < 1 || pred_num > 3) { print_err_usage("Invalid predictor option"); }
                sim_conf.predictor = (branchsim_conf::PREDICTOR) pred_num;
                break;

            case 'p':
            case 'P':
                sim_conf.P = (uint64_t) std::atoi(optarg);
                break;

            case 'l':
            case 'L':
                sim_conf.L = (uint64_t) std::atoi(optarg);
                break;

            case 'n':
            case 'N':
                sim_conf.N = (uint64_t) std::atoi(optarg);
                break;

            case 'g':
            case 'G':
                sim_conf.G = (uint64_t) std::atoi(optarg);
                break;
            
            case 'c':
            case 'C':
                sim_conf.C = (uint64_t) std::atoi(optarg);
                break;

            case 'i':
            case 'I':
                trace = std::fopen(optarg, "r");
                if (trace == NULL) {
                    print_err_usage("Could not open the input trace file");
                }
                break;

            case 'h':
            case 'H':
            default:
                print_err_usage("Invalid argument to program");
                break;
        }
    }

    if (!trace) {
        print_err_usage("No trace file provided!");
    }

    print_sim_config(&sim_conf);


    // TODO: fix the bounds checking nonsense

    if (sim_conf.P < 9) {
        printf("invalid configuration for P");
        exit(EXIT_FAILURE);
    }
    
    if (sim_conf.L < 9) {
        printf("invalid configuration for L");
        exit(EXIT_FAILURE);
    }
    
    if (sim_conf.N < 8) {
        printf("invalid configuration for N");
        exit(EXIT_FAILURE);
    }
    
    if (sim_conf.G < 32 || sim_conf.G > 64) {
        printf("invalid configuration for G");
        exit(EXIT_FAILURE);
    }
    if (sim_conf.C < 0 || sim_conf.C > 3) {
        printf("invalid configuration for C");
        exit(EXIT_FAILURE);
    }
    // if (pred_num == branchsim_conf::PREDICTOR::GSHARE && sim_conf.P > 2) {
    //     printf("invalid hash selector (P) for GShare\n");
    //     exit(EXIT_FAILURE);
    // }

    // if (pred_num == branchsim_conf::PREDICTOR::TAGE && (sim_conf.P < 3 || sim_conf.P > 10)) {
    //     printf("invalid number of tables (P) for TAGE\n");
    //     exit(EXIT_FAILURE);
    // }


    printf("SETUP COMPLETE - STARTING SIMULATION\n");

    // Allocate the predictor
    branch_predictor_base *predictor;
    switch (sim_conf.predictor) {
    
    case branchsim_conf::TOURNAMENT:
        predictor = new tournament();
        break;

    case branchsim_conf::PERCEPTRON:
        predictor = new perceptron();
        break;

    case branchsim_conf::YEHPATT:
    default:
        predictor = new yeh_patt();
        break;
    }

    // Initialize the predictor
    predictor->init_predictor(&sim_conf);

//    uint64_t HTl = pow(2,sim_conf.L);
//    uint64_t PTl = pow(2,sim_conf.P);
//    #ifdef DEBUG
//    printf("Yeh-Patt: Creating a history table of %" PRIu64
//    " entries of length %" PRIu64 "\n", HTl , sim_conf.P);
//    printf("Yeh-Patt: Creating a pattern table of %" PRIu64
//        " 2-bit saturating counters\n", PTl);
//    #endif

    branch branch;
    size_t num_branches = 0;
    while (!feof(trace)) {
        int is_taken;
        int ret = std::fscanf(trace, "%" PRIx64 " %d %" PRIu64 "\n", &branch.ip, &is_taken, &branch.inst_num);

        // Read branch from the input trace
        if (ret == 3) {
            branch.is_taken = is_taken;

//            #ifdef DEBUG
//                printf("Branch: %" PRIu64 "\n", (num_branches + 1));
//            #endif
            
            bool prediction = predictor->predict(&branch);
            branchsim_update_stats(prediction, &branch, &sim_stats);
            predictor->update_predictor(&branch);

            num_branches++;
        }
    }

    // Function cleanup final statistics
    branchsim_finish_stats(&sim_stats);

    // This calls the destructor of the predictor before freeing the allocated memory
    delete predictor;

    fclose(trace);

    print_sim_output(&sim_stats);

    return 0;
}

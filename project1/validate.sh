#!/bin/bash
set -e

student_stat_dir=student_outs
spotlight_benchmark=gcc
default_benchmarks=( gcc leela linpack matmul_naive matmul_tiled mcf )
config_flags_l1=
config_flags_l1_s='-c 14 -s 4'
config_flags_l1_f='-c 10 -s 4'
config_flags_l1_vipt='-v'

banner() {
    local message=$1
    printf '%s\n' "$message"
    printf "==================================================================="
    # yes = | head -n ${#message} | tr -d '\n'
    printf '\n'
}

student_stat_path() {
    local config=$1
    local benchmark=$2

    printf '%s' "${student_stat_dir}/${config}_${benchmark}.out"
}

ta_stat_path() {
    local config=$1
    local benchmark=$2

    printf '%s' "ref_outs/${config}_${benchmark}.out"
}

human_friendly_flags() {
    local config=$1

    local config_flags_var=config_flags_$config
    local flags="${!config_flags_var}"
    if [[ -n $flags ]]; then
        printf '%s' "$flags"
    else
        printf '(none)'
    fi
}

generate_stats() {
    local config=$1
    local benchmark=$2

    local config_flags_var=config_flags_$config
    ./run.sh ${!config_flags_var} <"traces/$benchmark.trace" >"$(student_stat_path "$config" "$benchmark")"
}

generate_stats_and_diff() {
    local config=$1
    local benchmark=$2

    printf '==> Running %s...\n' "$benchmark"
    generate_stats "$config" "$benchmark"
    if diff -u "$(ta_stat_path "$config" "$benchmark")" "$(student_stat_path "$config" "$benchmark")"; then
        printf 'Matched!\n\n'
    else
        printf '\nPlease examine the differences printed above. Benchmark: %s. Config name: %s. Flags to cachesim used: %s\n\n' "$benchmark" "$config" "$(human_friendly_flags "$config")"
    fi
}

main() {
    mkdir -p "$student_stat_dir"

    make clean && make

    banner "Testing default configurations (L1 PIPT)..."
    for benchmark in "${default_benchmarks[@]}"; do
        generate_stats_and_diff l1 "$benchmark"
    done

    banner "Testing set associative configurations (L1 PIPT)..."
    for benchmark in "${spotlight_benchmark}"; do
        generate_stats_and_diff l1_s "$benchmark"
    done

    banner "Testing fully associative configurations... (L1 PIPT)"
    for benchmark in "${spotlight_benchmark}"; do
        generate_stats_and_diff l1_f "$benchmark"
    done

    banner "Testing default configurations (L1 VIPT)..."
    for benchmark in "${default_benchmarks[@]}"; do
        generate_stats_and_diff l1_vipt "$benchmark"
    done
}

main

#!/bin/bash
set -e

student_stat_dir=student_outs
benchmarks=( gcc mcf perlbench x264 )
configs=( yp_default yp_big perc_default perc_big t0 t1 t2 t3 )

flags_yp_default='-M 1'
flags_yp_big='-M 1 -P 16 -L 16'
flags_perc_default='-M 2'
flags_perc_big='-M 2 -N 12 -G 59'
flags_t0='-M 3 -C 0'
flags_t1='-M 3 -C 1'
flags_t2='-M 3 -C 2'
flags_t3='-M 3 -C 3'

banner() {
    local message=$1
    printf '%s\n' "$message"
    echo "============================="
    # yes = | head -n ${#message} | tr -d '\n'
    printf '\n'
}

student_stat_path() {
    local config=$1
    local benchmark=$2

    printf '%s' "${student_stat_dir}/${config}_${benchmark}.10m.out"
}

ta_stat_path() {
    local config=$1
    local benchmark=$2

    printf '%s' "ref_outs/${config}_${benchmark}.10m.out"
}

benchmark_path() {
    benchmark=$1
    printf '%s' "traces/$benchmark.10m.br.trace"
}

generate_stats() {
    local config=$1
    local benchmark=$2

    local flags_var=flags_$config
    bash run.sh ${!flags_var} -I "$(benchmark_path "$benchmark")" >"$(student_stat_path "$config" "$benchmark")"
}

generate_stats_and_diff() {
    local config=$1
    local benchmark=$2

    printf '==> Running %s... ' "$benchmark"
    generate_stats "$config" "$benchmark"
    if diff -u "$(ta_stat_path "$config" "$benchmark")" "$(student_stat_path "$config" "$benchmark")"; then
        printf 'Matched!\n'
    else
        local flags_var=flags_$config
        printf '\nPlease examine the differences printed above. Benchmark: %s. Config name: %s. Flags to branchsim used: %s\n\n' "$benchmark" "$config" "${!flags_var} -I $(benchmark_path "$benchmark")"
    fi
}

main() {
    mkdir -p "$student_stat_dir"
    # This is a little hacky, but it makes validation ~3x as fast:
    # around ~2min instead of ~6 min
    make clean
    make FAST=1

    if [[ $# -gt 0 ]]; then
        local use_configs=( "$@" )

        for config in "${use_configs[@]}"; do
            local flags_var=flags_$config
            if [[ -z ${!flags_var} ]]; then
                printf 'Unknown configuration %s. Available configurations:\n' "$config"
                printf '%s\n' "${configs[@]}"
                return 1
            fi
        done
    else
        local use_configs=( "${configs[@]}" )
    fi

    local first=1
    for config in "${use_configs[@]}"; do
        if [[ $first -eq 0 ]]; then
            printf '\n'
        else
            local first=0
        fi

        banner "Testing "$config"..."
        for benchmark in "${benchmarks[@]}"; do
            generate_stats_and_diff "$config" "$benchmark"
        done
    done
}

main "$@"

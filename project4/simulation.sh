#!/bin/bash
set -e

exe=./dirsim
student_stat_dir=simulation_outs
traces=( experiment1 experiment2 experiment3 experiment4 experiment5  )
protocols=( MSI MESI MOSI MESIF MOESIF )
cores=( 4 8 16 )

banner() {
    local message=$1
    printf '%s\n' "$message"
    yes = | head -n ${#message} | tr -d '\n'
    printf '\n'
}

student_stat_path() {
    local protocol=$1
    local trace=$2

    printf '%s' "${student_stat_dir}/${protocol}_${trace}.out"
}

ta_stat_path() {
    local protocol=$1
    local trace=$2

    printf '%s' "ref_outs/${protocol}_${trace}.out"
}

trace_path() {
    trace=$1
    printf '%s' "experiments/${trace}"
}

generate_stats() {
    local protocol=$1
    local trace=$2
    local n_procs=$3

    if ! "$exe" -p "$protocol" -n "$n_procs" -t "$(trace_path "$trace")" &>"$(student_stat_path "$protocol" "$trace")"; then
        printf 'Exited with nonzero status code. Please check the log file %s for details\n' "$(student_stat_path "$protocol" "$trace")"
    fi
}

generate_stats_and_diff() {
    local protocol=$1
    local trace=$2
    local n_procs=$3

    printf '==> Running %s core %d... \n' "$trace" "$n_procs"
    generate_stats "$protocol" "$trace" "$n_procs"
}

main() {
    mkdir -p "$student_stat_dir"

    if [[ $# -gt 0 ]]; then
        local use_protocols=( "$@" )

        for protocol in "${use_protocols[@]}"; do
            local found=0
            # My grandpa says he can stop drinking whenever he wants, and I say
            # the same thing about O(n^2) algorithms
            for known_protocol in "${protocols[@]}"; do
                if [[ $known_protocol = $protocol ]]; then
                    found=1
                    break
                fi
            done

            if [[ $found -eq 0 ]]; then
                printf 'Unknown protocol %s. Available protocols:\n' "$protocol"
                printf '%s\n' "${protocols[@]}"
                return 1
            fi
        done
    else
        local use_protocols=( "${protocols[@]}" )
    fi

    local first=1
    for protocol in "${use_protocols[@]}"; do
        if [[ $first -eq 0 ]]; then
            printf '\n'
        else
            local first=0
        fi

        banner "Simulating $protocol..."

        for trace in "${traces[@]}"; do
            if [[ $trace == "experiment1" ]]; then
                local core=4
            elif [[ $trace == "experiment2" ]]; then
                local core=4
            elif [[ $trace == "experiment3" ]]; then
                local core=8
            elif [[ $trace == "experiment4" ]]; then
                local core=16
            elif [[ $trace == "experiment5" ]]; then
                local core=8
            fi
            generate_stats_and_diff "$protocol" "$trace" "$core"
        done
    done
}

main "$@"

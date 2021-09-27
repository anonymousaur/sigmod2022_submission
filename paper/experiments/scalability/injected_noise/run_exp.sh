#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/synthetic/injected_noise
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/scalability/injected_noise
SUFFIXES=( "f0.00" "f0.20" "f0.40" "f0.60" "f0.80" "f1.00" )
ALPHAS=( 1 )

mkdir -p results raw_results

function join_by { local IFS="$1"; shift; echo "$*"; }


for s in "${SUFFIXES[@]}"
do
    binary=$BASE_DIR/laplace_injected_noise_${s}.bin \
    #binary=$BASE_DIR/injected_noise_data_sort_octree_0_4_$s.bin
    
    $BINARY_DIR/run_mapped_correlation_index.sh 2 injected_noise_secondary_${s} \
        $binary \
        $EXP_DIR/queries/queries_0_s01.dat \
        $EXP_DIR/indexes/index_secondary_$s.build

    $BINARY_DIR/run_mapped_correlation_index.sh 2 injected_noise_hermit_${s} \
        $binary \
        $EXP_DIR/queries/queries_0_s01.dat \
        $EXP_DIR/indexes/index_hermit_$s.build

    for a in "${ALPHAS[@]}"
    do
        # Ours
        $BINARY_DIR/run_mapped_correlation_index.sh 2 injected_noise_a${a}_${s} \
            $binary \
            $EXP_DIR/queries/queries_0_s01.dat \
            $EXP_DIR/indexes/index_a${a}_$s.build

    done
done

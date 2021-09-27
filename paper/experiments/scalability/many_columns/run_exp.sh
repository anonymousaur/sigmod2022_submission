#!/bin/bash
set -x
BASE_DIR=/home/ubuntu/correlations/continuous/synthetic/injected_noise
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/scalability/many_columns
CORRIX_COLS=( 45 )
#SEC_COLS=(  )
ALPHAS=( 1 )

mkdir -p results raw_results

function join_by { local IFS="$1"; shift; echo "$*"; }

binary=$BASE_DIR/laplace_injected_noise_f0.20.bin \

for c in "${SEC_COLS[@]}"
do
    #binary=$BASE_DIR/injected_noise_data_sort_octree_0_4_$s.bin
    
    $BINARY_DIR/run_inflated_correlation_index.sh $c many_buckets_secondary_c${c} \
        $binary 2 \
        $EXP_DIR/queries/queries_c${c}_s01.dat \
        $EXP_DIR/indexes/index_secondary_c${c}.build
done

for c in "${CORRIX_COLS[@]}"
do
    for a in "${ALPHAS[@]}"
    do
        # Ours
        $BINARY_DIR/run_inflated_correlation_index.sh $c many_buckets_a${a}_c${c} \
            $binary 2 \
            $EXP_DIR/queries/queries_c${c}_s01.dat \
            $EXP_DIR/indexes/index_a${a}_c${c}.build

    done
done

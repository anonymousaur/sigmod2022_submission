#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/synthetic/normal
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/scalability/gaussian_noise
SUFFIXES=( "f40" "f50" )
ALPHAS=( 1 )
queries=(
    $EXP_DIR/queries/queries_0_s001.dat 
    $EXP_DIR/queries/queries_0_s01.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"


for s in "${SUFFIXES[@]}"
do
    binary=$BASE_DIR/gaussian_noise_$s.bin

    # Octree with secondary index
    $BINARY_DIR/run_mapped_correlation_index.sh 2 gaussian_noise_secondary_${s} \
        $binary \
        $all_queries \
        $EXP_DIR/indexes/index_secondary_$s.build
    
    for a in "${ALPHAS[@]}"
    do
        # Ours
        $BINARY_DIR/run_mapped_correlation_index.sh 2 gaussian_noise_a${a}_${s} \
            $binary \
            $all_queries \
            $EXP_DIR/indexes/index_a${a}_$s.build
    done
done

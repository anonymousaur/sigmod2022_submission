#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/synthetic/target_buckets
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/scalability/target_buckets
SUFFIXES=( "mb10000" "mb30000" "mb100000" "mb3000" "mb1000" "mb300000"
        #"mb1000000"
        )
ALPHAS=( 1 )
queries=(
    $BASE_DIR/queries/queries_0_s00001.dat
    $BASE_DIR/queries/queries_0_s00003.dat
#    $BASE_DIR/queries/queries_0_s0001.dat
#    $BASE_DIR/queries/queries_0_s0003.dat
#    $BASE_DIR/queries/queries_0_s001.dat 
#    $BASE_DIR/queries/queries_0_s003.dat 
#    $BASE_DIR/queries/queries_0_s01.dat
#    $BASE_DIR/queries/queries_0_s03.dat
#    $BASE_DIR/queries/queries_0_s1.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"


for s in "${SUFFIXES[@]}"
do
    binary=$BASE_DIR/laplace_injected_noise_f0.20.bin

    for a in "${ALPHAS[@]}"
    do
        # Ours
        $BINARY_DIR/run_mapped_correlation_index.sh 2 target_buckets_a${a}_${s} \
            $binary \
            $all_queries \
            $EXP_DIR/indexes/index_a${a}_$s.build
    done
done

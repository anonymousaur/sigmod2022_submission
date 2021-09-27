#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/wise
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/punchline/wise
SUFFIXES=( "btree0" )
ALPHAS=( 1 )
queries=(
    #$EXP_DIR/queries/queries_10_11_12_s0001.dat
    $EXP_DIR/queries/queries_small_2_3_4_5_s001.dat
    $EXP_DIR/queries/queries_small_2_3_4_5_s01.dat
    #$EXP_DIR/queries/queries_10_11_12_s05.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"

for s in "${SUFFIXES[@]}"
do
    binary=$BASE_DIR/wise_sort_$s.bin
    
    $BINARY_DIR/run_mapped_correlation_index.sh 15 wise_small_a1_${s} \
        $binary \
        $all_queries \
        $EXP_DIR/indexes/index_small_a1_$s.build
    
done 

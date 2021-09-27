#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/wise
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/punchline/wise
SUFFIXES=( "btree9" "octree_0_1_9_p10000" )
ALPHAS=( 0 0.2 0.5 1 2 5 10 )
queries=( $EXP_DIR/queries/queries_10_11_s0001.dat 
#    $EXP_DIR/queries/queries_4_5_s001.dat
#    $EXP_DIR/queries/queries_4_5_s01.dat
#    $EXP_DIR/queries/queries_4_5_s05.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"

for s in "${SUFFIXES[@]}"
do
    
    # secondary index
    $BINARY_DIR/run_mapped_correlation_index.sh 9 wise_secondary_truesize_${s} \
        $BASE_DIR/wise_sort_$s.bin \
        $all_queries \
        $EXP_DIR/indexes/index_secondary_$s.build
done 

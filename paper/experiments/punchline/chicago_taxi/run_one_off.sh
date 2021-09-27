#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/chicago_taxi
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/punchline/chicago_taxi
SUFFIXES=( "primary_8" )
ALPHAS=( 0.5 1 2 4 )
queries=( $EXP_DIR/queries/queries_3_4_5_s0001.dat 
#    $EXP_DIR/queries/queries_3_4_5_s001.dat
#    $EXP_DIR/queries/queries_3_4_5_s01.dat
#    $EXP_DIR/queries/queries_3_4_5_s05.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"

for s in "${SUFFIXES[@]}"
do

    $BINARY_DIR/run_mapped_correlation_index.sh 9 chicago_taxi_hermit_${s} \
        $BASE_DIR/chicago_taxi_sort_$s.bin \
        $all_queries \
        $EXP_DIR/indexes/index_hermit_$s.build
done

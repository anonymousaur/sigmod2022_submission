#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/chicago_taxi
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/punchline/chicago_taxi
SUFFIXES=( "primary_8" )
ALPHAS=( 0 0.2 0.5 1 2 5 )
queries=( $EXP_DIR/queries/queries_4_5_s0001.dat 
#    $EXP_DIR/queries/queries_4_5_s001.dat
#    $EXP_DIR/queries/queries_4_5_s01.dat
#    $EXP_DIR/queries/queries_4_5_s05.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"

# Full scan can go first
$BINARY_DIR/run_mapped_correlation_index.sh 9 chicago_taxi_full \
    $BASE_DIR/chicago_taxi_sort_primary_8.bin \
    $all_queries \
    $EXP_DIR/indexes/index_full.build

exit 0

for s in "${SUFFIXES[@]}"
do
    # CM
    $BINARY_DIR/run_mapped_correlation_index.sh 9 chicago_taxi_cm_${s} \
        $BASE_DIR/chicago_taxi_sort_$s.bin \
        $all_queries \
        $EXP_DIR/indexes/index_cm_$s.build
   
    # secondary index
    $BINARY_DIR/run_mapped_correlation_index.sh 9 chicago_taxi_secondary_${s} \
        $BASE_DIR/chicago_taxi_sort_$s.bin \
        $all_queries \
        $EXP_DIR/indexes/index_secondary_$s.build
    
    # Hermit
    $BINARY_DIR/run_mapped_correlation_index.sh 9 chicago_taxi_hermit_${s} \
        $BASE_DIR/chicago_taxi_sort_$s.bin \
        $all_queries \
        $EXP_DIR/indexes/index_hermit_$s.build

    for a in "${ALPHAS[@]}"
    do
        # Ours
        $BINARY_DIR/run_mapped_correlation_index.sh 9 chicago_taxi_a${a}_${s} \
            $BASE_DIR/chicago_taxi_sort_$s.bin \
            $all_queries \
            $EXP_DIR/indexes/index_a${a}_$s.build

    done
done

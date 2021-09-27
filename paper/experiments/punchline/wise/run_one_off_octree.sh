#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/wise
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/punchline/wise
SUFFIXES=( "octree_0_1_9_p10000" )
ALPHAS=( 0.2 0.5 1 )
queries=(
    $EXP_DIR/queries/queries_10_11_12_s0001.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"

for s in "${SUFFIXES[@]}"
do
    binary=$BASE_DIR/wise_sort_btree9.bin
    
    $BINARY_DIR/run_mapped_correlation_index.sh 15 wise_octree_base_${s} \
        $binary \
        $all_queries \
        $EXP_DIR/indexes/index_octree_base_$s.build

#    for a in "${ALPHAS[@]}"
#    do
#        # Ours
#        $BINARY_DIR/run_mapped_correlation_index.sh 15 wise_a${a}_${s} \
#            $binary \
#            $all_queries \
#            $EXP_DIR/indexes/index_a${a}_$s.build
#    done
done

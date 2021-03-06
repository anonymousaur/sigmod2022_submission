#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/wise
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/punchline/wise
SUFFIXES=( "btree4" )
ALPHAS=( 0.2 0.5 1 )
queries=(
    $EXP_DIR/queries/queries_2_s0001.dat
    $EXP_DIR/queries/queries_2_s001.dat
    $EXP_DIR/queries/queries_2_s01.dat
    $EXP_DIR/queries/queries_2_s05.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"

# Full scan can go first
$BINARY_DIR/run_mapped_correlation_index.sh 15 wise_full \
    $BASE_DIR/wise_sort_btree9.bin \
    $all_queries \
    $EXP_DIR/indexes/index_full.build

for s in "${SUFFIXES[@]}"
do
    binary=$BASE_DIR/wise_sort_$s.bin
    
    # CM
    $BINARY_DIR/run_mapped_correlation_index.sh 15 wise_cm_${s} \
        $binary \
        $all_queries \
        $EXP_DIR/indexes/index_cm_$s.build
   
    # Octree with secondary index
    $BINARY_DIR/run_mapped_correlation_index.sh 15 wise_secondary_${s} \
        $binary \
        $all_queries \
        $EXP_DIR/indexes/index_secondary_$s.build

    $BINARY_DIR/run_mapped_correlation_index.sh 15 wise_hermit_${s} \
        $binary \
        $all_queries \
        $EXP_DIR/indexes/index_trs_$s.build

    for a in "${ALPHAS[@]}"
    do
        # Ours
        $BINARY_DIR/run_mapped_correlation_index.sh 15 wise_a${a}_${s} \
            $binary \
            $all_queries \
            $EXP_DIR/indexes/index_a${a}_$s.build
    done
done

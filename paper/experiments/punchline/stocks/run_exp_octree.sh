#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/stocks
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/punchline/stocks
SUFFIXES=( "0_4_p10000" )
ALPHAS=( 0 0.2 0.5 1 2 5 )
queries=( $EXP_DIR/queries/queries_2_3_5_s0001.dat 
    $EXP_DIR/queries/queries_2_3_5_s001.dat
    $EXP_DIR/queries/queries_2_3_5_s01.dat
    $EXP_DIR/queries/queries_2_3_5_s05.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"


for s in "${SUFFIXES[@]}"
do
    binary=$BASE_DIR/stocks_data_sort_$s.bin
    #binary=$BASE_DIR/stocks_data_sort_octree_0_4_$s.bin
   
    
    $BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_hermit_${s} \
        $binary \
        $all_queries \
        $EXP_DIR/indexes/index_hermit_$s.build
    
    # Octree with secondary index
    $BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_secondary_${s} \
        $binary \
        $all_queries \
        $EXP_DIR/indexes/index_secondary_$s.build
    
    # CM
    $BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_cm_${s} \
        $binary \
        $all_queries \
        $EXP_DIR/indexes/index_cm_$s.build

#    # Octree full
#    $BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_octree_${s} \
#        $binary \
#        $all_queries \
#        $EXP_DIR/indexes/index_octree_$s.build

    # Do these first so we can confirm if anything is wrong 
    for a in "${ALPHAS[@]}"
    do
        # Ours
        $BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_a${a}_${s} \
            $binary \
            $all_queries \
            $EXP_DIR/indexes/index_a${a}_$s.build
    done
    
    
done

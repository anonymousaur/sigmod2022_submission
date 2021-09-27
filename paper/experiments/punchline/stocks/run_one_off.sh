#!/bin/bash

BASE_DIR=/home/ubuntu/correlations/continuous/stocks
BINARY_DIR=/home/ubuntu/correlations/cxx
EXP_DIR=/home/ubuntu/correlations/paper/experiments/punchline/stocks
SUFFIXES=( "btree4" )
ALPHAS=( 1 ) # 0.5 0 2 5 10 ) 
queries=( $EXP_DIR/queries/queries_2_3_5_s001.dat 
#    $EXP_DIR/queries/queries_2_3_5_s001.dat
#    $EXP_DIR/queries/queries_2_3_5_s01.dat
#    $EXP_DIR/queries/queries_2_3_5_s05.dat
    )
function join_by { local IFS="$1"; shift; echo "$*"; }

all_queries=`join_by , "${queries[@]}"`
echo "Querying workloads: $all_queries"


for s in "${SUFFIXES[@]}"
do
    binary=$BASE_DIR/stocks_data_sort_$s.bin
        
    $BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_octree_base \
            $binary \
            $all_queries \
            $EXP_DIR/indexes/index_octree_base_0_1_4_p10000.build

#    for a in "${ALPHAS[@]}"
#    do
        # Ours
#        $BINARY_DIR/run_mapped_correlation_index.sh 7 stocks_a${a}_compressed_$s \
#            $binary \
#            $all_queries \
#            $EXP_DIR/indexes/index_a${a}_$s.build
#    done
done

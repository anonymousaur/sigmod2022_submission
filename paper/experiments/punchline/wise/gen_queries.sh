#!/bin/bash
set -x 

DIR=/home/ubuntu/correlations/continuous
QUERY_DIR=/home/ubuntu/correlations/paper/experiments/punchline/wise/queries
mkdir -p $QUERY_DIR

SELS=( 0001 001 01 05 )
DIMS=( 2 3 9 )
AGG1=( 2 3 9 )

for s in "${SELS[@]}"
do
    for d in "${DIMS[@]}"
    do
        output=$QUERY_DIR/queries_${d}_s${s}.dat
        if [ ! -f "$output" ]; then 
            python $DIR/gen_workload.py --dataset $DIR/wise/wise_sort_btree9.bin --dim 15 --dtype int \
                --nqueries 1000 --filter-dims $d --selectivities 0.$s --distribution equiselective \
                --output $output
        fi
    done

    agg=""
    suffix=""
    for ag in "${AGG1[@]}"
    do
        queryfile=$QUERY_DIR/queries_${ag}_s${s}.dat
        agg="${agg} ${queryfile}"
        suffix="${suffix}_${ag}"
    done
    python $DIR/shuffle_queries.py --queries $agg \
        --cap 1000 --output $QUERY_DIR/queries${suffix}_s$s.dat

done



#!/bin/bash
set -x 

DIR=/home/ubuntu/correlations/continuous
QUERY_DIR=/home/ubuntu/correlations/paper/experiments/punchline/stocks/queries
mkdir -p $QUERY_DIR

SELS=( 01 05 1 )
AGG=( 0 2 4 )

for s in "${SELS[@]}"
do

    agg=""
    suffix=""
    for ag in "${AGG[@]}"
    do
        queryfile=$QUERY_DIR/queries_${ag}_s${s}.dat
        agg="${agg} ${queryfile}"
        suffix="${suffix}_${ag}"
    done
    python $DIR/shuffle_queries.py --queries $agg \
        --cap 1000 --output $QUERY_DIR/queries${suffix}_s$s.dat

done



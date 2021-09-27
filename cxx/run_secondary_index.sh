#!/bin/bash

NAME=$1
DATASET=$2
WORKLOAD=$3

if [ -z "$WORKLOAD" ];
then
    echo "Requires 3 arguments"
    exit 0
fi

./build9/run_secondary_index \
    --name=$NAME \
    --dataset=$DATASET \
    --workload=$WORKLOAD \
    --visitor=sum \
    --mapped-dim=5 \
    --target-dim=3 \
    --save=results/$NAME.dat


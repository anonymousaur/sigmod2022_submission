#!/bin/bash

set -x

batch=10000000
start_size=50000000

binary=/home/ubuntu/correlations/cxx/build2/run_mapped_correlation_index_inserts

FRACS=( 0.01 0.20 0.50 )
UPDATE_SIZES=( 1 2 5 10 25 )

for f in "${FRACS[@]}"
do

dataset=injected_noise_f${f}.bin
spec=indexes/index_inserts_f${f}.build

for s in "${UPDATE_SIZES[@]}"
do
    $binary \
        --name=inserts_seq_init50M_batch${s}M_f${f} \
        --dataset=$dataset \
        --insert-batch-size=${s}000000 \
        --indexer-spec=$spec \
        --initial-size=50000000 \
        --save=results/inserts_seq_init50M_batch${s}M_f${f}.dat > raw_results/inserts_seq_init50M_batch${s}_${f}.output
done

#INITS=( 10 20 30 40 50 60 70 80 90 100 )
#
#for s in "${INITS[@]}"
#do
#    $binary \
#        --name=inserts_init${s}M_f${f} \
#        --dataset=$dataset \
#        --insert-batch-size=100000000 \
#        --indexer-spec=$spec \
#        --initial-size=${s}000000 \
#        --save=results/inserts_init${s}M_f${f}.dat > raw_results/inserts_init${s}M_${f}.output
#done

done

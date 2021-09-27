#!/bin/bash

python run_chicago_taxi_outliers.py \
        --target flood_0_2_8 \
        --method octree

python run_chicago_taxi_outliers.py \
        --target flood_0_2_8 \
        --method isoforest

python run_chicago_taxi_outliers.py \
        --target octree_0_2_8_p10000 \
        --method isoforest

python run_chicago_taxi_outliers.py \
        --target octree_0_2_8_p10000 \
        --method flood


#!/bin/bash

curdir=`pwd`

#cd $GENDIR
#python run_wise_data.py octree_0_1_9_p1000
#python run_wise_data.py octree_0_1_9_p10000
#python run_wise_data.py btree9
#
#python run_chicago_taxi_data.py primary_8
#cd $curdir
#
#echo "===================="
#echo "===================="
#echo

cd punchline/chicago_taxi
./run_exps_btree.sh
./run_exps_octree.sh

echo "===================="
echo "===================="
echo
cd $curdir

cd punchline/wise
./run_exp_btree.sh
./run_exp_octree.sh
cd $curdir

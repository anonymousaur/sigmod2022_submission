import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
import sys
import os
sys.path.insert(1, '/home/ubuntu/correlations/paper/plot_scripts')
from plot_utils import *
from plot_target_buckets import *

DATASET = "injected_noise"
all_results = \
        parse_results('/home/ubuntu/correlations/paper/experiments/scalability/target_buckets/results')

plot_buckets(DATASET, all_results, "plots/target_buckets.png")



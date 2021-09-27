import os
import sys
sys.path.insert(1, '/home/ubuntu/correlations/paper/plot_scripts')
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from plot_utils import *
from breakdown_by_query import * 

def plot_octree_full(corrix_raw, octree_raw, outbase):
    matplotlib.rcParams['figure.figsize'] = [15,7]
    plt.rc('font', size=28)          # controls default text sizes
    plt.rc('axes', titlesize=36)     # fontsize of the axes title
    plt.rc('axes', labelsize=28)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=22)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=22)    # fontsize of the tick labels
    plt.rc('legend', fontsize=20)    # legend fontsize
    
    corrix_times = get_results_for_workload(corrix_raw)
    octree_times = get_results_for_workload(octree_raw)

    def sel_suffix(workload):
        return workload.split('_')[-1].split('.dat')[0]
    
    def perf_by_dims(raw_res):
        dim_split_by_workload = {}
        for wf, stats in raw_res.items():
            times_by_dim = defaultdict(list)
            for dims, stat in zip(workload_dimensions(wf), stats):
                assert len(dims) == 1
                d = dims[0]
                times_by_dim[d].append(stat["time"])

            avg_time_by_dim = {}
            for d, times in times_by_dim.items():
                avg_time_by_dim[d] = np.mean(times)
            dim_split_by_workload[sel_suffix(wf)] = avg_time_by_dim
        return dim_split_by_workload



    corrix_breakdown = perf_by_dims(corrix_times)
    octree_breakdown = perf_by_dims(octree_times)
    columns = {
            0: "Start time",
            1: "End time",
            2: "Duration",
            3: "Distance",
            4: "Metered fare",
            5: "Tips",
            8: "Total fare"
            }

    base_columns = [0, 2, 8]
    other_columns = [1, 3, 4, 5]

    width = 0.3
    # We'll leave a column gap between them
    x = np.arange(len(columns), dtype='float')
    x[len(base_columns):] += 1

    for wf in corrix_breakdown.keys():
        plt.clf()
        sel = sel_suffix(wf)
        corrix = corrix_breakdown[wf]
        octree = octree_breakdown[wf]
        plt.bar(x-width/2, [corrix[c] for c in base_columns+other_columns], width=width,
                color=COLORS["corrix"], label=BASELINE_NAMES["corrix"])
        plt.bar(x+width/2, [octree[c] for c in base_columns+other_columns], width=width,
                color=COLORS["octree"], label=BASELINE_NAMES["octree"])
        plt.xticks(x, [columns[x] for x in base_columns+other_columns], rotation='20')
        plt.ylabel('Avg Query Time (ms)')
        plt.axvline(x[2]+1, linestyle='--') 
        t1 = plt.text(1./x[-1], 1.03, "Host Columns", transform=plt.gca().transAxes)
        t2 = plt.text((x[-1]-3)/x[-1], 1.03, "Target Columns", transform=plt.gca().transAxes)
        plt.legend(loc="upper left")
        plt.savefig(outbase + '_' + sel + ".png", bbox_inches='tight', pad_inches=0,
                bbox_extra_artists=(t1, t2))

        
corrix_raw = 'raw_results/chicago_taxi_a1_all_queries_octree_0_2_8_p10000.output'
octree_raw = 'raw_results/chicago_taxi_octree_full_queries_octree_0_2_8_p10000.output'
plot_octree_full(corrix_raw, octree_raw, 'plots/octree_full_comparison')





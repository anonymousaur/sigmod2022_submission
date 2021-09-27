import os
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
sys.path.insert(1, '/home/ubuntu/correlations/paper/plot_scripts')
from plot_utils import *
from collections import defaultdict

DATASET="injected_noise"

def plot_creation(results, outfile, plot='updates'):
    matplotlib.rcParams['figure.figsize'] = [10, 7]
    plt.rc('font', size=36)          # controls default text sizes
    plt.rc('axes', titlesize=36)     # fontsize of the axes title
    plt.rc('axes', labelsize=24)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=22)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=22)    # fontsize of the tick labels
    plt.rc('legend', fontsize=20)    # legend fontsize

    def noise_frac(result):
        return float(result["name"].split('_f')[-1])

    def insert_size(result):
        return int(result["insert_size"])

    def sort_by_size(results):
        ss = defaultdict(list)
        for r in results:
            ss[insert_size(r)].append(r)
        
        matches = []
        for k in sorted(ss.keys()):
            matches.extend(get_latest(ss[k]))
        return matches
    plt.clf()

    secondary = match(results, lambda r: "secondary" in r["name"], clean=False)
    corrix = match(results, lambda r: "secondary" not in r["name"], clean=False)
    print(len(results), len(secondary), len(corrix)) 
    secondary_noise_split = defaultdict(list)
    corrix_noise_split = defaultdict(list)
    for s in secondary:
        secondary_noise_split[noise_frac(s)].append(s)
    for c in corrix:
        corrix_noise_split[noise_frac(c)].append(c)

    labels = None
    for k, v in secondary_noise_split.items():
        secondary_noise_split[k] = sort_by_size(v)
    for k, v in corrix_noise_split.items():
        corrix_noise_split[k] = sort_by_size(v)
    
    labels = [insert_size(c) for c in corrix_noise_split[0.2]]
    print(labels)

    x = np.arange(len(labels))
    width=0.2
    FRAC_ALPHAS = {
            0.01: 1.0,
            0.20: 0.6,
            0.50: 0.3
            }

    for i, k in enumerate(sorted(corrix_noise_split.keys())):
        c = corrix_noise_split[k]
        times = np.array([float(r["insert_time_ms"]) / 1e3 for r in c])

        plt.bar(x-(2-i)*width, times, width=width, color=COLORS["corrix"], alpha=FRAC_ALPHAS[k],
                label = "%s f=%d%%" % (BASELINE_NAMES["corrix"], int(100*k)))
    for i, k in enumerate(sorted(secondary_noise_split.keys())):
        s = secondary_noise_split[k]
        times = [float(r["insert_time_ms"]) / 1e3 for r in s]
        plt.bar(x+width, times, width=width, color=COLORS["secondary"], alpha=FRAC_ALPHAS[k],
                label = "%s" % BASELINE_NAMES["secondary"])
    
    def fmt_labels(sizes):
        return ["%d" % (s/1000000) for s in sizes]

    plt.xticks(x, fmt_labels(labels))
    plt.ylabel("Creation Time (s)")
    plt.xlabel("Table Size (millions of records)")
    plt.legend()
    plt.savefig(outfile + '_time.png', bbox_inches='tight', pad_inches=0)

    plt.clf()
    for i, k in enumerate(sorted(corrix_noise_split.keys())):
        c = corrix_noise_split[k]
        sizes = np.array([float(r["index_size"]) / 1e9 for r in c])
        plt.bar(x-(2-i)*width, sizes, width=width, color=COLORS["corrix"], alpha=FRAC_ALPHAS[k],
                label = "%s f=%d%%" % (BASELINE_NAMES["corrix"], int(100*k)))
    for i, k in enumerate(sorted(secondary_noise_split.keys())):
        s = secondary_noise_split[k]
        sizes = [float(r["index_size"]) / 1e9 for r in s]
        plt.bar(x+width, sizes, width=width, color=COLORS["secondary"], alpha=FRAC_ALPHAS[k],
                label = "%s" % BASELINE_NAMES["secondary"])
    plt.legend()
    plt.xlabel("Table Size (millions of records)")
    plt.ylabel("Size (GB)")
    plt.xticks(x, fmt_labels(labels))
    plt.savefig(outfile + '_sizes.png', bbox_inches="tight", pad_inches = 0) 


def plot_updates(results, outfile, plot='updates'):
    matplotlib.rcParams['figure.figsize'] = [10, 7]
    plt.rc('font', size=36)          # controls default text sizes
    plt.rc('axes', titlesize=36)     # fontsize of the axes title
    plt.rc('axes', labelsize=24)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=22)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=22)    # fontsize of the tick labels
    plt.rc('legend', fontsize=20)    # legend fontsize

    def noise_frac(result):
        return float(result["name"].split('_f')[-1])

    def insert_size(result):
        return int(result["update_size"])

    def sort_by_size(results):
        ss = defaultdict(list)
        for r in results:
            ss[insert_size(r)].append(r)
        
        matches = []
        for k in sorted(ss.keys()):
            matches.extend(get_latest(ss[k]))
        return matches
    plt.clf()

    secondary = match(results, lambda r: "secondary" in r["name"], clean=False)
    corrix = match(results, lambda r: "seq" not in r["name"] and "secondary" not in r["name"], clean=False)
    corrix_seq = match(results, lambda r: "seq" in r["name"] and "secondary" not in r["name"], clean=False)
    print(len(results), len(secondary), len(corrix), len(corrix_seq)) 
    secondary_noise_split = defaultdict(list)
    corrix_noise_split = defaultdict(list)
    corrix_seq_noise_split = defaultdict(list)
    for s in secondary:
        secondary_noise_split[noise_frac(s)].append(s)
    for c in corrix:
        corrix_noise_split[noise_frac(c)].append(c)
    for c in corrix_seq:
        corrix_seq_noise_split[noise_frac(c)].append(c)

    labels = None
    for k, v in secondary_noise_split.items():
        secondary_noise_split[k] = sort_by_size(v)
    for k, v in corrix_noise_split.items():
        corrix_noise_split[k] = sort_by_size(v)
    for k, v in corrix_seq_noise_split.items():
        corrix_seq_noise_split[k] = sort_by_size(v)

    labels = [insert_size(c) for c in corrix_noise_split[0.2]]
    print(labels)

    x = np.arange(len(labels))
    width=0.2
    FRAC_ALPHAS = {
            0.01: 1.0,
            0.20: 0.6,
            0.50: 0.3
            }

    for i, k in enumerate(sorted(corrix_noise_split.keys())):
        c = corrix_noise_split[k]
        print(c)
        times = np.array([float(r["Update time"]) / 1e3 for r in c])
        sizes = np.array([int(r["update_size"]) / 1e6 for r in c])
        plt.bar(x-(2-i)*width, sizes/times, width=width, color=COLORS["corrix"], alpha=FRAC_ALPHAS[k],
                label = "%s f=%d%%" % (BASELINE_NAMES["corrix"], int(100*k)))
    for i, k in enumerate(sorted(secondary_noise_split.keys())):
        s = secondary_noise_split[k]
        times = np.array([float(r["Update time"]) / 1e3 for r in s])
        sizes = np.array([int(r["update_size"]) / 1e6 for r in s])
        plt.bar(x+width, sizes/times, width=width, color=COLORS["secondary"], alpha=FRAC_ALPHAS[k],
                label = "%s" % BASELINE_NAMES["secondary"])
   

    def fmt_labels(sizes):
        return ["%d" % (s/1000000) for s in sizes]

    plt.xticks(x, fmt_labels(labels))
    plt.ylabel("Insert Throughput (millions/s)")
    plt.xlabel("Insert batch size (millions of records")
    plt.title("Random Inserts")
    plt.legend()
    plt.savefig(outfile + '_random.png', bbox_inches='tight', pad_inches=0)

    plt.clf()
    for i, k in enumerate(sorted(corrix_seq_noise_split.keys())):
        c = corrix_seq_noise_split[k]
        times = np.array([float(r["Update time"]) / 1e3 for r in c])
        sizes = np.array([int(r["update_size"]) / 1e6 for r in c])
        plt.bar(x-(2-i)*width, sizes/times, width=width, color=COLORS["corrix"], alpha=FRAC_ALPHAS[k],
                label = "%s f=%d%%" % (BASELINE_NAMES["corrix"], int(100*k)))
    for i, k in enumerate(sorted(secondary_noise_split.keys())):
        s = secondary_noise_split[k]
        times = np.array([float(r["Update time"]) / 1e3 for r in s])
        sizes = np.array([int(r["update_size"]) / 1e6 for r in s])
        plt.bar(x+width, sizes/times, width=width, color=COLORS["secondary"], alpha=FRAC_ALPHAS[k],
                label = "%s" % BASELINE_NAMES["secondary"])
    #plt.legend()
    plt.xlabel("Table Size (millions of records)")
    plt.ylabel("Insert throughput (millions/s)")
    plt.title("Sequential Inserts")
    plt.xticks(x, fmt_labels(labels))
    plt.savefig(outfile + '_sequential.png', bbox_inches="tight", pad_inches = 0) 


results_dir= '/home/ubuntu/correlations/paper/experiments/inserts/results'
all_results = []
for f in os.listdir(results_dir):
    r = split_kv(os.path.join(results_dir, f))
    all_results.append(r)
insert_results = match(all_results, lambda r: 'batch' not in r['name'], clean=False)
print(len(insert_results))
plot_creation(insert_results, "plots/creation", plot='inserts')

update_results = match(all_results, lambda r: 'batch' in r['name'] and \
        ('Update time' in r), clean=False)
print(len(update_results))
plot_updates(update_results, "plots/insert", plot='updates')




        
        





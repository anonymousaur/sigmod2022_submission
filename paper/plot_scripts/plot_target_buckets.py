import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from plot_utils import *
import re
from collections import defaultdict
from matplotlib import cm
from matplotlib.ticker import FormatStrFormatter

def plot_buckets(dataset, results, outfile):
    matplotlib.rcParams['figure.figsize'] = [8, 6]
    plt.rc('font', size=32)          # controls default text sizes
    plt.rc('axes', titlesize=32)     # fontsize of the axes title
    plt.rc('axes', labelsize=24)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=20)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=20)    # fontsize of the tick labels
    plt.rc('legend', fontsize=18)    # legend fontsize
    plt.clf()


    def buckets(result):
        mb = result["name"].split('_mb')[-1]
        m = int(mb)
        return m

    def selectivity(result):
        m = re.search("_s([\d]+).dat", result["workload"])
        if m is None:
            return None
        return parse_selectivity(m.group(1))

    def latest_by_selectivity(results):
        split_by_sel = defaultdict(list)
        for r in results:
            split_by_sel[selectivity(r)].append(r)

        final_res = []
        for s, res in split_by_sel.items():
            final_res.append(get_latest(res)[0])
        return final_res

    def stringify(bucket):
        if bucket >= 1000000:
            return "%dM" % (bucket / 1000000)
        elif bucket >= 1000:
            return "%dk" % (bucket / 1000)
        else:
            return str(bucket)

    def query_time(result):
        return float(result["avg_query_time_ns"])/1e6

    def total_index_size(result):
        return float(result["index_size"])/1e6


    def size(result):
        f = result["index_spec"]
        outlier_size = 0
        for line in open(f):
            if line.strip().endswith(".outliers"):
                outlier_size = os.stat(line.strip()).st_size
        return total_index_size(result) - outlier_size/1e6

    results_by_buckets = defaultdict(list)

    for r in results:
        results_by_buckets[buckets(r)].append(r)
    print([(b, len(r)) for b, r in results_by_buckets.items()])
    
    total = len(results_by_buckets)
    cmap = cm.get_cmap('PuBuGn', 2*total)
    colors = [cmap(x) for x in np.linspace(0.3, 1, total)]
    for i, k in enumerate(sorted(results_by_buckets.keys())):
        r = results_by_buckets[k]
        latest = latest_by_selectivity(r)
        latest.sort(key=lambda res: selectivity(res))
        for item in latest:
            print(k, selectivity(item), query_time(item))
        plt.plot([selectivity(item) for item in latest], [query_time(item) for item in latest],
                '.-', markersize=8, linewidth=3,
                color=colors[i], label=stringify(k))

    plt.xlabel("Query Selectivity")
    plt.ylabel("Avg Query Time (ms)")
    plt.yscale('log')
    plt.xscale('log')
    plt.title("Performance")
    plt.legend(loc="upper left", ncol=2)
    plt.tight_layout()
    outfile_base = os.path.splitext(outfile)[0]
    plt.savefig(outfile_base + "_speed.png", pad_inches=0, bbox_inches='tight')
   
    plt.clf()
    width=0.5
    labels = []
    sizes = []
    for i, k in enumerate(sorted(results_by_buckets.keys())):
        r = results_by_buckets[k]
        latest = latest_by_selectivity(r)
        s = size(latest[0])
        latest.sort(key=lambda res: selectivity(res))
        plt.bar(i, s, color=colors[i], width=width)
        labels.append(stringify(k))
        sizes.append(s)

    plt.xlabel("# Target Buckets")
    plt.ylabel("Index Size (MB)")
    plt.title("Storage")
    plt.xticks(range(len(results_by_buckets)), labels)
    #size_ticks = np.linspace(np.min(sizes), np.max(sizes), 5)
    #plt.yticks(size_ticks, ["%.2f" % s for s in size_ticks])
    #plt.gca().yaxis.set_major_formatter(FormatStrFormatter('%.2f'))
    plt.tight_layout()
    plt.savefig(outfile_base + "_size.png", pad_inches=0, bbox_inches='tight')
    



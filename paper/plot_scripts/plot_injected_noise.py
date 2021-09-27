import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from plot_utils import *
import re

def plot_noise_benchmark(dataset, results, outbase):
    matplotlib.rcParams['figure.figsize'] = [8, 6]
    plt.rc('font', size=32)          # controls default text sizes
    plt.rc('axes', titlesize=32)     # fontsize of the axes title
    plt.rc('axes', labelsize=24)    # fontsize of the x and y labels
    plt.rc('xtick', labelsize=20)    # fontsize of the tick labels
    plt.rc('ytick', labelsize=20)    # fontsize of the tick labels
    plt.rc('legend', fontsize=18)    # legend fontsize
    plt.clf()

    def suffix(result):
        m = re.search("_f([\.\d]+)", result["name"])
        if m is None:
            return None
        return float(m.group(1))

    print('\n'.join(r["name"] for r in results))
    full = get_latest(match(results, FULL_SCAN(dataset)))
    secondary = match(results, SECONDARY(dataset))
    hermit = match(results, HERMIT(dataset))
    corrix = match(results, CORRIX(dataset))
    
    def split_by_sel(all_res):
        split_by_f = defaultdict(list)
        for r in all_res:
            s = suffix(r)
            if s is None:
                continue
            split_by_f[s].append(r)
        final_res = []
        for s in sorted(split_by_f.keys()):
            final_res.extend(get_latest(split_by_f[s]))
        return final_res
    
    secondary = split_by_sel(secondary)
    hermit = split_by_sel(hermit)
    corrix = split_by_sel(corrix)
    
    noise_frac = [suffix(r) for r in corrix]
   
    full = ([0], [5.16e8/1e6]) #get_size_speed([full[0]])
    secondary = get_size_speed(secondary, sort=False)
    hermit = get_size_speed(hermit, sort=False)
    corrix = get_size_speed(corrix, sort=False)

    print(full[1][0])
    print(corrix)
    print(secondary)
    plt.plot(noise_frac, corrix[1], LINESTYLES["corrix"], markersize=10, linewidth=3, color=COLORS["corrix"],
            label=BASELINE_NAMES["corrix"])
    plt.plot(noise_frac, hermit[1], LINESTYLES["hermit"], markersize=10, linewidth=3, color=COLORS["hermit"],
            label=BASELINE_NAMES["hermit"])
    plt.plot(noise_frac, secondary[1], LINESTYLES["secondary"], markersize=10, linewidth=3,  color=COLORS["secondary"],
            label=BASELINE_NAMES["secondary"])
    plt.axhline(full[1][0], linestyle='--', linewidth=3, color=COLORS["full"])

    plt.xlabel("Injected noise fraction")
    plt.ylabel("Avg Query Time (ms)")
    plt.yscale('log')
    plt.legend()
    plt.tight_layout()
    plt.savefig(outbase + "_query_time.png", pad_inches=0, bbox_inches='tight')

    plt.clf()
    plt.plot(noise_frac, corrix[0], LINESTYLES["corrix"], markersize=10, linewidth=3, color=COLORS["corrix"],
            label=BASELINE_NAMES["corrix"])
    plt.plot(noise_frac, hermit[0], LINESTYLES["hermit"], markersize=10, linewidth=3, color=COLORS["hermit"],
            label=BASELINE_NAMES["hermit"])
    plt.plot(noise_frac, secondary[0], LINESTYLES["secondary"], markersize=10, linewidth=3, color=COLORS["secondary"],
            label=BASELINE_NAMES["secondary"])
    plt.legend()
    plt.xlabel("Injected noise fraction")
    plt.ylabel("Size (MB)")
    plt.ylim(ymin=0)
    plt.tight_layout()
    plt.savefig(outbase + "_sizes.png", pad_inches=0, bbox_inches='tight')
    

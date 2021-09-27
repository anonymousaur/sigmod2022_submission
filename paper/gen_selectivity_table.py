import os
import sys
from plot_scripts.plot_utils import *
import numpy as np

def get_all_names(dset, results):
    names = set()
    for n, f in FILTERS.items():
        if len(match(results, f(dset))) > 0:
            names.add(n)
    return names

def color(val):
    red_base = (179, 39, 16)
    green_base = (88, 183, 78)
    red_max = np.log2(10)
    green_max = np.log2(10)

    # A scale of 1 is the same color as ref. 0 is white.
    def lighten(ref, scale):
        print("Scale < 0:", scale)
        tlst = list(ref)
        t_mod = [(255 - (255 - t) * scale)/255 for t in tlst] 
        return tuple(t_mod)

    scaled = np.log2(val)
    if scaled < 0:
        return lighten(red_base, min(1., -scaled/red_max))
    else:
        return lighten(green_base, min(1., scaled/green_max))



def gen_for_baseline(baseline, reference, results_list):
    print_names = {
            "full": "Full Scan",
            "secondary": "Secondary B+ Tree",
            "cm": "Correlation Map",
            "octree": "Octree",
            "hermit": "Hermit",
            "corrix": "\\TheSystem"
            }

    line = print_names[baseline]
    if baseline == "corrix":
        line = '\\textbf{%s} ' % print_names[baseline]
    for n, res in results_list:
        results = match(res, FILTERS[baseline](n))
        if baseline == "corrix":
            # Get alpha = 1
            # Try alpha = 0.5 first, if it exists
            actual_results = match(results, lambda r: '_a1_' in r["name"])
            results = actual_results
        references = match(res, FILTERS[reference](n))
        for sel in ["0001", "001", "01", "05"]:
            selres = get_latest(match(results, lambda r: r["workload"].endswith("s%s.dat" % sel)))
            selref = get_latest(match(references, lambda r: r["workload"].endswith("s%s.dat" % sel)))
            if len(selres) == 0:
                line += " & "
            else:
                qs = float(selres[0]["avg_query_time_ns"])
                ref_qs = float(selref[0]["avg_query_time_ns"])
                line += "& %.3f " % (ref_qs/qs)
                c = color(ref_qs/qs)
                line += "\\cellcolor[rgb]{%f,%f,%f} " % c

    line += "\\\\"
    return line

# Results list is a list of (name, results) pairs for each dataset.
def gen_latex_table(results_list, reference):
    all_names = set()
    for n, res in results_list:
        all_names |= get_all_names(n, res)
    print(all_names)   
    all_lines = []

    ordering = ["full",  "cm", "hermit", "corrix"]
    name_list = []
    for n in ordering:
        if n in all_names:
            name_list.append(n)

    for n in name_list:
        if n == reference:
            continue
        all_lines.append(gen_for_baseline(n, reference, results_list))
    
    return '\n'.join(all_lines)

if __name__ == "__main__":
    base_dir = "/home/ubuntu/correlations/paper/experiments/punchline"
    specs = []
    if sys.argv[1] == "btree":
        specs.append(("stocks", os.path.join(base_dir, "stocks/results"), "btree4"))
        specs.append(("chicago_taxi", os.path.join(base_dir, "chicago_taxi/results"),
            "primary_8"))
        specs.append(("wise", os.path.join(base_dir, "wise/results"), "btree9"))
    elif sys.argv[1] == "octree":
        specs.append( ("stocks", os.path.join(base_dir, "stocks/results"), "0_4_p1000"))
        specs.append(("chicago_taxi", os.path.join(base_dir, "chicago_taxi/results"),
            "octree_0_2_8_p1000"))
        specs.append( ("wise", os.path.join(base_dir, "wise/results"), "octree_0_1_9_p1000"))
    elif sys.argv[1] == "flood":
        specs.append( ("stocks", os.path.join(base_dir, "stocks/results"), "flood_0_4"))
        specs.append(("chicago_taxi", os.path.join(base_dir, "chicago_taxi/results"),
            "flood"))
        specs.append( ("wise", os.path.join(base_dir, "wise/results"), "flood_0_1_9"))
    else:
        print("First argument should be 'btree', 'octree', or 'flood'")
        sys.exit(1)
    
    ds = []
    for s in specs:
        all_res = parse_results(s[1])
        res = match(all_res, lambda r: s[2] in r["name"])
        res += match(all_res, FULL_SCAN(s[0]))
        ds.append((s[0], res))
    table = gen_latex_table(ds, "secondary")
    print(table)




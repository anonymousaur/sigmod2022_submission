import os
import numpy as np
import gen_linear_map as glm

RESULTS_DIR = "/home/ubuntu/correlations/cxx/results"
OUTPUT_DIR = "/home/ubuntu/correlations/continuous"

def get_prefix(name):
    base = ""
    if name.startswith('chicago_taxi'):
        base = "chicago_taxi"
    elif name.startswith("airline"):
        base = "airline"
    elif name.startswith("msft_buildings"):
        base = "msft_buildings"
    elif name.startswith("stocks"):
        base = "stocks"
    else:
        assert (False), "name %s unrecognized" % name

    parts = name.split('.')
    prefix = "%s/%s_linear.%s" % (base, parts[0], '.'.join(parts[1:])) 
    return prefix

def load_to_match(result_file):
    f = open(result_file)
    lines = f.readlines()
    parts = lines[1].split(',')
    name = parts[1]
    dataset = parts[2]
    num_outliers = int(parts[7])
    ixr_size = int(parts[11])
    prefix = os.path.join(OUTPUT_DIR, get_prefix(name))
    if os.path.exists(prefix + ".data"):
        print("Skipping %s: output exists" % prefix)
        return

    data = np.fromfile(dataset, dtype=int).reshape(-1, 2)
    num_allowed_outliers = ixr_size / 8
    assert (num_allowed_outliers >= num_outliers)
    
    
    glm.write_linear_map(prefix, data[:,0], data[:,1], num_allowed_outliers)

results_files = os.listdir(RESULTS_DIR)
for f in sorted(results_files):
    a = f.split('.')[0].split('_')[-1]
    if not a.startswith('a'):
        print("Skipping %s" % f)
        continue
    print("Processing %s" % f)
    ff = os.path.join(RESULTS_DIR, f)
    load_to_match(ff)




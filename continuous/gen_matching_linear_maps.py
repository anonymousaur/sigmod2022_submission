import gen_linear_map as glm
import os
import argparse
import numpy as np

# For all the corrix mapping files in the given directory, generates a linear
# map with the corresponding number of outliers.

parser = argparse.ArgumentParser("")
parser.add_argument("--reference-dir",
        type=str,
        required=True,
        help="Directory with existing mappings to match")
parser.add_argument("--dataset",
        type=str,
        required=True)
parser.add_argument("--dims",
        type=int,
        required=True,
        help="Dimension of dataset")
parser.add_argument("--suffix",
        type=str,
        default="linear",
        help="Suffix to identify the generated mappings")
args = parser.parse_args()


data = np.fromfile(args.dataset, dtype=int).reshape(-1, args.dims)
print("Loaded dataset")

generated_pairs = set()
ref_files = []
for f in os.listdir(args.reference_dir):
    if "linear" in f:
        # This is a file we wrote, skip
        continue
    if not f.endswith("outliers"):
        continue
    mapping = f.split('.')[-2].split('_')
    if len(mapping) != 2:
        continue
    # Don't repeat models
    pair = (int(mapping[0]), int(mapping[1]))
    if pair in generated_pairs:
        continue
    # Only generate for corrix models
    parts = f.split('.')[0].split('_')
    for i, p in enumerate(parts):
        if p.startswith('a'):
            try:
                alpha = int(p[1:])
            except Exception as e:
                break
            # Outliers are stored in binary format and take 8 bytes each
            num_outliers = os.stat(os.path.join(args.reference_dir, f)).st_size / 8
            prefix = '_'.join(parts[:i+1]) + '_linear_' + '_'.join(parts[i+1:])
            prefix += '.%d_%d' % pair
            prefix = os.path.join(args.reference_dir, prefix)
            print ("Writing %s with %d outliers" % (prefix, num_outliers))
            glm.write_linear_map(prefix, data, pair[0], pair[1], num_outliers) 
    



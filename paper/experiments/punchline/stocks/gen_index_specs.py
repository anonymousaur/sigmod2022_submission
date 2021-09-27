import os
import sys

#suffixes = ["0_4_p10000", "btree4", "flood_0_4"]
suffixes = ["flood_0_4"]
mappings = [[(2, 4), (3, 4), (5, 4)],
        [(2, 4), (3, 4), (5, 4)]]
host_cols = [[0,4], [4]]
alphas = ["a0", "a0.2", "a0.5", "a1", "a2", "a5"]
BASE_DIR = "/home/ubuntu/correlations/continuous/stocks/compressed"
INDEX_DIR = "/home/ubuntu/correlations/paper/experiments/punchline/stocks/indexes"

corrix_mapping_file = lambda m, a, s: \
        os.path.join(BASE_DIR, f"stocks_{a}_{s}.k1.{m[0]}_{m[1]}.mapping")
corrix_outliers_file = lambda m, a, s: \
        os.path.join(BASE_DIR, f"stocks_{a}_{s}.k1.{m[0]}_{m[1]}.outliers")
corrix_targets_file = lambda m, a, s: \
        os.path.join(BASE_DIR, f"stocks_{a}_{s}.k1.{m[0]}_{m[1]}.targets")

class SpecBuilder(object):
    def __init__(self):
        self.output = ""
        self.num_indents = 0

    def shift(self, s):
        indent = '\t' * self.num_indents
        return '\n'.join(indent + st for st in s.splitlines())

    def add(self, line):
        if line.strip().startswith('}'):
            self.num_indents -= 1
        self.output += self.shift(line) + "\n"
        if line.strip().endswith('{'):
            self.num_indents += 1
        return self

    def get(self):
        assert (self.num_indents == 0)
        return self.output

    def write(self, fname):
        f = open(fname, 'w')
        f.write(self.get())
        f.close()

def get_primary(spec, s):
    if s == "0_4_p10000":
        spec.add("OctreeIndex {").add("10000").add("0 4").add("}")
    if s == "0_4_p1000":
        spec.add("OctreeIndex {").add("1000").add("0 4").add("}")
    if s == "btree4":
        spec.add("BinarySearchIndex { 4 }")
    if s == "flood_0_4":
        flood_dir = os.path.join(os.getcwd(), "flood_gen_0_4")
        assert os.path.exists(flood_dir), "Flood directory " + flood_dir + " doesn't exist"
        spec.add("FloodIndex { " + flood_dir + " }")

def gen_secondary(mps, s):
    spec = SpecBuilder().add("CompositeIndex {")
    get_primary(spec, s)
    for m in sorted(mps, key = lambda mm: mm[0]):
        spec.add("SecondaryBTreeIndex { %d }" % m[0])
    spec.add("}")
    return spec


def gen_corrix(mps, s, a):
    def comb_corr(spec, m, s, a):
        spec.add("CombinedCorrelationIndex {").add("MappedCorrelationIndex {")
        spec.add(corrix_mapping_file(m, a, s)).add(corrix_targets_file(m, a, s))
        spec.add("}")
        spec.add("BucketedSecondaryIndex {").add(str(m[0])).add(corrix_mapping_file(m, a, s))
        spec.add(corrix_outliers_file(m, a, s))
        spec.add("}").add("}")

    output = SpecBuilder().add("CompositeIndex {")
    get_primary(output, s)
    for m in sorted(mps, key = lambda mm: mm[0]):
        comb_corr(output, m, s, a)
    output.add("}")
    return output

def gen_cm(mps, s):
    return gen_corrix(mps, s, "cm")

def gen_hermit(mps, s):
    spec = SpecBuilder().add("CompositeIndex {")
    get_primary(spec, s)
    for m in sorted(mps, key = lambda mm: mm[0]):
        spec.add("TRSTreeRewriter {").add(str(m[0])).add(str(m[1])).add("2")
        spec.add("SecondaryBTreeIndex { %d }" % m[0])
        spec.add("}")
    spec.add("}")
    return spec

def gen_full():
    return SpecBuilder().add("DummyIndex { }")

def gen_octree(hosts, mps, s):
    page_size = "1000"
    if '_p' in s:
        page_size = s.split('_p')[-1]
    output = SpecBuilder().add("OctreeIndex {").add(page_size)
    all_columns = set([m[0] for m in mps] + hosts)
    all_columns = sorted(list(all_columns))
    output.add(' '.join(str(m) for m in all_columns))
    output.add('}')
    return output


# Do the generation
gen_full().write(os.path.join(INDEX_DIR, 'index_full.build'))

for (hosts, maps, suff) in zip(host_cols, mappings, suffixes):
    if 'btree' not in suff and 'flood' not in suff:
        gen_octree(hosts, maps, suff).write(os.path.join(INDEX_DIR, 'index_octree_%s.build' % suff))
    gen_secondary(maps, suff).write(os.path.join(INDEX_DIR, 'index_secondary_%s.build' % suff))
    gen_cm(maps, suff).write(os.path.join(INDEX_DIR, "index_cm_%s.build" % suff))
    gen_hermit(maps, suff).write(os.path.join(INDEX_DIR, "index_hermit_%s.build" % suff))
    for a in alphas:
        gen_corrix(maps, suff, a).write(
                os.path.join(INDEX_DIR, 'index_%s_%s.build' % (a, suff)))



import os
import sys

suffixes = [ "mb1000", "mb3000", "mb10000", "mb30000", "mb100000",
        "mb300000", "mb1000000"]
mappings = [(0,1)]
host_cols = [[1]]
alphas = [ "a1" ]
BASE_DIR = "/home/ubuntu/correlations/continuous/synthetic/target_buckets"
INDEX_DIR = "/home/ubuntu/correlations/paper/experiments/scalability/target_buckets/indexes"

corrix_mapping_file = lambda m, a, s: \
        os.path.join(BASE_DIR, f"injected_noise_{a}_{s}_f0.20.k1.{m[0]}_{m[1]}.mapping")
corrix_outliers_file = lambda m, a, s: \
        os.path.join(BASE_DIR, f"injected_noise_{a}_{s}_f0.20.k1.{m[0]}_{m[1]}.outliers")
corrix_targets_file = lambda m, a, s: \
        os.path.join(BASE_DIR, f"injected_noise_{a}_{s}_f0.20.k1.{m[0]}_{m[1]}.targets")

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
    spec.add("BinarySearchIndex { 1 }")

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

def gen_hermit(mps, s):
    spec = SpecBuilder().add("CompositeIndex {")
    get_primary(spec, s)
    for m in sorted(mps, key = lambda mm: mm[0]):
        spec.add("TRSTreeRewriter {").add(str(m[0])).add(str(m[1]))
        spec.add("SecondaryBTreeIndex { %d }" % m[0])
        spec.add("}")
    spec.add("}")
    return spec


for suff in suffixes:
    maps = mappings
    gen_secondary(maps, suff).write(os.path.join(INDEX_DIR, 'index_secondary_%s.build' % suff))
    #gen_hermit(maps, suff).write(os.path.join(INDEX_DIR, "index_hermit_%s.build" % suff))
    for a in alphas:
        gen_corrix(maps, suff, a).write(
                os.path.join(INDEX_DIR, 'index_%s_%s.build' % (a, suff)))



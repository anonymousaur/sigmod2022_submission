import os
import sys

cols = [10, 15, 20, 25, 30, 35, 40, 45, 50]
mapping = (0, 1)
alphas = ["a1"]
BASE_DIR = "/home/ubuntu/correlations/continuous/synthetic/injected_noise"
INDEX_DIR = "/home/ubuntu/correlations/paper/experiments/scalability/many_columns/indexes"

corrix_mapping_file = lambda m, a: \
        os.path.join(BASE_DIR, f"injected_noise_{a}_f0.20.k1.{m[0]}_{m[1]}.mapping")
corrix_outliers_file = lambda m, a: \
        os.path.join(BASE_DIR, f"injected_noise_{a}_f0.20.k1.{m[0]}_{m[1]}.outliers")
corrix_targets_file = lambda m, a: \
        os.path.join(BASE_DIR, f"injected_noise_{a}_f0.20.k1.{m[0]}_{m[1]}.targets")

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

def get_primary(spec):
    spec.add("BinarySearchIndex { %d }" % mapping[1])

def gen_secondary(mps, c):
    spec = SpecBuilder().add("CompositeIndex {")
    get_primary(spec)
    for i in range(c):
        if i != mapping[1]:
            spec.add("SecondaryBTreeIndex { %d }" % i)
    spec.add("}")
    return spec

def gen_corrix(mps, c, a):
    def comb_corr(spec, m, i, a):
        spec.add("CombinedCorrelationIndex {").add("MappedCorrelationIndex {")
        spec.add(corrix_mapping_file(m, a)).add(corrix_targets_file(m, a))
        spec.add("}")
        spec.add("BucketedSecondaryIndex {").add(str(i)).add(corrix_mapping_file(m, a))
        spec.add(corrix_outliers_file(m, a))
        spec.add("}").add("}")

    output = SpecBuilder().add("CompositeIndex {")
    get_primary(output)
    for i in range(c):
        if i != mapping[1]:
            comb_corr(output, mapping, i, a)
    output.add("}")
    return output

# Do the generation
for col in cols:
    suff = "c%d" % col
    maps = [mapping]
    gen_secondary(maps, col).write(os.path.join(INDEX_DIR, 'index_secondary_%s.build' % suff))
    for a in alphas:
        gen_corrix(maps, col, a).write(
                os.path.join(INDEX_DIR, 'index_%s_%s.build' % (a, suff)))



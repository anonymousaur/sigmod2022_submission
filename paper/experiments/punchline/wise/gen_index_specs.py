import os
import sys

#suffixes = ["btree9", "octree_0_1_9_p1000", "octree_0_1_9_p10000"]
#mappings = [[(10,9), (11,9)],
#            [(5, 0), (6, 0), (7, 0), (8,0), (10,9), (11,9)],
#            [(5, 0), (6, 0), (7, 0), (8,0), (10,9), (11,9)]]
suffixes = ["flood_0_1_9"]
mappings = [[(5, 0), (6, 0), (7, 0), (8,0), (10,9), (11,9)]]
octree_cols = [0, 1, 5, 6, 7, 8, 9, 10, 11]
alphas = ["a0", "a0.2", "a0.5", "a1", "a2", "a5", "a10"]
BASE_DIR = "/home/ubuntu/correlations/continuous/wise/compressed"
INDEX_DIR = "/home/ubuntu/correlations/paper/experiments/punchline/wise/indexes"

corrix_mapping_file = lambda m, a, s: \
        os.path.join(BASE_DIR, f"wise_{a}_{s}.k1.{m[0]}_{m[1]}.mapping")
corrix_outliers_file = lambda m, a, s: \
        os.path.join(BASE_DIR, f"wise_{a}_{s}.k1.{m[0]}_{m[1]}.outliers")
corrix_targets_file = lambda m, a, s: \
        os.path.join(BASE_DIR, f"wise_{a}_{s}.k1.{m[0]}_{m[1]}.targets")

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
    if s == "octree_0_1_9_p10000":
        spec.add("OctreeIndex {").add("10000").add("0 1 9").add("}")
    elif s == "octree_0_1_9_p1000":
        spec.add("OctreeIndex {").add("1000").add("0 1 9").add("}")
    elif s == "btree9":
        spec.add("BinarySearchIndex { 9 }")
    elif s == "btree4":
        spec.add("BinarySearchIndex { 4 }")
    elif s == "btree11":
        spec.add("BinarySearchIndex { 11 }")
    elif s == "flood_0_1_9":
        flood_dir = os.path.join(os.getcwd(), "flood_gen_0_1_9")
        spec.add("FloodIndex { " + flood_dir + " }")
    else:
        assert False, "Unrecognized primary index " + s

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

def gen_octree(mps, s):
    assert "btree" not in s
    page_size = s.split('_p')[-1]
    output = SpecBuilder().add("OctreeIndex {").add(page_size)
    output.add(' '.join(str(m) for m in octree_cols))
    output.add('}')
    return output


# Do the generation
gen_full().write(os.path.join(INDEX_DIR, 'index_full.build'))

for (maps, suff) in zip(mappings, suffixes):
    if 'btree' not in suff and 'flood' not in suff:
        gen_octree(maps, suff).write(os.path.join(INDEX_DIR, 'index_octree_%s.build' % suff))
    gen_secondary(maps, suff).write(os.path.join(INDEX_DIR, 'index_secondary_%s.build' % suff))
    gen_cm(maps, suff).write(os.path.join(INDEX_DIR, "index_cm_%s.build" % suff))
    gen_hermit(maps, suff).write(os.path.join(INDEX_DIR, "index_hermit_%s.build" % suff))
    for a in alphas:
        gen_corrix(maps, suff, a).write(
                os.path.join(INDEX_DIR, 'index_%s_%s.build' % (a, suff)))



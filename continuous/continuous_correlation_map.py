import numpy as np
from progress.bar import ShadyBar

class MappingBucket(object):
    def __init__(self, bstart, bend, ranges):
        self.start = bstart
        self.end = bend
        self.ranges = ranges

    def to_string(self):
        s = str(self.start) + "\t" + str(self.end) + "\t%d\n" % len(self.ranges)
        for r in self.ranges:
            s += str(r[0]) + "\t" + str(r[1]) + "\n"
        return s


class ContinuousCorrelationMap(object):
    def __init__(self):
        self.mapped_buckets = None
        self.target_buckets = None
        self.mapped_dim = None
        self.target_dims = None
        self.bucket_map = None
    
    def set_mapped_bucketer(self, mb):
        self.mapped_buckets = mb.get_ranges()[0]
        self.mapped_dim = mb.dims()[0]

    def set_target_bucketer(self, tb):
        self.target_dims = tb.dims()

    # bmap is a dictionary from map buckets to lists of target buckets
    def set_bucket_map(self, bmap):
        self.bucket_map = bmap

    # bstart and bend are the beginning (inclusive) and end (exclusive) of the mapped column.
    # Ranges is a list of [start, end) ranges as well.
    def add_mapping(self, bstart, bend, ranges):
        self.buckets.append(MappingBucket(bstart, bend, ranges))

    def write(self, fobj):
        lr = 0
        fobj.write("continuous-0\n")
        fobj.write("source\t%d\t%d\n" % (self.mapped_dim, len(self.mapped_buckets)))
        for mb in self.mapped_buckets:
            fobj.write("%d\t%s\t%s\n" % (mb[0], str(mb[1]), str(mb[2])))

        # Mapping from mapped dimension buckets to lists of target dimension buckets
        fobj.write("mapping\t%d\n" % len(self.bucket_map))
        keys = sorted(self.bucket_map.keys())
        bar = ShadyBar("Writing Phase 2", max=len(self.bucket_map))
        for k in keys:
            v = [str(val) for val in sorted(self.bucket_map[k])]
            lr += len(v)
            fobj.write("%d\t%s\n" % (k, '\t'.join(v)))
            bar.next()
        bar.finish()
        
        if len(self.bucket_map) > 0:
            print("Average # target buckets: %.02f" % (float(lr)/len(self.bucket_map)))


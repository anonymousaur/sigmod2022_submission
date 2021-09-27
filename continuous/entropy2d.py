import numpy as np
import sys
import os


class Entropy2D(object):
    def __init__(self, data, buckets1, bucket_ids2):
        b1 = np.unique(buckets1)
        b2 = np.unique(bucket_ids2)
        b1 = np.concatenate(b1, b1.max()+1)
        b2 = np.concatenate(b2, b2.max() + 1)
        self.cells = np.histogram2d(data, bucket_ids2, bins=(b1, b2))[0]

    # Given a target bucket (schema 2), what's the conditional entropy of the mapped buckets. 
    def cond_entropy(self):
        cumuls = np.sum(self.cells, axis=1)
        probs = np.zeros(self.cells.shape)
        probs = self.cells / cumuls.reshape(-1, 1)
        probs[self.cells == 0] = 1
        cuml_ent = np.sum(-probs * np.log2(probs * self.cells.shape[1]), axis=1)
        return np.mean(cuml_ent)

    def weighted_cond_entropy(self):
        cumuls = np.sum(self.cells, axis=1)
        probs = np.zeros(self.cells.shape)
        probs = self.cells / cumuls.reshape(-1, 1)
        probs[self.cells == 0] = 1
        cuml_ent = np.sum(-probs * np.log2(probs * self.cells.shape[1]), axis=1)
        return np.inner(cumuls, cuml_ent) / np.sum(cumuls)
       


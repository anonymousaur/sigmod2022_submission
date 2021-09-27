import os
import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--dir',
        required=True,
        type=str,
        help="Directory with the index specs to be validated")
args = parser.parse_args()

err = False
for f in os.listdir(args.dir):
    if not f.endswith('.build'):
        continue
    tokens = open(os.path.join(args.dir, f)).read().split()
    for t in tokens:
        if '/' in t:
            exists = os.path.exists(t)
            if not exists:
                err = True
                print("%s: Invalid parameter %s" % (f, t))

assert not err, "Found missing files."


import numpy as np
import random
import argparse

parser = argparse.ArgumentParser("Query Shuffler")
parser.add_argument("--queries",
        type=str,
        nargs="+",
        help="List of query files to shuffle")
parser.add_argument("--output",
        type=str,
        required=True,
        help="Where to write the output")
parser.add_argument("--cap",
        type=int,
        default=-1,
        help="Max number of queries to write")
args = parser.parse_args()

if len(args.queries) == 0:
    print("No queries to shuffle")
    sys.exit(0)

queries = []
cur_q = ""
for f in args.queries:
    for line in open(f):
        if line.startswith("=") and len(cur_q) > 0:
            queries.append(cur_q)
            cur_q = ""
        cur_q += line
    assert len(cur_q) > 0
    queries.append(cur_q)
    cur_q = ""

w = open(args.output, 'w')
perm = np.random.permutation(len(queries))
written = 0
for p in perm:
    w.write(queries[p])
    written += 1
    if args.cap > 0 and written >= args.cap:
        break
w.close()


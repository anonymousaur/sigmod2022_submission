import numpy as np
import argparse

# Converts plaintext query files into binary format for use with Flood / Tsunami

parser = argparse.ArgumentParser("Query Converter")
parser.add_argument("--queries",
        type=str,
        required=True,
        help="Plaintext queries to convert")
parser.add_argument('--output',
        type=str,
        required=True,
        help="Output binary file")
args = parser.parse_args()

NINF = -(1 << 63) + 1
PINF = (1 << 63) - 2

def to_binary(query_str):
    lines = query_str.splitlines()
    assert lines[0].startswith("=")
    b = np.zeros((2, len(lines)-1), dtype=int)
    for i, line in enumerate(lines[1:]):
        line = line.strip()
        if line == "none":
            b[0, i] = NINF
            b[1, i] = PINF
        else:
            r = line.split()
            assert r[0] == "ranges"
            b[0, i] = int(r[1])
            b[1, i] = int(r[2])
    return b

queries = []
cur_q = ""
for line in open(args.queries):
    if line.startswith("=") and len(cur_q) > 0:
        queries.append(cur_q)
        cur_q = ""
    cur_q += line
queries.append(cur_q)

w = open(args.output, 'wb')
for q in queries:
    w.write(to_binary(q).tobytes())
w.close()


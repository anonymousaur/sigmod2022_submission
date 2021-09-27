import run_dataset as rdset
import gen_workload as gwl
import os

args1 = {
    "name": "airline",
    "datafile": "/home/ubuntu/data/airlines/flight_data.bin",
    "ncols": 16,
    "map_spec": (1, 3600*6),
    "target_spec": [(0, 3600*6)],
    "k": 1,
    "alphas": [-1, 1, 5, 10, 15, 20, 30, 50],
    }

args2 = {
    "name": "airline",
    "datafile": "/home/ubuntu/data/airlines/flight_data.bin",
    "ncols": 16,
    "map_spec": (3, 3600*6),
    "target_spec": [(0, 3600*6)],
    "k": 1,
    "alphas": [-1, 1, 5, 10, 15, 20, 30, 50],
    }

args3 = {
    "name": "airline",
    "datafile": "/home/ubuntu/data/airlines/flight_data.bin",
    "ncols": 16,
    "map_spec": (12, 1),
    "target_spec": [(9, 1)],
    "k": 1,
    "alphas": [-1, 1, 5, 10, 15, 20, 30, 50],
    }

for args in [args1, args2, args3]:
    rdset.run(args)
    gwl.gen_from_spec(args, "uniform", 1000,
            os.path.join(args["name"], "queries_%d_w%d_uniform.dat" % args["map_spec"]))
    gwl.gen_from_spec(args, "point", 10000,
            os.path.join(args["name"], "queries_%d_point.dat" % args["map_spec"][0]))



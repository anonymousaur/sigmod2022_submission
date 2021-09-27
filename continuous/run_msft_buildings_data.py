import run_dataset as rdset
import gen_workload as gwl
import os

args = {
    "name": "msft_buildings",
    "datafile": "/home/ubuntu/data/msft_buildings/msft_buildings.bin",
    "ncols": 3,
    "map_spec": (2, 30000),
    "target_spec": [(1, 30000)],
    "k": 1,
    "alphas": [-1, 1, 5, 10, 15, 20, 30, 50],
    }

#rdset.run(args)
gwl.gen_from_spec(args, "uniform", 1000,
        os.path.join(args["name"], "queries_%d_w%d_uniform.dat" % args["map_spec"]))
gwl.gen_from_spec(args, "point", 10000,
        os.path.join(args["name"], "queries_%d_point.dat" % args["map_spec"][0]))



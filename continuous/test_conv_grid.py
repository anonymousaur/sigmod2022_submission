from stash import ConvolutionGrid
import numpy as np
import sys


def test_zeros():
    grid = [0, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1]
    g = np.array(grid).reshape((1, len(grid)))
    cg = ConvolutionGrid(g, 3)
    zl, zr = cg.zeros_left, cg.zeros_right
    zl, zr = zl.flatten(), zr.flatten()
    l = len(grid)

    zl_want = [-1, 0, -1, 2, 3, 4, 5, 5, 7, 8, 7]
    zr_want = [1, 2, 3, 4, 5, 7, 7, 10, 9, 10, 11]

    correct = True
    if not np.array_equal(zl, zl_want):
        print("Zeros left: Want", zl_want, "but got", zl)
        correct = False
    if not np.array_equal(zr, zr_want):
        print("Zeros right: Want", zr_want, "but got", zr)
        correct = False
    if correct:
        print("PASS")
    return correct

def test_benefit():
    grid = [0, 0, 2, 4, 5, 2, 0, 1, 0, 0, 1]
    g = np.array(grid).reshape((1, len(grid)))
    cg = ConvolutionGrid(g, 3)
    ben = cg.benefit
    ben = ben.flatten()
    pts_saved = np.array([13, 18, 12, 8, 8, 36, 28, 28, 14])
    pts_tot = np.array([18, 54, 99, 99, 63, 27, 9, 9, 9])
    ben_want = pts_saved - pts_tot
    
    if not np.array_equal(ben_want, ben):
        print("Benefits: want", ben_want, "but got", ben)
        print("FAIL")
    else:
        print("PASS")

def test_pop():
    grid = [0, 0, 2, 4, 5, 2, 0, 1, 0, 0, 1]
    g = np.array(grid).reshape((1, len(grid)))
    cg = ConvolutionGrid(g, 3)
    pts, b, ixs = cg.pop_best()
    
    correct=True
    if pts != 1:
        print("pop_best should return 1 point, returned", pts)
        correct = False
    if b != 19:
        print("pop_best should return benefit 19, returned", ben)
        correct = False
    if ixs[0] != 0 or (ixs[1] not in [6,7]) or (ixs[2] not in [9,10]):
        print("pop_best should return indexes (0, 6|7, 9|10) but got", ixs)
        correct = False

    # Test to see if everything was updated properly.
    new_grid = [0, 0, 2, 4, 5, 2, 0, 0, 0, 0, 1]
    if not np.array_equal(cg.grid.flatten(), new_grid):
        print("Grid not updated: want", new_grid, "but got", cg.grid.flatten())
        correct = False
    if not cg.grid_cumul[0] == np.sum(new_grid):
        print("Grid cumul not updated: want [", np.sum(new_grid), "], but got", cg.grid_cumul)
        correct = False
    zl = [-1, 0, -1, 2, 3, 4, 5, 6, 7, 8, 7]
    zr = [1, 2, 3, 4, 5, 8, 7, 8, 9, 10, 11]
    if not np.array_equal(cg.zeros_left.flatten(), zl):
        print("After update, want zeros_left=", zl, "but got", cg.zeros_left.flatten())
        correct = False
    if not np.array_equal(cg.zeros_right.flatten(), zr):
        print("After update, want zeros_right=", zr, "but got", cg.zeros_right.flatten())
        correct = False
    pts_saved = np.array([12, 16, 9, 9, 14, 12, 0, 0, 13])
    pts_tot = np.array([18, 54, 99, 99, 63, 18, 0, 0, 9])
    ben_want = pts_saved - pts_tot 
    if not np.array_equal(cg.benefit.flatten(), ben_want):
        print("After update, want benefit=", ben_want, "but got", cg.benefit.flatten())
        correct = False

    if not correct:
        print("FAIL")
    else:
        print("PASS")



test_zeros()
test_benefit()
test_pop()

import numpy as np
import sys
import scipy.stats as stats
import argparse

def write_linear_map(prefix, data, mapped_col, target_col, num_outliers):
    x, y = data[:,mapped_col], data[:,target_col]
    xix = np.argsort(x)
    x = x[xix]
    y = y[xix]
    
    print("Fitting linear model")
    slope, intcpt, _, _, _ = stats.linregress(x, y)
    coeff = [slope, intcpt]
    err = y - (coeff[0]*x + coeff[1])
    # Deviation above which points are considered outliers
    frac = 1. - float(num_outliers) / len(x)
    outlier_offset = np.percentile(np.abs(err), 100*frac)
    outlier_offset = outlier_offset if coeff[0] > 0 else -outlier_offset;
    low_bnd = coeff[0]*x + coeff[1] - outlier_offset
    high_bnd = coeff[0]*x + coeff[1] + outlier_offset
    if coeff[0] < 0:
        low_bnd, high_bnd = high_bnd, low_bnd
    
    outliers = np.where(np.logical_or(y > high_bnd, y < low_bnd))[0]
    print("Found %d outliers with offset %f" % (len(outliers), outlier_offset))
    
    # Write data file (again, even though it's not sorted)
    # np.vstack((x, y)).T.tofile(prefix + ".data")
    
    # Write outliers file
    outliers.tofile(prefix + ".outliers")
    
    # Write mapping file
    f = open(prefix + ".model", 'w')
    # Source and target dimension
    f.write("linear\n%d\t%d\n" % (mapped_col, target_col))
    # Coefficients in order of degree, to high precision
    f.write(str(coeff[1]) + "\t" + str(coeff[0]) + "\n")
    # Offset to high precision
    f.write(str(outlier_offset))
    f.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser("GenLinearModel")
    parser.add_argument("--dataset",
            required=True,
            type=str,
            help="Path to the binary dataset")
    parser.add_argument("--dtype",
            required=True,
            type=str,
            help="Type of data (np compatible)")
    parser.add_argument("--dim",
            required=True,
            type=int,
            help="Dimension of the dataset")
    parser.add_argument("--mapped-col",
            default=0,
            type=int,
            help="Index of column being mapped from (default 0)")
    parser.add_argument("--target-col",
            default=1,
            type=int,
            help="Index of column being mapped to (default 1)")
    parser.add_argument("--num-outliers",
            required=True,
            type=int,
            help="Number of points to be considered outliers")
    parser.add_argument("--out-prefix",
            required=True,
            type=str,
            help="Prefix of files written for the indexer")
    args = parser.parse_args()
    
    #DATAFILE = "/home/ubuntu/data/stocks/eod_data/stocks_eod_daily.bin"
    #data = np.fromfile(DATAFILE, dtype=np.int32).reshape(-1, 7)
    DATAFILE = "/home/ubuntu/data/airlines/flight_data.bin"
    data = np.fromfile(args.dataset, dtype=args.dtype).reshape(-1, args.dim)
    write_linear_map(args.prefix, data, args.mapped_col, args.target_col, args.num_outliers)


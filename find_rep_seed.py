#!/usr/bin/env python3
"""Find representative seed across multiple scenarios (min avg RMSE from median)."""

import argparse
import gzip
import os
import sys
import numpy as np


def read_lsd_variable(res_file, variable_name):
    """Read a single variable from a .res.gz file. Returns array of values."""
    opener = gzip.open if res_file.endswith('.gz') else open
    with opener(res_file, 'rt') as f:
        header = f.readline().strip().split('\t')

        # Find column index for the variable
        col_idx = None
        for i, h in enumerate(header):
            # Header format: "VarName N_instance (start end)"
            var = h.split()[0] if h.strip() else ""
            if var == variable_name:
                col_idx = i
                break

        if col_idx is None:
            raise ValueError(f"Variable '{variable_name}' not found in {res_file}")

        values = []
        for line in f:
            fields = line.strip().split('\t')
            if col_idx < len(fields):
                val = fields[col_idx]
                try:
                    values.append(float(val))
                except ValueError:
                    values.append(np.nan)

    return np.array(values)


def find_representative_seed_global(paths, variable="GINI", warmup=100, workdir="."):
    """Find the single seed with minimum average RMSE across all scenarios."""

    all_scenario_data = {}
    seed_numbers = None  # Actual seed numbers from filenames

    for path in paths:
        full_path = os.path.join(workdir, path)
        res_files = sorted([
            os.path.join(full_path, f)
            for f in os.listdir(full_path)
            if f.endswith('.res.gz') or f.endswith('.res')
        ])

        if not res_files:
            print(f"WARNING: No .res files in {full_path}")
            continue

        print(f"  {path}: {len(res_files)} files")

        # Extract seed numbers from filenames (format: ConfigName_SEED.res.gz)
        if seed_numbers is None:
            seed_numbers = []
            for rf in res_files:
                base = os.path.basename(rf).replace('.res.gz', '').replace('.res', '')
                # Last number after underscore is the seed
                seed_num = int(base.split('_')[-1])
                seed_numbers.append(seed_num)

        # Read variable from each seed
        data = []
        for rf in res_files:
            vals = read_lsd_variable(rf, variable)
            if warmup < len(vals):
                vals = vals[warmup:]
            data.append(vals)

        # Ensure all same length (trim to shortest)
        min_len = min(len(d) for d in data)
        data = np.array([d[:min_len] for d in data])  # shape: (n_seeds, n_periods)

        all_scenario_data[path] = data

    if not all_scenario_data:
        print("ERROR: No data found!")
        sys.exit(1)

    n_seeds = None
    for path, data in all_scenario_data.items():
        if n_seeds is None:
            n_seeds = data.shape[0]
        elif data.shape[0] != n_seeds:
            print(f"ERROR: Mismatched seed counts: {n_seeds} vs {data.shape[0]}")
            sys.exit(1)

    print(f"\n  Seeds per scenario: {n_seeds}")
    print(f"  Seed numbers: {seed_numbers}")
    print(f"  Variable: {variable}, warmup: {warmup}")

    # Compute RMSE for each seed, averaged across scenarios
    avg_rmse = np.zeros(n_seeds)

    for path, data in all_scenario_data.items():
        median_traj = np.median(data, axis=0)
        for i in range(n_seeds):
            diff = data[i] - median_traj
            rmse = np.sqrt(np.nanmean(diff ** 2))
            avg_rmse[i] += rmse

    avg_rmse /= len(all_scenario_data)

    best_idx = np.argmin(avg_rmse)
    best_seed = seed_numbers[best_idx]
    worst_idx = np.argmax(avg_rmse)

    print(f"\n===== REPRESENTATIVE SEED =====")
    print(f"  Seed number: {best_seed} (file index {best_idx})")
    print(f"  Avg RMSE from median: {avg_rmse[best_idx]:.6f}")
    print(f"  Median RMSE across seeds: {np.median(avg_rmse):.6f}")
    print(f"  Worst seed: {seed_numbers[worst_idx]} (RMSE: {np.max(avg_rmse):.6f})")

    return best_seed


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--paths", nargs="+", required=True)
    parser.add_argument("--variable", default="Country_Gini_Index_Wealth")
    parser.add_argument("--warmup", type=int, default=100)
    parser.add_argument("--workdir", default=".")
    args = parser.parse_args()

    seed = find_representative_seed_global(args.paths, args.variable, args.warmup, args.workdir)

    # Write seed number to file for shell script to read
    with open(os.path.join(args.workdir, "rep_seed.txt"), "w") as f:
        f.write(str(seed))

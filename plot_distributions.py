#!/usr/bin/env python3
"""Plot income/wealth distributions from REP runs: histograms + Lorenz curves."""

import argparse
import gzip
import os
import numpy as np
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
import matplotlib.pyplot as plt


def read_lsd_variable(res_file, variable_name):
    """Read a single variable from a .res.gz file. Returns array of values per period."""
    opener = gzip.open if res_file.endswith('.gz') else open
    with opener(res_file, 'rt') as f:
        header = f.readline().strip().split('\t')

        # Find ALL column indices for the variable (one per household)
        col_indices = []
        for i, h in enumerate(header):
            var = h.split()[0] if h.strip() else ""
            if var == variable_name:
                col_indices.append(i)

        if not col_indices:
            raise ValueError(f"Variable '{variable_name}' not found in {res_file}")

        # Read all data
        all_data = []
        for line in f:
            fields = line.strip().split('\t')
            row = []
            for ci in col_indices:
                if ci < len(fields):
                    try:
                        row.append(float(fields[ci]))
                    except ValueError:
                        row.append(np.nan)
                else:
                    row.append(np.nan)
            all_data.append(row)

    return np.array(all_data)  # shape: (n_periods, n_households)


def read_country_variable(res_file, variable_name):
    """Read a single country-level variable (1 column). Returns 1D array."""
    opener = gzip.open if res_file.endswith('.gz') else open
    with opener(res_file, 'rt') as f:
        header = f.readline().strip().split('\t')

        col_idx = None
        for i, h in enumerate(header):
            var = h.split()[0] if h.strip() else ""
            if var == variable_name:
                col_idx = i
                break

        if col_idx is None:
            raise ValueError(f"Variable '{variable_name}' not found")

        values = []
        for line in f:
            fields = line.strip().split('\t')
            if col_idx < len(fields):
                try:
                    values.append(float(fields[col_idx]))
                except ValueError:
                    values.append(np.nan)

    return np.array(values)


def lorenz_curve(values):
    """Compute Lorenz curve from array of values. Returns (cum_pop, cum_share)."""
    values = values[~np.isnan(values)]
    values = np.sort(values)
    n = len(values)
    if n == 0 or np.sum(values) == 0:
        return np.array([0, 1]), np.array([0, 1])

    cum_share = np.cumsum(values) / np.sum(values)
    cum_pop = np.arange(1, n + 1) / n

    # Prepend origin
    cum_pop = np.insert(cum_pop, 0, 0)
    cum_share = np.insert(cum_share, 0, 0)

    return cum_pop, cum_share


def plot_all(workdir, seed):
    """Generate all distribution plots."""
    plotdir = os.path.join(workdir, "plots")
    os.makedirs(plotdir, exist_ok=True)

    scenarios = {"S0": "Baseline (no tax)", "S1": "Wealth tax"}
    colors = {"S0": "#2196F3", "S1": "#F44336"}
    time_points = [100, 250, 500]  # Strategic snapshots

    # Load REP data
    rep_data = {}
    for scen in scenarios:
        resdir = os.path.join(workdir, f"Results_REP_{scen}")
        res_files = sorted([
            os.path.join(resdir, f)
            for f in os.listdir(resdir)
            if f.endswith('.res.gz') or f.endswith('.res')
        ])
        if not res_files:
            print(f"  WARNING: No results for REP_{scen}")
            continue

        res_file = res_files[0]  # Only 1 seed
        print(f"  Reading {scen}: {os.path.basename(res_file)}")

        rep_data[scen] = {
            "income": read_lsd_variable(res_file, "Household_Nominal_Disposable_Income"),
            "wealth": read_lsd_variable(res_file, "Household_Net_Wealth"),
        }

    if len(rep_data) < 2:
        print("ERROR: Need both scenarios for comparison plots")
        return

    # =========================================================
    # PLOT 1: Income Lorenz curves at strategic time points
    # =========================================================
    fig, axes = plt.subplots(1, len(time_points), figsize=(5 * len(time_points), 5))
    fig.suptitle(f"Income Lorenz Curves (Rep Seed {seed})", fontsize=14)

    for ax, t in zip(axes, time_points):
        t_idx = t - 1  # 0-indexed
        ax.plot([0, 1], [0, 1], 'k--', alpha=0.3, label="Perfect equality")

        for scen, label in scenarios.items():
            if t_idx < rep_data[scen]["income"].shape[0]:
                vals = rep_data[scen]["income"][t_idx]
                cum_pop, cum_share = lorenz_curve(vals)
                ax.plot(cum_pop, cum_share, color=colors[scen], label=label, linewidth=2)

        ax.set_title(f"t = {t}")
        ax.set_xlabel("Cumulative population share")
        ax.set_ylabel("Cumulative income share")
        ax.legend(fontsize=8)
        ax.set_xlim(0, 1)
        ax.set_ylim(0, 1)
        ax.set_aspect('equal')

    plt.tight_layout()
    plt.savefig(os.path.join(plotdir, "lorenz_income.png"), dpi=150)
    plt.close()
    print("  Saved: plots/lorenz_income.png")

    # =========================================================
    # PLOT 2: Wealth Lorenz curves at strategic time points
    # =========================================================
    fig, axes = plt.subplots(1, len(time_points), figsize=(5 * len(time_points), 5))
    fig.suptitle(f"Wealth Lorenz Curves (Rep Seed {seed})", fontsize=14)

    for ax, t in zip(axes, time_points):
        t_idx = t - 1
        ax.plot([0, 1], [0, 1], 'k--', alpha=0.3, label="Perfect equality")

        for scen, label in scenarios.items():
            if t_idx < rep_data[scen]["wealth"].shape[0]:
                vals = rep_data[scen]["wealth"][t_idx]
                cum_pop, cum_share = lorenz_curve(vals)
                ax.plot(cum_pop, cum_share, color=colors[scen], label=label, linewidth=2)

        ax.set_title(f"t = {t}")
        ax.set_xlabel("Cumulative population share")
        ax.set_ylabel("Cumulative wealth share")
        ax.legend(fontsize=8)
        ax.set_xlim(0, 1)
        ax.set_ylim(0, 1)
        ax.set_aspect('equal')

    plt.tight_layout()
    plt.savefig(os.path.join(plotdir, "lorenz_wealth.png"), dpi=150)
    plt.close()
    print("  Saved: plots/lorenz_wealth.png")

    # =========================================================
    # PLOT 3: Wealth histograms — scenario comparison at t=250
    # =========================================================
    t_hist = 250
    t_idx = t_hist - 1

    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    fig.suptitle(f"Distribution Comparison at t={t_hist} (Rep Seed {seed})", fontsize=14)

    # Income histogram
    ax = axes[0]
    for scen, label in scenarios.items():
        if t_idx < rep_data[scen]["income"].shape[0]:
            vals = rep_data[scen]["income"][t_idx]
            vals = vals[~np.isnan(vals)]
            ax.hist(vals, bins=50, alpha=0.5, color=colors[scen], label=label, density=True)
    ax.set_xlabel("Nominal Disposable Income")
    ax.set_ylabel("Density")
    ax.set_title("Income Distribution")
    ax.legend()

    # Wealth histogram
    ax = axes[1]
    for scen, label in scenarios.items():
        if t_idx < rep_data[scen]["wealth"].shape[0]:
            vals = rep_data[scen]["wealth"][t_idx]
            vals = vals[~np.isnan(vals)]
            ax.hist(vals, bins=50, alpha=0.5, color=colors[scen], label=label, density=True)
    ax.set_xlabel("Net Wealth")
    ax.set_ylabel("Density")
    ax.set_title("Wealth Distribution")
    ax.legend()

    plt.tight_layout()
    plt.savefig(os.path.join(plotdir, "histograms_t250.png"), dpi=150)
    plt.close()
    print("  Saved: plots/histograms_t250.png")

    # =========================================================
    # PLOT 4: GINI trajectory comparison (from MC country-level data)
    # =========================================================
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))
    fig.suptitle(f"Inequality Trajectories (Rep Seed {seed})", fontsize=14)

    for ax, (var, title) in zip(axes, [("Country_Gini_Index", "Income Gini"), ("Country_Gini_Index_Wealth", "Wealth Gini")]):
        for scen, label in scenarios.items():
            resdir = os.path.join(workdir, f"Results_REP_{scen}")
            res_files = sorted([
                os.path.join(resdir, f)
                for f in os.listdir(resdir)
                if f.endswith('.res.gz') or f.endswith('.res')
            ])
            if res_files:
                try:
                    vals = read_country_variable(res_files[0], var)
                    ax.plot(range(1, len(vals) + 1), vals, color=colors[scen], label=label, linewidth=1.5)
                except ValueError:
                    print(f"  WARNING: {var} not found in REP_{scen}")

        ax.set_xlabel("Period")
        ax.set_ylabel(var)
        ax.set_title(title)
        ax.legend()

    plt.tight_layout()
    plt.savefig(os.path.join(plotdir, "gini_trajectories.png"), dpi=150)
    plt.close()
    print("  Saved: plots/gini_trajectories.png")

    print(f"\n  All plots saved in: {plotdir}/")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--seed", type=int, required=True)
    parser.add_argument("--workdir", default=".")
    args = parser.parse_args()

    plot_all(args.workdir, args.seed)

#!/usr/bin/env python3
"""Create REP .lsd configs by enabling save flag on 2 HH variables."""

import argparse
import os


def enable_hh_var_save(content, var_name):
    """Change save flag from 'n' to 's' for a household variable in the definitions section.

    Format: Var: Name num_lag save_flag + debug_flag plot_flag [lag_values...]
    We need to change the save_flag from 'n' to 's'.
    """
    # In the definitions section, lines look like:
    # Var: Household_Net_Wealth 1 n + n n\t0\t0...
    old = f"Var: {var_name} "
    lines = content.split('\n')
    found = False

    for i, line in enumerate(lines):
        if line.startswith(old):
            # Parse: "Var: Name num_lag save + debug plot [lag_vals]"
            parts = line.split('\t')
            prefix = parts[0]  # "Var: Name num_lag n + n n" (space-separated)

            # Split the prefix by spaces
            tokens = prefix.split(' ')
            # tokens: ['Var:', 'Name', 'num_lag', 'n', '+', 'n', 'n']
            # Index 3 is save_flag
            if len(tokens) >= 4 and tokens[3] == 'n':
                tokens[3] = 's'
                parts[0] = ' '.join(tokens)
                lines[i] = '\t'.join(parts)
                found = True
                print(f"    Enabled save for: {var_name}")
            elif len(tokens) >= 4 and tokens[3] == 's':
                print(f"    Already saved: {var_name}")
                found = True
            break

    if not found:
        print(f"    WARNING: Could not find/modify {var_name}")

    return '\n'.join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--workdir", default=".")
    args = parser.parse_args()

    hh_vars = [
        "Household_Nominal_Disposable_Income",
        "Household_Net_Wealth",
    ]

    for scen in ["S0", "S1"]:
        src = os.path.join(args.workdir, f"Test_{scen}.lsd")
        dst = os.path.join(args.workdir, f"REP_{scen}.lsd")

        with open(src, 'r') as f:
            content = f.read()

        print(f"  Creating REP_{scen}.lsd from Test_{scen}.lsd:")
        for var in hh_vars:
            content = enable_hh_var_save(content, var)

        with open(dst, 'w') as f:
            f.write(content)


if __name__ == "__main__":
    main()

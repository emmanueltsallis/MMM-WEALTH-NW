# MMM-WEALTH: Micro-Macro Multisectoral Model with Household Wealth Dynamics

A dynamic agent-based macroeconomic model combining Post-Keynesian, Kaleckian, and Neo-Schumpeterian approaches to investigate wealth inequality, taxation, evasion, and capital flight in capitalist economies. Built on the [LSD](https://github.com/marcov64/Lsd) (Laboratory for Simulation Development) framework.

This repository provides a **standalone headless (no-window) version** that compiles and runs without installing LSD.

---

## Prerequisites

You need a C++ compiler, `make`, and `zlib`:

| Platform | Install command |
|----------|----------------|
| **macOS** | `xcode-select --install` |
| **Ubuntu / Debian** | `sudo apt install build-essential zlib1g-dev` |
| **Fedora / RHEL** | `sudo dnf install gcc-c++ make zlib-devel` |
| **Windows** | Install [WSL2](https://learn.microsoft.com/en-us/windows/wsl/install), then follow the Ubuntu instructions |

---

## Quick Start

```bash
git clone https://github.com/emmanueltsallis/MMM-WEALTH-NW.git
cd MMM-WEALTH-NW
./build.sh
```

This auto-detects your platform, checks dependencies, and compiles the `lsdNW` executable.

---

## Running Simulations

### Choose a scenario

| Scenario | File | Description |
|----------|------|-------------|
| 0 | `Scenario_0.lsd` | **Baseline** — No wealth tax, no evasion, no enforcement |
| 1 | `Scenario_1.lsd` | **Action** — Wealth tax is introduced |
| 2 | `Scenario_2.lsd` | **Reaction** — Households evade the wealth tax |
| 3 | `Scenario_3.lsd` | **Counter-reaction** — Government enforces against evasion |

### Run a single scenario

```bash
# Run Scenario 0 with 50 Monte Carlo seeds, showing progress
./lsdNW -f Scenario_0.lsd -s 1 -e 50 -o Results_Scenario_0 -b
```

### Run all 4 scenarios in parallel

```bash
./run_all.sh          # 50 seeds each (default)
./run_all.sh 10       # 10 seeds each (faster)
```

### Quick test (minimal config, ~2 seconds)

```bash
./lsdNW -f Test_NW.lsd -s 1 -e 1 -b
```

---

## Command-Line Reference

```
./lsdNW -f FILENAME.lsd [OPTIONS]
```

| Flag | Argument | Description |
|------|----------|-------------|
| `-f` | `FILE.lsd` | Configuration file (required) |
| `-s` | `SEED` | Starting random seed (default: from .lsd file) |
| `-e` | `RUNS` | Number of Monte Carlo runs (default: from .lsd file) |
| `-o` | `PATH` | Output directory (created if needed) |
| `-b` | | Show progress bar |
| `-t` | | Output CSV instead of binary `.res` format |
| `-r` | | Skip per-seed result files (totals only) |
| `-z` | | Disable compression (produce `.res` instead of `.res.gz`) |

---

## Output Files

Simulations produce compressed result files:

| File | Description |
|------|-------------|
| `NAME_SEED.res.gz` | Per-seed results (e.g., `Scenario_0_1.res.gz`) |
| `NAME_FIRST_LAST.tot.gz` | Totals across seeds (e.g., `Scenario_0_1_50.tot.gz`) |

These can be read with the [LSDinterface](https://cran.r-project.org/package=LSDinterface) R package or exported as CSV using the `-t` flag.

---

## Rebuilding After Code Changes

After editing any `.h` equation file:

```bash
./build.sh
```

Only the model equations are recompiled; LSD framework objects are cached.

To start fresh:

```bash
./build.sh clean
./build.sh
```

---

## Citation

If you use this model in your research, please cite:

> Tsallis, E. (2026). *An Agent-Based Analysis of Wealth Distribution: Modeling a Wealth Tax subject to Evasion Dynamics and Capital Flight*. Master's Thesis, UFRJ.

---

## License

The LSD framework source code (in `src/`) is distributed under the GNU General Public License by Marco Valente (Universita dell'Aquila) and Marcelo C. Pereira (University of Campinas). See the [LSD project](https://github.com/marcov64/Lsd) for details.

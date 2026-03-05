# MMM-WEALTH: Micro-Macro Multisectoral Model with Household Wealth Dynamics

A mmicro-meso-macro agent-based model combining Post-Keynesian, Kaleckian, and Neo-Schumpeterian frameworks to investigate inequality, wealth taxation, evasion, and capital flight in modern capitalist economies. Built on the [LSD](https://github.com/marcov64/Lsd) (Laboratory for Simulation Development) software.

This repository provides a **standalone headless (no-window) version** that compiles and runs without installing LSD. For the full GUI version (with LSD's graphical browser, real-time plotting, and interactive configuration), see [MMM-WEALTH](https://github.com/emmanueltsallis/MMM-WEALTH).

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
./run.sh 0              # Scenario 0, 50 seeds, default population
./run.sh 0 10           # Scenario 0, 10 seeds
./run.sh 0 10 500       # Scenario 0, 10 seeds, 500 households
```

Or directly with `lsdNW`:

```bash
./lsdNW -f Scenario_0.lsd -s 1 -e 50 -o Results_Scenario_0 -b
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
| `-l` | `FILE` | Redirect log/diagnostic output to a file |
| `-c` | `T` | Use T threads for within-run parallelism |
| `-c` | `T:R` | Run R seeds in parallel, T threads each (advanced) |

### Run all 4 scenarios at once

```bash
./run_all.sh              # 50 seeds, 10000 households (defaults)
./run_all.sh 10           # 10 seeds, 10000 households
./run_all.sh 10 500       # 10 seeds, 500 households (faster)
```

This launches all 4 scenarios simultaneously (one process each, seeds run sequentially). The second argument overrides the household population without modifying the original scenario files. On machines with limited resources, run scenarios one at a time instead.

### Quick test (minimal config, ~2 seconds)

```bash
./lsdNW -f Test_NW.lsd -s 1 -e 1 -b
```

### Household population

The scenarios default to **10,000 households**. Larger populations produce richer distributional dynamics but take longer to run. The `Test_NW.lsd` config uses 50 households for quick tests.

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

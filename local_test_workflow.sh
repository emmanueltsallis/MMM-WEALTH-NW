#!/bin/bash
# =============================================================
# local_test_workflow.sh — Full 2-Tier Test Workflow
# =============================================================
# Tier 1: MC runs (10 seeds × 2 scenarios, 0 HH vars)
# Find representative seed (GINI_W, single-variable)
# Tier 2: REP runs (1 seed × 2 scenarios, 2 HH vars)
# Plot distributions (histograms + Lorenz curves)
# =============================================================

set -e
WORKDIR="$(cd "$(dirname "$0")" && pwd)"
cd "$WORKDIR"

LOGFILE="$WORKDIR/local_test.log"
SEEDS=10
SCENARIOS="S0 S1"

# Log function: writes to both stdout and log file
log() { echo "$@" | tee -a "$LOGFILE"; }

# Clear previous log
> "$LOGFILE"

log "========================================"
log "  LOCAL 2-TIER TEST WORKFLOW"
log "  $(date)"
log "  500 HH, 500 periods, $SEEDS seeds"
log "========================================"

# =============================================================
# TIER 1: MC Runs (no HH vars saved)
# =============================================================
log ""
log "=== TIER 1: Monte Carlo Runs ==="

TIER1_START=$(date +%s)

for scen in $SCENARIOS; do
    OUTDIR="$WORKDIR/Results_Test_${scen}"
    rm -rf "$OUTDIR"
    mkdir -p "$OUTDIR"

    log ""
    log "--- Scenario $scen: $SEEDS seeds ---"

    for seed in $(seq 1 $SEEDS); do
        SEED_START=$(date +%s)
        ./lsdNW -f "Test_${scen}.lsd" -s "$seed" -e 1 -o "$OUTDIR" -b > /dev/null 2>&1
        SEED_END=$(date +%s)
        log "  Seed $seed/$SEEDS done ($(( SEED_END - SEED_START ))s)"
    done

    NRES=$(ls "$OUTDIR"/*.res.gz 2>/dev/null | wc -l | tr -d ' ')
    log "  Completed: $NRES result files"
done

TIER1_END=$(date +%s)
log ""
log "=== TIER 1 COMPLETE ($(( TIER1_END - TIER1_START ))s total) ==="

# =============================================================
# FIND REPRESENTATIVE SEED
# =============================================================
log ""
log "=== FINDING REPRESENTATIVE SEED ==="

python3 "$WORKDIR/find_rep_seed.py" \
    --paths "Results_Test_S0" "Results_Test_S1" \
    --variable "Country_Gini_Index_Wealth" \
    --warmup 100 \
    --workdir "$WORKDIR" 2>&1 | tee -a "$LOGFILE"

REP_SEED=$(cat "$WORKDIR/rep_seed.txt")
log "Representative seed: $REP_SEED"

# =============================================================
# TIER 2: REP Runs (2 HH vars saved)
# =============================================================
log ""
log "=== TIER 2: Representative Seed Runs ==="

python3 "$WORKDIR/create_rep_config.py" --workdir "$WORKDIR" 2>&1 | tee -a "$LOGFILE"

TIER2_START=$(date +%s)

for scen in $SCENARIOS; do
    OUTDIR="$WORKDIR/Results_REP_${scen}"
    rm -rf "$OUTDIR"
    mkdir -p "$OUTDIR"

    SEED_START=$(date +%s)
    ./lsdNW -f "REP_${scen}.lsd" -s "$REP_SEED" -e 1 -o "$OUTDIR" -b > /dev/null 2>&1
    SEED_END=$(date +%s)
    log "  REP $scen (seed $REP_SEED) done ($(( SEED_END - SEED_START ))s)"
done

TIER2_END=$(date +%s)
log ""
log "=== TIER 2 COMPLETE ($(( TIER2_END - TIER2_START ))s total) ==="

# =============================================================
# PLOT DISTRIBUTIONS
# =============================================================
log ""
log "=== PLOTTING DISTRIBUTIONS ==="

python3 "$WORKDIR/plot_distributions.py" \
    --seed "$REP_SEED" \
    --workdir "$WORKDIR" 2>&1 | tee -a "$LOGFILE"

TOTAL_END=$(date +%s)
log ""
log "========================================"
log "  WORKFLOW COMPLETE"
log "  $(date)"
log "  Total time: $(( TOTAL_END - TIER1_START ))s"
log "  Plots saved in: $WORKDIR/plots/"
log "  Full log: $LOGFILE"
log "========================================"

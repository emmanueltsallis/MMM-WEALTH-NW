#!/bin/bash
# =============================================================
# run_all.sh — Run all 4 MMM-WEALTH scenarios
# =============================================================
#
# Usage:
#   ./run_all.sh                     50 seeds, sequential
#   ./run_all.sh 10                  10 seeds, sequential
#   ./run_all.sh 50 4:5              50 seeds, 5 parallel runs with 4 threads each
# =============================================================

set -e

SEEDS="${1:-50}"
PARALLEL="${2:-}"

if [ ! -f ./lsdNW ]; then
    echo "Error: lsdNW not found. Run ./build.sh first."
    exit 1
fi

echo "=============================================="
echo "  MMM-WEALTH — Running All 4 Scenarios"
echo "  Seeds per scenario: $SEEDS"
if [ -n "$PARALLEL" ]; then
    echo "  Parallel mode: -c $PARALLEL"
fi
echo "=============================================="
echo ""

PARALLEL_FLAG=""
if [ -n "$PARALLEL" ]; then
    PARALLEL_FLAG="-c $PARALLEL"
fi

for i in 0 1 2 3; do
    DIR="Results_Scenario_${i}"
    mkdir -p "$DIR"
    echo "Starting Scenario_${i} -> ${DIR}/ ($SEEDS seeds)"
    ./lsdNW -f "Scenario_${i}.lsd" -s 1 -e "$SEEDS" \
        -o "$DIR" \
        -l "${DIR}/run.log" \
        $PARALLEL_FLAG \
        -b &
done

echo ""
echo "All 4 scenarios launched in parallel."
echo "Monitor progress: tail -f Results_Scenario_*/run.log"
echo ""
wait

echo ""
echo "=============================================="
echo "  All scenarios complete!"
echo "=============================================="
for i in 0 1 2 3; do
    COUNT=$(ls Results_Scenario_${i}/*.res.gz 2>/dev/null | wc -l | xargs)
    echo "  Scenario_${i}: ${COUNT} result files"
done
echo ""

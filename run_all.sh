#!/bin/bash
# =============================================================
# run_all.sh — Run all 4 MMM-WEALTH scenarios
# =============================================================
#
# Launches all 4 scenarios simultaneously (one process each).
# Seeds within each scenario run sequentially by default.
#
# Usage:
#   ./run_all.sh              50 seeds, 10000 households (defaults)
#   ./run_all.sh 10           10 seeds, 10000 households
#   ./run_all.sh 10 500       10 seeds, 500 households
# =============================================================

set -e

SEEDS="${1:-50}"
POP="${2:-}"

if [ ! -f ./lsdNW ]; then
    echo "Error: lsdNW not found. Run ./build.sh first."
    exit 1
fi

# If population override requested, create temp copies with modified value
CLEANUP_FILES=""
if [ -n "$POP" ]; then
    for i in 0 1 2 3; do
        TMP="Scenario_${i}_tmp.lsd"
        sed "s/\(Param: country_total_population 0 n + n n\t\)[0-9]*/\1${POP}/" \
            "Scenario_${i}.lsd" > "$TMP"
        CLEANUP_FILES="$CLEANUP_FILES $TMP"
    done
    SUFFIX="_tmp"
    POP_MSG=" | Households: $POP"
else
    SUFFIX=""
    POP_MSG=""
fi

echo "=============================================="
echo "  MMM-WEALTH — Running All 4 Scenarios"
echo "  Seeds per scenario: $SEEDS${POP_MSG}"
echo "=============================================="
echo ""

for i in 0 1 2 3; do
    DIR="Results_Scenario_${i}"
    mkdir -p "$DIR"
    echo "Starting Scenario_${i} -> ${DIR}/ ($SEEDS seeds)"
    ./lsdNW -f "Scenario_${i}${SUFFIX}.lsd" -s 1 -e "$SEEDS" \
        -o "$DIR" \
        -l "${DIR}/run.log" \
        -b &
done

SECONDS=0

echo ""
echo "All 4 scenarios launched (one process each)."
echo ""

# Monitor progress until all processes finish
while pgrep -x lsdNW > /dev/null 2>&1; do
    ELAPSED=$SECONDS
    MINS=$((ELAPSED / 60))
    SECS=$((ELAPSED % 60))

    STATUS=""
    for i in 0 1 2 3; do
        LOG="Results_Scenario_${i}/run.log"
        if [ ! -f "$LOG" ]; then
            STATUS="${STATUS}  S${i}: waiting"
            continue
        fi
        if grep -q "Finished" "$LOG" 2>/dev/null; then
            COUNT=$(ls "Results_Scenario_${i}"/*.res.gz 2>/dev/null | wc -l | xargs)
            STATUS="${STATUS}  S${i}: done(${COUNT})"
        else
            SEED=$(grep "Simulation .* of .* running" "$LOG" 2>/dev/null | tail -1 | grep -o 'Simulation [0-9]*' | grep -o '[0-9]*')
            PCT=$(grep '^[.0-9%]*$' "$LOG" 2>/dev/null | grep -o '[0-9]*%' | tail -1)
            STATUS="${STATUS}  S${i}: ${SEED:-?}/${SEEDS} ${PCT:-0%}"
        fi
    done

    printf "\r  [%02dm %02ds]%s    " "$MINS" "$SECS" "$STATUS"
    sleep 1
done
wait

ELAPSED=$SECONDS
MINS=$((ELAPSED / 60))
SECS=$((ELAPSED % 60))
printf "\r  [%02dm %02ds]  S0: done  S1: done  S2: done  S3: done    \n" "$MINS" "$SECS"

# Clean up temp files if created
if [ -n "$CLEANUP_FILES" ]; then
    rm -f $CLEANUP_FILES
fi

echo ""
echo "=============================================="
echo "  All scenarios complete!"
echo "=============================================="
for i in 0 1 2 3; do
    COUNT=$(ls Results_Scenario_${i}/*.res.gz 2>/dev/null | wc -l | xargs)
    echo "  Scenario_${i}: ${COUNT} result files"
done
MINS=$((ELAPSED / 60))
SECS=$((ELAPSED % 60))
echo ""
echo "  Total time: ${MINS}m ${SECS}s"
echo ""

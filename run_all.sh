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

PIDS=""
for i in 0 1 2 3; do
    DIR="Results_Scenario_${i}"
    mkdir -p "$DIR"
    echo "Starting Scenario_${i} -> ${DIR}/ ($SEEDS seeds)"
    nohup ./lsdNW -f "Scenario_${i}${SUFFIX}.lsd" -s 1 -e "$SEEDS" \
        -o "$DIR" \
        -b > "${DIR}/run.log" 2>&1 &
    PIDS="$PIDS $!"
done
# Detach children so they survive if this script is killed
for pid in $PIDS; do
    disown "$pid" 2>/dev/null
done

echo ""
echo "All 4 scenarios launched (PIDs:$PIDS)"
echo "Processes are independent — safe to close this terminal."
echo ""
echo "Monitor with:  watch -n5 'for i in 0 1 2 3; do echo \"S\$i: \$(grep -oE \"PROG:[0-9]+%\" Results_Scenario_\$i/run.log 2>/dev/null | tail -1)\"; done'"
echo ""

# Optional: stay and monitor (Ctrl-C safe — won't kill simulations)
echo "Live monitor (Ctrl-C to detach):"
echo ""

trap '' INT  # Ignore Ctrl-C in monitor loop (won't kill children)
while true; do
    ALIVE=0
    STATUS=""
    for i in 0 1 2 3; do
        LOG="Results_Scenario_${i}/run.log"
        if grep -q "Finished" "$LOG" 2>/dev/null; then
            COUNT=$(ls "Results_Scenario_${i}"/*.res.gz 2>/dev/null | wc -l | xargs)
            STATUS="${STATUS}  S${i}: DONE(${COUNT})"
        elif [ -f "$LOG" ]; then
            ALIVE=$((ALIVE + 1))
            SEED=$(grep "Simulation .* of .* running" "$LOG" 2>/dev/null | tail -1 | grep -o 'Simulation [0-9]*' | grep -o '[0-9]*')
            PCT=$(grep -oE 'PROG:[0-9]+%' "$LOG" 2>/dev/null | grep -oE '[0-9]+%' | tail -1)
            STATUS="${STATUS}  S${i}: ${SEED:-?}/${SEEDS} ${PCT:-0%}"
        else
            STATUS="${STATUS}  S${i}: waiting"
        fi
    done

    ELAPSED=$SECONDS
    HRS=$((ELAPSED / 3600))
    MINS=$(((ELAPSED % 3600) / 60))
    SECS=$((ELAPSED % 60))
    printf "\r  [%02dh %02dm %02ds]%s    " "$HRS" "$MINS" "$SECS" "$STATUS"

    [ "$ALIVE" -eq 0 ] && break
    sleep 5
done
trap - INT

echo ""
echo ""
echo "=============================================="
echo "  All scenarios complete!"
echo "=============================================="
for i in 0 1 2 3; do
    COUNT=$(ls Results_Scenario_${i}/*.res.gz 2>/dev/null | wc -l | xargs)
    echo "  Scenario_${i}: ${COUNT} result files"
done
ELAPSED=$SECONDS
HRS=$((ELAPSED / 3600))
MINS=$(((ELAPSED % 3600) / 60))
echo ""
echo "  Total time: ${HRS}h ${MINS}m"
echo ""

# Clean up temp files if created
if [ -n "$CLEANUP_FILES" ]; then
    rm -f $CLEANUP_FILES
fi

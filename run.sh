#!/bin/bash
# =============================================================
# run.sh — Run a single MMM-WEALTH scenario
# =============================================================
#
# Usage:
#   ./run.sh 0              Scenario 0, 50 seeds, default population
#   ./run.sh 0 10           Scenario 0, 10 seeds
#   ./run.sh 0 10 500       Scenario 0, 10 seeds, 500 households
# =============================================================

set -e

if [ -z "$1" ]; then
    echo "Usage: ./run.sh SCENARIO [SEEDS] [POPULATION]"
    echo ""
    echo "  SCENARIO    0-3 (required)"
    echo "  SEEDS       Number of Monte Carlo seeds (default: 50)"
    echo "  POPULATION  Number of households (default: from .lsd file)"
    exit 1
fi

SCENARIO="$1"
SEEDS="${2:-50}"
POP="${3:-}"
CONFIG="Scenario_${SCENARIO}.lsd"

if [ ! -f ./lsdNW ]; then
    echo "Error: lsdNW not found. Run ./build.sh first."
    exit 1
fi

if [ ! -f "$CONFIG" ]; then
    echo "Error: $CONFIG not found."
    exit 1
fi

# If population override requested, create temp copy
if [ -n "$POP" ]; then
    TMP="Scenario_${SCENARIO}_tmp.lsd"
    sed "s/\(Param: country_total_population 0 n + n n\t\)[0-9]*/\1${POP}/" \
        "$CONFIG" > "$TMP"
    CONFIG="$TMP"
    POP_MSG=" | Households: $POP"
else
    TMP=""
    POP_MSG=""
fi

DIR="Results_Scenario_${SCENARIO}"
mkdir -p "$DIR"

echo "=============================================="
echo "  MMM-WEALTH — Scenario ${SCENARIO}"
echo "  Seeds: $SEEDS${POP_MSG}"
echo "=============================================="
echo ""

SECONDS=0
LOG="${DIR}/run.log"

./lsdNW -f "$CONFIG" -s 1 -e "$SEEDS" \
    -o "$DIR" \
    -l "$LOG" \
    -b &

# Monitor progress until finished
while pgrep -x lsdNW > /dev/null 2>&1; do
    ELAPSED=$SECONDS
    MINS=$((ELAPSED / 60))
    SECS=$((ELAPSED % 60))

    if [ -f "$LOG" ]; then
        SEED=$(grep "Simulation .* of .* running" "$LOG" 2>/dev/null | tail -1 | grep -o 'Simulation [0-9]*' | grep -o '[0-9]*')
        PCT=$(grep -oE 'PROG:[0-9]+%' "$LOG" 2>/dev/null | grep -oE '[0-9]+%' | tail -1)
        printf "\r  [%02dm %02ds]  Seed %s/%s  %s    " "$MINS" "$SECS" "${SEED:-?}" "$SEEDS" "${PCT:-0%}"
    fi
    sleep 1
done
wait

ELAPSED=$SECONDS
MINS=$((ELAPSED / 60))
SECS=$((ELAPSED % 60))
printf "\r  [%02dm %02ds]  Seed %s/%s  100%%    \n" "$MINS" "$SECS" "$SEEDS" "$SEEDS"

# Clean up temp file if created
if [ -n "$TMP" ]; then
    rm -f "$TMP"
fi

echo ""
COUNT=$(ls "${DIR}"/*.res.gz 2>/dev/null | wc -l | xargs)
MINS=$((ELAPSED / 60))
SECS=$((ELAPSED % 60))
echo ""
echo "Done. ${COUNT} result files in ${DIR}/ (${MINS}m ${SECS}s)"

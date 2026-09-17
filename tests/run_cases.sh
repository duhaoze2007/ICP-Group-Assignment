#!/bin/bash
# SDAMS scripted regression cases.
#
#   usage:  bash tests/run_cases.sh [path-to-binary]
#
# Every case runs in its own sandbox (/tmp/sdams-cases/<case>) so the
# committed data/*.txt seed files are never modified.  The output log of
# each case is kept for the "Sample Outputs" chapter of the report.
set -u

BIN=${1:-./sdams}
ROOT=$(cd "$(dirname "$0")/.." && pwd)
WORK=${SDAMS_WORK:-/tmp/sdams-cases}

if [ ! -x "$BIN" ]; then
    if [ -x "$ROOT/sdams" ]; then BIN="$ROOT/sdams"; else
        echo "Binary '$BIN' not found. Build first, e.g.:" >&2
        echo "  cc -std=c11 -Wall -Wextra -pedantic -Iinclude src/*.c -o sdams" >&2
        exit 1
    fi
fi

# Each case runs inside its own sandbox directory, so the binary path must
# be absolute before we cd away from the invocation directory.
BIN_DIR=$(cd "$(dirname "$BIN")" && pwd)
BIN="$BIN_DIR/$(basename "$BIN")"

mkdir -p "$WORK"

for case in manager student admin instructor facility negative; do
    dir="$WORK/$case"
    rm -rf "$dir"
    mkdir -p "$dir"
    cp -R "$ROOT/data" "$dir/data"

    # The negative case needs a class starting in two hours so that the
    # late-cancellation penalty rule can be triggered.
    if [ "$case" = "negative" ]; then
        echo "C008|I001|MMA|$(date -v+2H '+%Y-%m-%d %H:%M')|10|1|active" >> "$dir/data/classes.txt"
        echo "B004|S001|C008|$(date '+%Y-%m-%d %H:%M')|booked" >> "$dir/data/bookings.txt"
    fi

    ( cd "$dir" && "$BIN" < "$ROOT/tests/inputs/$case.txt" > "$dir/output.log" 2>&1 )
    echo "case $case: exit=$?  log=$dir/output.log"
done

echo
echo "Compare a log with the committed data afterwards, for example:"
echo "  head -40 $WORK/student/data/bookings.txt"

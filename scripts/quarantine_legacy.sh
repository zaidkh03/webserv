#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"

TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
QUARANTINE_ROOT="$ROOT_DIR/legacy_quarantine"
DEST="$QUARANTINE_ROOT/$TIMESTAMP"
MANIFEST="$DEST/MANIFEST.txt"

mkdir -p "$DEST"

PATHS=(
  "CRITICAL_SETUP.md"
  "FILE_INDEX.md"
  "FIXES.md"
  "IMPLEMENTATION_SUMMARY.md"
  "INSTALLATION.md"
  "QUICKSTART.md"
  "README_FIRST.md"
  "START_HERE.txt"
  "SUMMARY.md"
  "TESTING_REPORT.md"
  "SETUP.sh"
  "ULTIMATE_SETUP.sh"
  "setup.sh"
  "verify_setup.sh"
  "complete_test.py"
  "comprehensive_tester.py"
  "evaluation_tester.py"
  "integration_test.py"
  "pre_flight_test.py"
  "test_webserv.sh"
  "eval_suite.conf"
  "webserv.conf"
  "config/web.conf"
  "config/abeer.conf"
  "config/loay.conf"
  "test_root"
  "www"
  "cgi-bin"
  ".eval_quick_tmp"
  "__pycache__"
  "obj"
  "webserv"
  ".eval_smoke.log"
)

{
  echo "Quarantine Timestamp: $TIMESTAMP"
  echo "Root: $ROOT_DIR"
  echo ""
  echo "Moved paths:"
} > "$MANIFEST"

MOVED_COUNT=0
for rel in "${PATHS[@]}"; do
  src="$ROOT_DIR/$rel"
  if [ -e "$src" ]; then
    target="$DEST/$rel"
    mkdir -p "$(dirname "$target")"
    mv "$src" "$target"
    echo "- $rel" >> "$MANIFEST"
    MOVED_COUNT=$((MOVED_COUNT + 1))
  fi
done

if [ $MOVED_COUNT -eq 0 ]; then
  echo "No matching legacy paths were found. Nothing moved." >> "$MANIFEST"
fi

printf '%s\n' "$TIMESTAMP" > "$QUARANTINE_ROOT/LATEST"

echo "Quarantine complete."
echo "Destination: $DEST"
echo "Manifest: $MANIFEST"
echo "Moved entries: $MOVED_COUNT"

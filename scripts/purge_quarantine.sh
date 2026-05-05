#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
QUARANTINE_ROOT="$ROOT_DIR/legacy_quarantine"

if [ ! -d "$QUARANTINE_ROOT" ]; then
  echo "No legacy_quarantine directory found."
  exit 0
fi

TARGET=""
if [ -n "${1:-}" ]; then
  TARGET="$QUARANTINE_ROOT/$1"
elif [ -f "$QUARANTINE_ROOT/LATEST" ]; then
  TS="$(cat "$QUARANTINE_ROOT/LATEST")"
  TARGET="$QUARANTINE_ROOT/$TS"
fi

if [ -z "$TARGET" ] || [ ! -d "$TARGET" ]; then
  echo "No valid quarantine target found."
  echo "Usage: ./scripts/purge_quarantine.sh <timestamp>"
  echo "Or run after quarantine so LATEST exists."
  exit 1
fi

rm -rf "$TARGET"

if [ -f "$QUARANTINE_ROOT/LATEST" ] && [ "$(cat "$QUARANTINE_ROOT/LATEST")" = "$(basename "$TARGET")" ]; then
  rm -f "$QUARANTINE_ROOT/LATEST"
fi

if [ -z "$(find "$QUARANTINE_ROOT" -mindepth 1 -maxdepth 1 -type d -print -quit)" ]; then
  rm -rf "$QUARANTINE_ROOT"
fi

echo "Purged quarantine snapshot: $(basename "$TARGET")"

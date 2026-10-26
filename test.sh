#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")" && pwd)"
cd "$root"
python3 -B scripts/sync.py check
make test
python3 -B scripts/check-bytecode.py
python3 -B sample-suite/test_runner.py

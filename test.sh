#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")" && pwd)"
cd "$root"
python3 -B scripts/sync.py check
make test
python3 -B scripts/check-bytecode.py
python3 -B sample-suite/test_runner.py

# Compile the public example and compare its actual trace with the canonical case.
"${LMSCRIPT_BIN:-lmscript}" examples/rule-entries.lms build/rule-entries.dslb
build/lmcode-cpp-sample --dsl build/rule-entries.dslb > build/rule-entries.out
python3 - <<'PYTEST'
from pathlib import Path
actual = Path('build/rule-entries.out').read_bytes().replace(b'\r\n', b'\n')
expected = Path('sample-suite/expected/068-rule-entries.out').read_bytes()
assert actual == expected, 'annotated interface example output differs'
print('PASS annotated interface example')
PYTEST

# lmcode C++ shop sample

**Agent entry point:** this is a complete C++17 host, not another interpreter.
Read these three files first: [domain types](src/shop.hpp), [bindings](src/shop.cpp),
and [CLI](src/main.cpp). The SDK is header-only; the host needs only the C++
standard library, Make, Python 3 and `lmscript >= 0.2.420` (Node >=24.15).
No images or external application framework are needed.

```sh
npm install --global @lmctl-ai/lmctl@0.2.420
./test.sh
```

## Write a host in ten steps

1. **Choose a small domain.** `Shop` owns three priced items; `Ledger` records
   entries; `Money` is signed int64 cents. Read [the portable contract](sample-suite/CONTRACT.md).
2. **Declare the API.** [host.schema.json](sample-suite/host.schema.json) shows
   contexts, four object types, `Currency`, exact overloads and void methods.
   [sample.dsl.json](sample-suite/sample.dsl.json) merges that domain with std.
3. **Pin dependencies.** [sdk.lock](sdk.lock) and [suite.lock](suite.lock) hash
   every vendored byte; `scripts/sync.py check` rejects drift. No sibling repo
   or network is required to build this checked-out sample.
4. **Own objects before exposing handles.** [Host](src/shop.hpp) keeps `Money`
   allocations in an arena; borrowed pointers stay valid until the host dies.
   Standard collections are owned/cleared by the registry per run, not this arena.
5. **Register the schema's signatures.** [shop.cpp](src/shop.cpp) binds context,
   factory and method callbacks. `int` prices are cents; `float` prices are units.
   Enums cross the native boundary as strings. `returns:self` registers its owner type.
6. **Map language operations to methods.** Schema operators call `plus`, `minus`,
   `times`, `neg`; a declared conversion uses `money(int)`. `len/at` enable foreach.
   Ledger recording and Money.set return their receiver; note(string) returns void.
7. **Include standard/testing functions deliberately.** Default `Registry` supplies
   string/math/collections/file; optional testing supplies `print` and `assert_close`.
   The [coverage audit](sample-suite/audit.py) checks all 72 std signatures in actual imports.
8. **Compile, never interpret source in the host.** [program wrappers](sample-suite/programs)
   use `_lmcode_`/`savedsl`; `.dslb` is committed. Load with the **exact schema bytes**.
   [embed-schema.py](scripts/embed-schema.py) preserves them in a generated header.
9. **Handle errors at the right boundary.** [main.cpp](src/main.cpp) reports one
   JSON result after print traces: success 0, CLI/compile error 2, load/run error 3,
   application failure 4. A negative final balance is an application rule, not a VM error.
   The VM attaches code/line/column to host and arithmetic errors; only loader wording
   is normalized for portable teaching outputs. No timestamps or temp paths are printed.
10. **Run the shared integration test.** [run-suite.sh](sample-suite/run-suite.sh)
    accepts a binary or argv command, e.g. `./sample-suite/run-suite.sh dotnet host.dll`.
    Every case gets fresh objects and `SAMPLE_TMP`; stdout bytes and exit must match.
    The same suite is intended for C/C++/Rust/Go/C#/Java/Kotlin/Scala samples.

## Feature coverage

| Feature | Programs |
|---|---|
| Primitive literals/arithmetic/comparison/logic/if | 001–004 |
| While, range, inline fn, switches | 005–008, 036, 039, 043, 045 |
| Host overloads, objects, operators, conversion, chaining, void methods | 009–014, 034–035, 040 |
| Every std collection method | 015–018, 037–038 |
| Every std math overload + all 18 intrinsic math operations | 019–020, 025, 048 |
| Every std string function and conversion | 021–023 |
| Every std file function | 024 |
| Located VM/host errors, testing failure and application rule | 026–033 |
| int64 wrap/overflow, actual short-circuiting, host foreach | 041–044 |
| Raw/dedented inner heredocs and all four outer splice types | 046–047 |
| Bad magic/schema binding rejection | 090–091 |

The [full feature -> program matrix](sample-suite/README.md) links every source,
bytecode and oracle. The C++ reference generated expectations, then an independent
Go SDK host matched **50/50** byte-for-byte; approximate math uses assertions rather
than platform-specific decimal traces. Negative runner controls ensure wrong exits,
changed JSON whitespace, stale fixtures, empty manifests and missing std calls fail.

```text
PASS host CLI protocol (0/2/3/4; schema/signatures/discovery)
PASS: all 72 standard signatures are imported by the corpus
PASS 50/50 sample cases
PASS compiler reproduces all pinned bytecode
```

## Updating the reference

Canonical suite source lives in `lmctl-ai/lmcode-cpp/sample-suite`; edit it there,
not in this vendored copy. With reviewed commits in a sibling checkout:

```sh
./scripts/sync-sdk.sh ../lmcode-cpp
./scripts/sync-suite.sh ../lmcode-cpp
./test.sh
```

A new standard function must get a case/import; the audit rejects uncovered
signatures. Compiler/language changes must recompile/review the corpus and run
all sample ports, not silently rewrite expected files in CI. CI tests native Linux,
macOS and Windows builds; workflow_dispatch selects a newer published compiler.
The pinned-bytecode reproducibility check intentionally reports compiler drift
for review. Standard library and schema bytes are vendored with MIT licensing.

## Native installation check

The manual **Release and lmscript install** workflow first tests every native
build, publishes a pinned **prerelease** (never moves `latest`), then uses a fresh
cache and `lmscript install` on Linux x64/arm64, macOS x64/arm64 and Windows x64
and runs the same corpus against the installed bytes. Existing tags/assets are
never overwritten. For the initial version:

```sh
lmscript install github:lmctl-ai/lmcode-cpp-sample@v0.1.0
```

[installed-path.lms](scripts/installed-path.lms) shows the exact compiler discovery
contract (`cli` + pinned `install`); its final line reports the installed native
path. Release binaries include SHA256SUMS, MIT licensing and runtime notices.

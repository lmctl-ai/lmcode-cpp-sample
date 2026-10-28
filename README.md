# lmcode C++ shop sample

**Agent entry point:** this is a complete C++17 host, not another interpreter.
Read these three files first: [domain types](src/shop.hpp), [bindings](src/shop.cpp),
and [CLI](src/main.cpp). The SDK is header-only; the host needs only the C++
standard library, Make, Python 3 and an R47/R48-capable `lmscript` (Node >=24.15).
No images or external application framework are needed.

```sh
LMSCRIPT_BIN=/path/to/rule-method-capable/lmscript ./test.sh
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
   The [coverage audit](sample-suite/audit.py) checks all 181 std signatures in actual imports.
   File functions use the host filesystem; `SAMPLE_TMP` is a test convention, not a sandbox.
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
| Annotated rule method implementing a schema SAM interface | 067 |
| Host overloads, objects, operators, conversion, chaining, void methods | 009–014, 034–035, 040 |
| Pure host function `fee(int)` | 002 |
| Every std collection method | 015–018, 037–038, 101–104 |
| Every std math overload + all 18 intrinsic math operations | 019–020, 025, 048 |
| Every std string function and conversion | 021–023 |
| Every std file function | 024 |
| Located VM/host errors, testing failure and application rule | 026–033 |
| int64 wrap/overflow, actual short-circuiting, host foreach | 041–044 |
| Raw/dedented inner heredocs and all four outer splice types | 046–047 |
| Bad magic/schema/import binding rejection | 090–092 |
| JSON navigation/types and typed map/stack/queue/priority queues | 101–108 |

The [full feature -> program matrix](sample-suite/README.md) links every source,
bytecode and oracle. The C++ reference generated expectations, then an independent
Go SDK host matched **77/77** byte-for-byte; approximate math uses assertions rather
than platform-specific decimal traces. Negative runner controls ensure wrong exits,
changed JSON whitespace, stale fixtures, empty manifests and missing std calls fail.

```text
PASS: all 181 standard signatures are imported by the corpus
PASS: all 45 host signatures are imported by the corpus
PASS host CLI protocol (0/2/3/4; schema/signatures/discovery)
PASS 91/91 sample cases
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
lmscript install github:lmctl-ai/lmcode-cpp-sample@v0.3.0
```

[installed-path.lms](scripts/installed-path.lms) shows the exact compiler discovery
contract (`cli` + pinned `install`); its final line reports the installed native
path. Release binaries include SHA256SUMS, MIT licensing and runtime notices.

## Conversions

| lmcode value | C++ representation | Boundary rule |
|---|---|---|
| int | `std::int64_t` | Never `int`/Windows `long`; wrap through uint64 arithmetic. |
| float | `double` | Preserve binary64, signed zero, NaN/inf and subnormals; `str` is canonical G15. |
| bool | `bool` | Boolean is a distinct tag, not an integer convention. |
| string | `std::string` | Raw bytes + size; NUL/invalid UTF-8 are valid. Never use strlen/UTF-8 replacement. |
| enum | string-backed Value | Register the enum's schema type; preserve exact member spelling. |
| object | type tag + `void*` | Stable owned arena, preserve identity; typed null differs from void. |
| void | `Value::none()` | Produces no stack result. |
| std list/map | SDK arena pointer | Copy data through callbacks; allocate returns in registry arena; expire after run. |

Programs 049–063 cover conversion edges: invalid file bytes, one-MiB strings,
all four collection types in both directions, nullable/aliased/wrong handles,
nested located callback errors, 255 arguments, a 509-slot operand stack, long
imports and a million-iteration loop. Recursion is a checked compiler rejection;
acyclic inline nesting is exercised instead. See the exact shared contract.

Build all eight ports and compare their **actual** outputs to one another as well
as the oracle with the canonical SDK's `sample-suite/run-all-samples.sh --all`.
Unimplemented ports fail visibly; they are never silently skipped.

The pinned corpus includes lmcode literals and R47/R48 rule methods. Use a
matching `LMSCRIPT_BIN` to reproduce all 91 cases. The old v1 literal-only skip
cannot make a compiler accept the interface-bearing schema. SDK and suite locks
remain exact.

The v3 pin adds four strict byte codecs and six independent map snapshot methods. Programs 301–305 and 311–313 exercise them in the 91-case C++ corpus; other SDK v3 ports remain pending.

## Rule methods (R47/R48)

The suite is pinned to `lmcode-cpp` commit `aec9b09`. Program 067 calls an
`@PRICE_RULE` function implementing `interfaces.Adjuster` from the chunk and
checks the adjusted prices. The shared compile-error corpus rejects a mismatched
rule signature. These are compiler-only features: this host still executes DSLB
v5; the exact schema hash changes, so the suite includes recompiled bytecode.
Run `LMSCRIPT_BIN=/path/to/rule-method-capable/lmscript ./test.sh` to reproduce
the sources. The first published capable compiler is `@lmctl-ai/lmctl@0.2.442`
(currently on the `staging` dist-tag). The manual test workflow requires a
matching published compiler version input; older releases cannot reproduce the
interface-bearing corpus.

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
   The [coverage audit](sample-suite/audit.py) checks all 182 std signatures in actual imports.
   File functions use the host filesystem; `SAMPLE_TMP` is a test convention, not a sandbox.
8. **Compile, never interpret source in the host.** [program wrappers](sample-suite/programs)
   use `_lmcode_ ... out=ARGV[0]`; `.dslb` is committed. Load with the **exact schema bytes**.
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
| Rule entries called by the host after the body (`@item`/`@total`, `entries_called`) | 068–069 |
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
PASS: all 182 standard signatures are imported by the corpus
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

## Rule methods (R47/R48) and rule entries (R49, DSLB v6)

Program 067 calls an `@PRICE_RULE` function implementing `interfaces.Adjuster`
from the chunk and checks the adjusted prices. With DSLB v6 every annotated rule
also compiles to a rule entry: after the body runs, [main.cpp](src/main.cpp)
calls each `@item` entry once per shop item in order and each `@total` entry
once with the shop total (068), prints a non-void entry result as
`entry NAME = VALUE` (069, `interfaces.Score`), never calls other annotations,
and reports the number of calls as `"entries_called"` in the result object.
Annotation without `implements` is a compiler rejection; a v5 file is a load
error (093). The first published rule-methods-capable compiler is
`@lmctl-ai/lmctl@0.2.442` (currently on the `staging` dist-tag); the v6 corpus
needs the v6-entries compiler
(`LMSCRIPT_BIN=/path/to/v6-entries/lmscript ./test.sh`). Older releases cannot
reproduce the interface-bearing corpus.

## Runnable annotated interface example

[examples/rule-entries.lms](examples/rule-entries.lms) defines two `@item`
functions and one `@total` function, each with `implements Adjuster`. The
schema declares `Adjuster(Money) -> void`; the compiler checks the signature.
The body prints first, then the host calls entries by annotation. `@never`
demonstrates an entry that this host does not dispatch.

```sh
make
lmscript examples/rule-entries.lms build/rule-entries.dslb
build/lmcode-cpp-sample --dsl build/rule-entries.dslb
```

Use a private mono main compiler build at 0bc80277 or newer (DSLB v6), selected with `LMSCRIPT_BIN`. The expected trace is pinned in
[sample-suite/expected/068-rule-entries.out](sample-suite/expected/068-rule-entries.out).
For a non-void interface and returned entry value, see
[069-rule-value-entry.lms](sample-suite/programs/069-rule-value-entry.lms).

## Automatic releases

Every push to `main` builds and tests Linux x64/arm64, macOS x64/arm64,
and Windows x64. All five jobs must pass the full suite, bytecode
reproduction and the public annotated interface example before publication.
Pull requests run the same checks without publishing.

The release workflow installs lmctl and lmscript from npm and publishes an immutable
`v0.4.<workflow run number>` release, marking it latest. Executables report
that same version; assets use the lmscript platform naming convention and
include `SHA256SUMS`, license and notices. Failed tests prevent publication.
A workflow rerun keeps an already published release unchanged.

```sh
lmscript install github:lmctl-ai/lmcode-cpp-sample@latest
```

## Tests driven by lmscript

```sh
npm install -g @lmctl-ai/lmctl
lmctl test.lms
# Equivalent executable:
lmscript test.lms
```

Both executables run `.lms` files; `lmctl` also accepts a URL ending in `.lms`.
These repository tests require the checkout and run from its root because they
use the pinned corpus and native sources. `./test.sh` is a convenience wrapper.
CI uses `lmscript --no-preflight test.lms` to execute the tests once.

The suite, protocol checks, signature coverage, compiler reproduction and
negative controls are orchestrated by `.lms` files in `sample-suite/`. A small
Node adapter preserves binary hashes and byte-exact subprocess output; it
contains no test assertions. Python remains for SDK lock synchronization and
build-time schema embedding, not test orchestration.

## Text templates

[examples/text-template.lms](examples/text-template.lms) renders ordinary text from a nested context with indexed lists, loops and boolean conditions. The standard `template_render(string, Json) -> string` function performs one pass; inserted values stay literal. Compile it with `lmscript --no-preflight examples/text-template.lms build/text-template.dslb`, then run the sample host with `--dsl build/text-template.dslb`.

The lmscript-driven suite runs 108 cases, including template success/error cases. See the [portable template contract](https://github.com/lmctl-ai/lmcode-cpp/blob/main/std/template.md).

### Invocation return frame

The shop diagnostic and int entry traces retain their previous order. Successful
`--dsl` execution then appends exactly one SDK `LMCODE_RETURN_BEGIN` / compact JSON /
`LMCODE_RETURN_END` frame with LF bytes. An unset `$return` emits null. Body and
entry writes share one owned sink through all item/total calls; failed VM or
application validation emits no frame. See [ownership and local controls](durable-memory/invocation-return.md).

# Invocation result adoption (consensus slice 6)

One host-owned C++ ResultSink is bound before the body and retained across all
@item/@total calls. Never reset it in call_entry or move it into standard arenas.
The host still prints int entry HALT results and the shop/ledger diagnostic;
only after successful application validation does it append the SDK terminal
return frame. Unset success is null. Errors never frame. Binary stdout preserves
LF on Windows. The canonical suite supplies independent result oracles and the
exact trace/diagnostic/frame bytes.

Use a private mono main build at 0bc80277 or newer via LMSCRIPT_BIN. Compiler
wrappers now use `_lmcode_ ... out=ARGV[0]`; savedsl is retired. Do not record
expectations in this consumer: canonical C++ owns sample-suite/record.py output.
Sync the committed canonical suite and SDK with scripts/sync-suite.sh and
scripts/sync-sdk.sh, then run ./test.sh. Optional local actual-host mutation
proof: `python3 scripts/test-return-controls.py` after make. Missing-frame and
reset-before-entry mutations compile successfully and must fail the independent
return oracle; signals, stderr and compile failures are driver errors.

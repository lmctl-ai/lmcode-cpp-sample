#!/usr/bin/env python3
"""Mutation controls for the canonical C++ sample return-frame host.

The driver compiles each mutation in a temporary directory.  It never edits
the checked-out host or the vendored SDK, and it treats compiler/link failures
as driver failures rather than as caught mutants.
"""
import json
import os
from pathlib import Path
import runpy
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HOST_SOURCE = ROOT / "src/main.cpp"
HOST_BINARY = ROOT / "build/lmcode-cpp-sample"
ORACLE = runpy.run_path(str(ROOT / "sample-suite/return_protocol.py"))["validate"]


def load_cases():
    manifest = json.loads((ROOT / "sample-suite/manifest.json").read_text(encoding="utf-8"))
    wanted = {"001-literals", "360-return-scalar", "364-return-entry-lifetime",
              "365-return-entry-error", "366-return-app-error"}
    return [case for case in manifest["cases"] if case["name"] in wanted]


def run_case(binary, case):
    result = subprocess.run(
        [str(binary), "--dsl", str(ROOT / "sample-suite" / case["bytecode"])],
        capture_output=True,
    )
    if result.returncode != case["expected_exit"]:
        raise RuntimeError(f"{case['name']}: wrong exit/signal {result.returncode}")
    ORACLE(result.stdout, result.returncode, case.get("expected_return"))
    if result.stderr:
        raise RuntimeError(f"{case['name']}: unexpected stderr {result.stderr!r}")
    return result


def build(source: Path, output: Path) -> None:
    cxx = os.environ.get("CXX", "c++")
    flags = ["-std=c++17", "-O0", "-Wall", "-Wextra", "-Werror", "-pedantic"]
    includes = [
        f"-I{ROOT / 'src'}",
        f"-I{ROOT / 'build'}",
        f"-I{ROOT / 'vendor/lmcode/include'}",
        f"-I{ROOT / 'vendor/lmcode/stdlib'}",
        '-DSAMPLE_VERSION="0.4.0"',
    ]
    subprocess.run(
        [cxx, *flags, *includes, str(source), str(ROOT / "src/shop.cpp"),
         str(ROOT / "src/conversions.cpp"), "-o", str(output)],
        check=True,
    )


def assert_baseline(binary: Path) -> None:
    if not binary.is_file():
        raise SystemExit(f"missing baseline host: {binary}; build the sample first")
    for case in load_cases():
        run_case(binary, case)


def mutation(name: str, old: str, new: str, expected_case: str) -> None:
    original = HOST_SOURCE.read_text(encoding="utf-8")
    if original.count(old) != 1:
        raise AssertionError(f"{name}: expected one source anchor, found {original.count(old)}")
    with tempfile.TemporaryDirectory(prefix=f"sample-return-{name}-", dir=ROOT / "build") as work:
        workdir = Path(work)
        source = workdir / "main.cpp"
        binary = workdir / "sample"
        source.write_text(original.replace(old, new), encoding="utf-8", newline="\n")
        build(source, binary)
        try:
            run_case(binary, next(case for case in load_cases() if case["name"] == expected_case))
        except (AssertionError, ValueError) as error:
            print(f"{name}: CAUGHT ({error})")
            return
        raise AssertionError(f"{name}: mutant survived")


def main() -> None:
    assert_baseline(HOST_BINARY)
    mutation(
        "missing-frame",
        "dslvm::write_result_frame(std::cout, invocation_result);",
        "(void)invocation_result;",
        "360-return-scalar",
    )
    mutation(
        "reset-before-entry",
        "dslvm::Value result = program.call(index, context, host.registry, {std::move(argument)});",
        "invocation_result.reset();\n            dslvm::Value result = program.call(index, context, host.registry, {std::move(argument)});",
        "364-return-entry-lifetime",
    )
    print("PASS: baseline oracle and 2 return-frame mutants")


if __name__ == "__main__":
    main()

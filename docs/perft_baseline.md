
# Knightmare Engine — Perft Baseline

## Purpose

Record reference perft node counts and execution performance so future engine changes and search optimizations can be checked for correctness and speed regressions.

## Environment

- Compiler: g++
- Language standard: C++17
- Build configuration: Release (`-O2 -DNDEBUG`)
- Hardware: Not recorded
- Date: 2026-10-18

## Reference Node Counts

Expected node counts are taken from `tests/test_data.h`.

| Position | Depth | Expected Nodes |
|---|---:|---:|
| Starting Position | 1 | 20 |
| Starting Position | 2 | 400 |
| Starting Position | 3 | 8,902 |
| Starting Position | 4 | 197,281 |
| Starting Position | 5 | 4,865,609 |
| Kiwipete | 1 | 48 |
| Kiwipete | 2 | 2,039 |
| Kiwipete | 3 | 97,862 |
| Kiwipete | 4 | 4,085,603 |
| Position 3 | 1 | 14 |
| Position 3 | 2 | 191 |
| Position 3 | 3 | 2,812 |
| Position 3 | 4 | 43,238 |
| Position 4 | 1 | 6 |
| Position 4 | 2 | 264 |
| Position 4 | 3 | 9,467 |
| Position 4 | 4 | 422,333 |
| Position 5 | 1 | 44 |
| Position 5 | 2 | 1,486 |
| Position 5 | 3 | 62,379 |
| Position 5 | 4 | 2,103,487 |
| Position 6 | 1 | 46 |
| Position 6 | 2 | 2,079 |
| Position 6 | 3 | 89,890 |
| Position 6 | 4 | 3,894,594 |

## Performance Baseline

Measured using the Release build and bulk perft.

Latest recorded run:

| Position | Depth | Expected Nodes | Actual Nodes | Elapsed Time (s) | Nodes/Second |
|---|---:|---:|---:|---:|---:|
| Starting Position | 5 | 4,865,609 | 4,865,609 | 5.801 | 838,802 |
| Kiwipete | 4 | 4,085,603 | 4,085,603 | 4.308 | 948,326 |
| Position 3 | 4 | 43,238 | 43,238 | 0.056 | 770,326 |
| Position 4 | 4 | 422,333 | 422,333 | 0.452 | 935,383 |
| Position 5 | 4 | 2,103,487 | 2,103,487 | 2.261 | 930,528 |
| Position 6 | 4 | 3,894,594 | 3,894,594 | 4.015 | 969,975 |

All six benchmark positions produced the expected node counts in this run.

These measurements are reference values, not guaranteed performance figures. Runtime varies with hardware, compiler version, build configuration, and system load.

## Regression Procedure

1. Run the fast suite after every engine change.
2. Run the slow suite before important milestones and AI-search optimizations.
3. Investigate every perft node-count mismatch before accepting an optimization.
4. Compare performance using the same compiler configuration and similar system conditions.
5. When a bug is fixed, preserve a regression test for the failing position or move sequence, preferably by adding it to `testRegressions()` in `tests/test_rules.cpp`.
6. Never delete a failing test simply to make the suite pass; fix the underlying issue.

## Test Commands

Build the Release test runner:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_release.ps1
```

Run the fast suite:

```powershell
.\tests\test_runner_release.exe --fast
```

Run the slow suite:

```powershell
.\tests\test_runner_release.exe --slow
```

## Latest Regression Results

Date: 2026-10-18

| Suite | Passed | Failed | Total |
|---|---:|---:|---:|
| Fast | 838 | 0 | 838 |
| Slow | 1,159 | 0 | 1,159 |

The fast suite uses 8 random playouts. The slow suite uses 50 random playouts and includes deeper perft, mirror tests, and benchmark checks.

## Notes

- Expected node counts are correctness references and must not change merely because an optimization was introduced.
- Perft validates move-generation correctness; it does not establish playing strength.
- Search optimizations may change move ordering and execution speed, but must preserve legal move generation and expected perft counts.
- Update this document whenever a new validated benchmark position is added or a new baseline is measured.

## Phase 8 repeated performance measurements

The same Release perft positions and depths were measured twice after adding
incremental Zobrist hashing and cached king squares. All expected node counts
matched. The two current runs show timing variation and mixed throughput; these
are measurements, not a claim that every position became faster.

| Position | Depth | Original ms / NPS | Current run 1 ms / NPS | Current run 2 ms / NPS |
|---|---:|---:|---:|---:|
| Starting Position | 5 | 5,801 / 838,802 | 5,583 / 871,466 | 5,506 / 883,587 |
| Kiwipete | 4 | 4,308 / 948,326 | 4,554 / 897,121 | 4,450 / 918,025 |
| Position 3 | 4 | 56 / 770,326 | 56 / 770,571 | 55 / 783,345 |
| Position 4 | 4 | 452 / 935,383 | 483 / 873,627 | 466 / 905,741 |
| Position 5 | 4 | 2,261 / 930,528 | 2,361 / 890,688 | 2,312 / 909,578 |
| Position 6 | 4 | 4,015 / 969,975 | 4,235 / 919,434 | 4,174 / 932,944 |

Command: `powershell -ExecutionPolicy Bypass -File .\tests\run_perft_benchmark.ps1`.
Runtime depends on system load; repeat on the target machine before using small
percentage differences to make hardware-specific decisions.

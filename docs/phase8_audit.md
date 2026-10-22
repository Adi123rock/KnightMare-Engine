# Phase 8 Implementation and Verification

Updated 2026-10-21. No Phase 8-specific specification or implementation notes
were present in `docs/`; this audit uses the complete supplied Phase 8 task as
the working specification. The workspace has no `.git` directory, so Git status
or a pre-change diff could not be inspected. Existing files were inspected and
no destructive Git or file operations were performed.

## Initial baseline and scope

- Existing Phase 7 search build succeeded and its fast suite passed 1,716 / 1,716
  before Phase 8 edits.
- `docs/phase7_audit.md` already contained fixed-depth search results for start
  position and Kiwipete. Those results are used for the same-position historical
  comparison below.
- `docs/perft_baseline.md` contains the pre-Phase 8 perft results from the
  established Release workflow.
- Initial code had FEN-string repetition history, no Zobrist hash or TT, repeated
  king scans, no UCI executable/loop, and no production benchmark executable.
- `build_release.ps1` initially failed to link because it omitted search and
  minimax sources included by `tests/run_all.cpp`; that build integration was
  corrected and verified.

## Requirements and evidence

| Requirement | Status | Implementation | Verification |
|---|---|---|---|
| Reproducible benchmark baseline | COMPLETE | `tests/bench_search.cpp`, `tests/bench_perft.cpp`, and their PowerShell runners use fixed positions/depths and report nodes, time, NPS, depth, score, move, and search cutoffs/TT counts. | Both benchmark commands ran; all perft counts and all five paired search scores matched. Historical same-position results are recorded below. |
| Engine hot-path optimization | COMPLETE | `Board` caches both king squares; `findKing()` reads the cache. Board history vectors reserve capacity. `makeMove()` updates the position hash incrementally. | Randomized 80-ply legal sequence, special moves, undo and full perft suites pass. Search and perft timings are recorded below, including regressions. |
| Zobrist hashing | COMPLETE | Deterministic SplitMix64 keys cover piece-square, side, castling and en-passant state. `Undo` restores the prior hash; `Board` retains hash history alongside canonical position keys. | Recompute-vs-incremental checks on random legal play, captures, castling, en passant, promotion, and undo; deterministic/load-state identity checks pass. Repetition confirms a hash match against the canonical position key. |
| Transposition table | COMPLETE | `TranspositionTable` stores the full 64-bit key, depth, normalized score, bound, best move, generation, halfmove clock and quiescence mode. Default capacity is 16 MiB; UCI `Hash` and `Clear Hash` options configure it. | Exact/bound hits, collision verification, depth replacement, stale-generation replacement, clear, mate normalization, repeated searches, score equivalence and legal TT/PV results pass. Benchmarks expose TT hits/cutoffs. |
| Move ordering | COMPLETE | Ordering prioritizes PV/TT moves, favorable captures and promotions, killers, history/quiet moves, then likely losing captures. It operates on generated legal moves. | Five-position controlled search benchmarks preserve exact scores, moves, board/history, and legality; cutoff effectiveness is reported. Existing ordering regressions pass. |
| Optional search techniques | COMPLETE WITH DOCUMENTED SELECTION | PVS and aspiration windows are implemented behind `SearchLimits` flags. Both default off because this engine's controlled measurements were mixed or slower. Check extensions, null-move pruning and LMR remain unimplemented: the available task calls these optional and no Phase 8 source specification was available to designate them as required. | PVS-only and aspiration-only were measured against the same five-position baseline. All returned identical scores. Aspiration-only raised aggregate nodes substantially; PVS was position-dependent. |
| Time management | COMPLETE | `SearchLimits` supports fixed move time, side clocks, increments, moves-to-go, depth, nodes, and infinite mode. Search retains the last fully completed iteration/legal fallback; stop is atomic. | Unit tests cover budget math, zero clock, node limits, tiny time, async cancellation and state restoration. UCI script exercises `movetime`, clock fields, depth, nodes and `stop`. Stop/time polling is every 2,048 search nodes, so strict deadlines are not guaranteed. |
| UCI | COMPLETE FOR REQUIRED COMMANDS | New `src/uci_main.cpp` handles `uci`, `isready`, `ucinewgame`, startpos/FEN with move lists, `go`, `stop`, `quit`, `setoption`, `debug`, `ponderhit`, and `go infinite`. Search runs on a worker over a copied board; output is synchronized. | `tests/test_uci.ps1` communicates with the actual executable and checks handshake, readiness during search, info fields, bestmove, position updates, time/node/depth limits, stop and clean exit. |
| Correctness/build integration | COMPLETE | Search and minimax are included in Release test build; new engine and benchmark build scripts are provided. | `build_search_tests.ps1`, `build_release.ps1`, and `build_engine.ps1` succeeded. Final fast and slow suites passed. |
| Documentation | COMPLETE | This audit records feature decisions, verification and performance; `perft_baseline.md` adds current repeated measurements. | Commands and exact outputs are listed below. |

### Optional UCI behavior

`debug` is accepted as a no-op because no debug protocol output is enabled.
`ponderhit` is accepted but pondering is not implemented. `go searchmoves` is
not implemented; if supplied, its list is ignored. These commands are outside
the required command set in the available task. `go infinite` and `stop` are
implemented and tested.

## Search benchmark

Command: `powershell -ExecutionPolicy Bypass -File .\tests\run_benchmarks.ps1`

All cases use the same FEN, completed depth, quiescence-off setting, and default
move ordering. `BASELINE` disables TT, PVS, and aspiration; `TT_ONLY` enables the
16 MiB TT; PVS and aspiration are separately measured and remain opt-in. Each
entry is `nodes / elapsed ms / NPS`. Score and best move were identical between
baseline and every configuration for every position. All moves were legal and
boards/histories were restored.

| Position (depth) | Baseline | TT only | TT hits / cutoffs | Beta / first-move cutoffs |
|---|---:|---:|---:|---:|
| Opening start (5) | 23,996 / 362 / 66,287 | 21,791 / 333 / 65,438 | 1,094 / 336 | 3,210 / 2,787 |
| Middlegame Kiwipete (4) | 8,668 / 206 / 42,077 | 8,696 / 210 / 41,409 | 474 / 163 | 2,280 / 2,241 |
| Tactical hanging queen (5) | 29,907 / 488 / 61,284 | 24,735 / 410 / 60,329 | 1,051 / 284 | 3,180 / 2,801 |
| Rook endgame ladder (4) | 2,021 / 14 / 144,357 | 1,872 / 14 / 133,714 | 207 / 141 | 539 / 539 |
| High-branching Position 6 (4) | 10,115 / 224 / 45,156 | 11,035 / 250 / 44,140 | 604 / 292 | 2,603 / 2,571 |

Across these positions the TT-only run searched 68,129 vs 74,707 nodes (8.8%
fewer) and used 1,217 vs 1,294 ms (6.0% less). The Kiwipete and Position 6
individual runs regressed slightly; timings are machine/load sensitive. The TT
is retained because it is a required feature and produced a measurable aggregate
benefit, with the per-position trade-offs shown instead of hidden.

PVS-only totalled 71,112 nodes and 1,285 ms; it improved some positions and
regressed others. Aspiration-only totalled 141,070 nodes and 3,033 ms because
successive iterations often failed the narrow window and needed wider re-search.
PVS and aspiration remain available but disabled by default. The combined
TT/PVS/aspiration configuration preserved scores but used 76,155 nodes and
1,537 ms; it is not the default.

Historical Phase 7 `complete` ordering baseline (same FENs, depths, no
quiescence) was 24,967 nodes / 398 ms / 62,731 NPS for start and 9,189 / 209 ms
/ 43,966 NPS for Kiwipete. The Phase 8 no-new-search-feature baseline measured
23,996 / 356 / 67,404 and 8,668 / 202 / 42,910 respectively. Scores/moves were
unchanged (+110/e2e4 and -36/e2a6). The comparison is reproducible but timing
values should not be read as a controlled hardware benchmark across dates.

## Perft performance

Command: `powershell -ExecutionPolicy Bypass -File .\tests\run_perft_benchmark.ps1`

Both current runs returned every expected node count. The original baseline is
the recorded Release run in `perft_baseline.md`; two current runs are listed to
show timing spread. Values are `elapsed ms / NPS`.

| Position / depth | Original baseline | Current run 1 | Current run 2 |
|---|---:|---:|---:|
| Start / 5 | 5,801 / 838,802 | 5,583 / 871,466 | 5,506 / 883,587 |
| Kiwipete / 4 | 4,308 / 948,326 | 4,554 / 897,121 | 4,450 / 918,025 |
| Position 3 / 4 | 56 / 770,326 | 56 / 770,571 | 55 / 783,345 |
| Position 4 / 4 | 452 / 935,383 | 483 / 873,627 | 466 / 905,741 |
| Position 5 / 4 | 2,261 / 930,528 | 2,361 / 890,688 | 2,312 / 909,578 |
| Position 6 / 4 | 4,015 / 969,975 | 4,235 / 919,434 | 4,174 / 932,944 |

Perft throughput is mixed: start improved; Kiwipete and some tactical positions
were slower by a few percent. The added cache/hash bookkeeping is retained for
search acceleration and Phase 8 functionality, not claimed as a universal perft
speedup. Rerun on the target machine before drawing a hardware-specific
conclusion.

## Final verification

Commands executed:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_search_tests.ps1
.\tests\test_runner_search.exe --fast
powershell -ExecutionPolicy Bypass -File .\build_release.ps1
.\tests\test_runner_release.exe --fast
.\tests\test_runner_release.exe --slow
powershell -ExecutionPolicy Bypass -File .\build_engine.ps1
powershell -ExecutionPolicy Bypass -File .\tests\test_uci.ps1
powershell -ExecutionPolicy Bypass -File .\tests\run_benchmarks.ps1
powershell -ExecutionPolicy Bypass -File .\tests\run_perft_benchmark.ps1
```

- Search fast suite: 1,753 passed, 0 failed.
- Release slow suite (includes the fast cases): 2,504 passed, 0 failed. No
  tests were skipped.
- Exact minimax comparisons: all 20 negamax and 20 alpha-beta positions passed
  in the search suite.
- UCI integration: passed against `knightmare.exe`.
- Search benchmark: all five positions completed requested depths; all variants
  preserved score, best move, legality and board/history.
- Perft benchmark: all six node counts matched in both current runs.

## Final build and launch

```powershell
powershell -ExecutionPolicy Bypass -File .\build_engine.ps1
.\knightmare.exe
```

The UCI loop reads commands from stdin and writes protocol output to stdout.
The 16 MiB hash can be changed with `setoption name Hash value <MB>` and cleared
with `setoption name Clear Hash`.

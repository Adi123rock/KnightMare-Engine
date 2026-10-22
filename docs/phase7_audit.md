# Phase 7 Search Audit and Verification

Audit updated 2026-10-21 (Asia/Calcutta). The requested source specifications,
“Phase 7: What We Will Do” and “Phase 7: Functions to Make in Each Step,”
were not present in this workspace or the two sibling Knightmare folders
searched. `docs/phase7_search.md` is available and was read; the supplied task
description was used as the detailed acceptance checklist. The workspace has
no `.git` directory, so a Git diff and historical pre-existing-change list
cannot be produced. No files in `engine/` were edited.

## Initial state and baseline

The initial implementation already had production search in `engine2/`, a
test-only minimax in `tests/`, a search test suite registered by
`tests/run_all.cpp`, and `build_search_tests.ps1`. The first available search
binary reported 1,619 passes and two failures. The failures were in
`testAvoidStalemate`: it used an invalid KQK FEN (the black king was already
attacked while White was to move), leading to the material-win assertion
failing, and the same invalid position tripped a dependent terminal assertion.
The initial baseline was run from the existing executable; it was rebuilt
before subsequent verification.

| Requirement | Status | Evidence / limitation |
|---|---|---|
| 1. Search types and mate helpers | **COMPLETE AND VERIFIED** | `search_types.h` defines centipawn scores, `MATE=30000`, `INF=32001`, `MAX_PLY=64`, threshold and mate helpers; helper boundary and formatting tests pass. |
| 2. Test-only minimax reference | **COMPLETE AND VERIFIED** | The reference is in `tests/minimax_reference.cpp`, uses no production search or ordering calls, independently checks repetition, and uses matching static leaf evaluation and mate scoring. 20 exact comparisons passed for both search modes; board and history restoration are checked. |
| 3. Negamax | **COMPLETE AND VERIFIED** | Added full-width negamax mode for direct comparison. Exact scores matched the independent minimax reference on 20 positions, with depths 1–3 covered. |
| 4. Terminal and draw handling | **COMPLETE AND VERIFIED** | Checkmate/stalemate, insufficient material, fifty-move, and repetition are covered. Checkmate precedence over draw, root draw scoring, knight-cycle root/interior repetition, history restoration, and pawn-move scan reset pass. |
| 5. Fail-soft alpha-beta | **COMPLETE AND VERIFIED** | Exact scores matched the independent minimax reference on 20 positions at depths 1–4 with quiescence disabled on both sides. This covers equivalent fixed-depth trees and includes forced-response and mate-distance positions. |
| 6. MVV-LVA and promotion ordering | **COMPLETE AND VERIFIED** | Isolated MVV-LVA/promotion configuration preserved scores and reduced nodes on both fixed-depth benchmarks. See measurements below. |
| 7. PV move, killers, history | **COMPLETE AND VERIFIED** | Each heuristic was benchmarked independently; scores and best moves matched the no-order baseline, legal moves were returned, and board/history state was restored. See measurements below. |
| 8. Quiescence search | **COMPLETE AND VERIFIED** | Stand-pat, quiet evasions beyond qply 2, max-qply in-check safeguard, draw/stop behavior, board/history restoration, and Kiwipete termination pass focused regressions. |
| 9. Iterative deepening and PV | **COMPLETE AND VERIFIED** | Last completed result and PV are preserved across interruption; direct, iterative, and interrupted PV sequences are validated move by move for legality. |
| 10. Time and stop handling | **COMPLETE AND VERIFIED** | 1 ms, 100 ms, pre-set stop, asynchronous cancellation after depth one, monotonic time, and budget decisions pass focused regressions. |
| 11. Output and diagnostics | **COMPLETE AND VERIFIED** | Captured output checks verify mate signs/distances, zero-time NPS safety, and bestmove output. |
| 12. Search tests and runner | **COMPLETE AND VERIFIED** | Search tests are included and registered in fast and slow modes. Final fast run: 1,716 passed, 0 failed. Final slow run: 2,430 passed, 0 failed. |
| 13. Build integration | **COMPLETE AND VERIFIED** | `build_search_tests.ps1` rebuilt successfully using existing engine files and `engine2/` sources. Legacy `build_tests.ps1` intentionally does not include search. |
| 14. Benchmarks and documentation | **COMPLETE AND VERIFIED** | Fixed-depth node/time/NPS comparisons were run for baseline, each individual heuristic, and complete ordering on two positions. Results and reproducible configurations are below. |

## Fixes in this audit

- Quiescence now continues searching every legal evasion whenever the side to
  move is in check, instead of reverting to stand-pat after two evasion plies.
- Quiescence checks terminal positions before repetition/draw rules and handles
  a checked position at the hard qsearch bound by searching its legal evasions
  one final ply.
- Alpha-beta checks terminal positions before repetition/fifty-move/material
  draws, so checkmate takes precedence.
- Root draw positions now score zero while returning a legal move when one
  exists.
- Mate diagnostics now preserve the sign for losing mate scores and display
  mate distance in whole moves.
- Replaced the invalid KQK FEN in the stalemate/winning-material test and added
  regressions for checkmate at halfmove 100 and a drawn root.
- Added optional ordering controls and a no-pruning negamax mode for controlled
  verification and benchmarks. Defaults preserve the complete existing search
  behavior.
- Repaired the test-only minimax reference to use matching static evaluation,
  terminal ply scores, and independently implemented draw detection.

## Commands

```powershell
powershell -ExecutionPolicy Bypass -File .\build_search_tests.ps1
.\tests\test_runner_search.exe --fast
.\tests\test_runner_search.exe --slow
```

The search is callable from C++ through `engine2/search.h` as
`search(Board&, SearchLimits)`. No UCI command loop is implemented in Phase 7.

## Ordering benchmark from the latest slow run

All benchmark configurations used `evaluate()`, fixed depth, no quiescence,
and the same alpha-beta implementation. The baseline disables optional move
ordering. The `mvv_lva_promotion`, `pv`, `killer`, and `history` configurations
enable only that heuristic; `complete` enables all four. Every run had the
same score and best move within its position. The first number in parentheses
is the node reduction relative to the no-order baseline.

| Position / config | Depth | Score | Best move | Nodes (reduction) | Elapsed | NPS |
|---|---:|---:|---|---:|---:|---:|
| Start, baseline | 5 | +110 | e2e4 | 140,181 (0%) | 1,936 ms | 72,407 |
| Start, MVV-LVA + promotion | 5 | +110 | e2e4 | 91,496 (34.7%) | 1,326 ms | 69,001 |
| Start, PV only | 5 | +110 | e2e4 | 114,478 (18.3%) | 1,553 ms | 73,714 |
| Start, killer only | 5 | +110 | e2e4 | 34,400 (75.5%) | 518 ms | 66,409 |
| Start, history only | 5 | +110 | e2e4 | 34,407 (75.5%) | 530 ms | 64,918 |
| Start, complete | 5 | +110 | e2e4 | 24,967 (82.2%) | 398 ms | 62,731 |
| Kiwipete, baseline | 4 | -36 | e2a6 | 571,748 (0%) | 8,035 ms | 71,157 |
| Kiwipete, MVV-LVA + promotion | 4 | -36 | e2a6 | 9,151 (98.4%) | 206 ms | 44,422 |
| Kiwipete, PV only | 4 | -36 | e2a6 | 539,548 (5.6%) | 7,613 ms | 70,871 |
| Kiwipete, killer only | 4 | -36 | e2a6 | 337,843 (40.9%) | 4,839 ms | 69,816 |
| Kiwipete, history only | 4 | -36 | e2a6 | 310,254 (45.7%) | 4,455 ms | 69,641 |
| Kiwipete, complete | 4 | -36 | e2a6 | 9,189 (98.4%) | 209 ms | 43,966 |

PV ordering alone gave a smaller reduction than other heuristics on these
positions. It still reduced nodes and did not change scores or best moves.
Complete ordering was fastest by node count, though NPS varied by configuration.
Benchmark FENs: start position `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1`;
Kiwipete `r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1`.
Node totals include all iterative-deepening passes through the listed depth;
each configuration starts with a fresh search state.

The existing production search performance measurements remain separate:

| Position / setting | Completed depth | Nodes | Elapsed | NPS | Result |
|---|---:|---:|---:|---:|---|
| Kiwipete, fixed depth 5, quiescence on | 5 | 855,407 | 40,047 ms | 21,360 | score +66 cp, best move d5e6 |
| Starting position, 1,000 ms budget | 5 | 31,954 | 709 ms | 45,069 | legal move returned |

The time manager declined to start another depth once the conservative
half-budget gate was reached, so the one-second budget run stopped at 648 ms.
These values are machine and load sensitive.

## Exact minimax comparison records

The 40 per-case records (20 negamax plus 20 alpha-beta), including each FEN,
depth, reference score, production score, and result, are in
[`phase7_minimax_results.txt`](phase7_minimax_results.txt). All 40 passed with
exact score equality. Negamax covers depths 1–3; alpha-beta covers depths 1–4.

## Remaining verification limits

- The two original Phase 7 specification documents remain unavailable.
- Hardware model could not be read in this environment; compiler was MinGW-w64
  g++ 16.2.0 with C++17, `-O2`, and `-g` as configured by the build script.
- No Git metadata is present to establish pre-existing changes. Engine files
  were only read; all edits in this audit are limited to `engine2/`, `tests/`,
  and `docs/`.

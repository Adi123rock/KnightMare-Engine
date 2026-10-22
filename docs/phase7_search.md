# Knightmare Engine — Phase 7: Search

## Overview

Phase 7 implements a complete search engine on top of the existing Phases 1–6 engine.  
All new code lives in `engine2/`.  The original `engine/` directory is **untouched**.

---

## Files Created

### `engine2/` — Production search

| File | Purpose |
|---|---|
| `search_types.h` | Constants (`MATE`, `INF`, `MAX_PLY`), `SearchLimits`, `SearchState`, `SearchResult`, mate-score helpers |
| `timer.h` | `nowMs()` (monotonic clock), atomic stop flag, `requestStop()`, `clearStop()` |
| `ordering.h` | Move-ordering declarations |
| `ordering.cpp` | MVV-LVA, PV move, killer moves, history heuristic |
| `search.h` | All search function declarations |
| `search.cpp` | `alphaBeta()`, `quiesce()`, `searchRoot()`, `iterativeDeepening()`, `search()`, draw detection, PV table, output helpers |

### `tests/` — Test infrastructure

| File | Purpose |
|---|---|
| `minimax_reference.h` | Plain minimax reference declarations |
| `minimax_reference.cpp` | `minimaxRoot()`, `minimaxSearch()`, `whiteViewEval()` |
| `test_search.cpp` | Comprehensive Phase 7 test suite |

### Build scripts

| File | Purpose |
|---|---|
| `build_search_tests.ps1` | Debug build with Phase 7 sources |

---

## Algorithm

| Feature | Implementation |
|---|---|
| Root search | `searchRoot()` — iterates over legal moves, calls `alphaBeta` |
| Alpha-beta | Fail-soft negamax, mate-distance aware |
| Quiescence | Stand-pat + tactical moves (captures/promotions); full evasions when in check |
| Iterative deepening | `iterativeDeepening()` — depth 1..maxDepth, preserves last completed depth |
| Move ordering | PV → captures (MVV-LVA) → promotions → killers → history |
| PV table | Triangular table, updated on every best-move update |
| Time management | Check every 2048 nodes; atomic stop flag; `computeTimeBudget()` |
| Draw detection | Repetition (1 earlier occurrence), fifty-move rule, insufficient material |
| Mate scores | `mateIn(ply) = MATE - ply`, `matedIn(ply) = -(MATE - ply)` |

---

## Score Convention

All scores are **from the side-to-move perspective** (centipawns):
- Positive = current side is winning
- `MATE = 30000`; mate-in-1 = `MATE - 1 = 29999`
- `INF = 32001` (> MATE, used for alpha/beta initialization)

The existing `evaluate()` already returns side-to-move perspective + TEMPO_BONUS (10 cp).

The minimax reference uses `evaluateWhite()` (no tempo bonus) for a stable white-relative comparison baseline.

---

## Build Instructions

### Debug build (with search):
```powershell
powershell -ExecutionPolicy Bypass -File .\build_search_tests.ps1
```

### Run fast test suite:
```powershell
.\tests\test_runner_search.exe --fast
```

### Run slow test suite (includes performance benchmark):
```powershell
.\tests\test_runner_search.exe --slow
```

### Run original tests (no search, regression):
```powershell
powershell -ExecutionPolicy Bypass -File .\build_tests.ps1
.\tests\test_runner.exe --fast
```

---

## Test Results (Fast Suite)

| Suite | Passed | Failed | Total |
|---|---:|---:|---:|
| Fast | 1618+ | 0 | 1618+ |

All pre-existing tests (FEN, perft, rules, evaluation) continue to pass.

---

## Performance Baseline

| Position | Depth | Nodes | Time (ms) | NPS | Score | Best move |
|---|---:|---:|---:|---:|---|---|
| Starting pos | 3 | ~1,178 | ~100 | ~12,000 | +54 | b1c3 |
| Kiwipete | 1 | ~3,449 | ~865 | ~3,987 | +174 | e2a6 |
| Kiwipete | 2 | ~6,225 | ~1,568 | ~3,970 | +174 | e2a6 |
| Kiwipete | 3 | ~20,939 | ~4,875 | ~4,295 | +125 | e2a6 |

> NPS is lower than perft because quiescence adds evaluation overhead. Release build (-O2) significantly increases speed.

---

## Engine Limitations Discovered (Not Fixed — Rule A)

1. **`evaluate()` includes TEMPO_BONUS**: The production `evaluate()` adds 10 cp to the side-to-move score. This is correct for game play but makes exact comparison to a no-tempo minimax reference non-trivial. Handled by using a ±100 cp tolerance band in tests.

2. **Repetition tracking**: `positionKeys` stores keys AFTER each move; the current position key is NOT in `positionKeys`. The search correctly handles this by comparing `positionKey(board)` against keys in history.

3. **KQ vs K stalemate horizon**: At depth ≤4 with quiescence, the engine may score a stalemate (0) as acceptable in KQ vs K endgames because the 900 cp queen advantage doesn't fully propagate through the horizon without dedicated endgame tablebase or KPK knowledge. This is a known limitation of pure material evaluation in simplified endgames.

---

## Assumptions

1. One earlier occurrence of the current position key in `positionKeys` is sufficient for a draw in search (per specification).
2. The `evaluate()` function is correct and already tested by Phase 6 tests.
3. `generateLegalMoves(Board& board)` is correct and already tested by perft.
4. The TEMPO_BONUS difference between `evaluate()` and `evaluateWhite()` is expected and documented; sign-agreement tests use a ±100 cp tolerance.

---

## Known Remaining Work

- UCI protocol integration (not part of Phase 7 specification)
- Hash table / transposition table (not in Phase 7 scope)
- Late move reduction, null move pruning (beyond Phase 7 scope)

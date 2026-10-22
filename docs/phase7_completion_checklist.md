# Phase 7 Remaining Work Checklist

Created 2026-10-21 from `docs/phase7_audit.md`, the supplied Phase 7 acceptance
criteria, and inspection of the current implementation. The two original
specification documents remain unavailable. There is no `.git` directory in
this workspace, so Git diff/status evidence cannot be obtained.

## Outstanding items found

| Requirement | Final status | Behavior and files inspected | Verification evidence |
|---|---|---|---|
| Search types and mate helpers | COMPLETE AND VERIFIED | Score constants, mate normalization and signed mate distance are defined and tested. | Helper boundary, odd/even distance, and formatter tests pass. |
| Independent minimax reference | COMPLETE AND VERIFIED | The reference remains test-only and independently handles evaluation, terminals, and draws. | 20 unique exact comparisons for each production mode; all pass. |
| Negamax | COMPLETE AND VERIFIED | Full-window negamax matches the equivalent reference tree. | 20 unique positions, depths 1-3; exact scores. |
| Repetition and draw-state verification | COMPLETE AND VERIFIED | Root and interior knight-cycle repetition, draw score, board/history preservation, and pawn-move scan reset are covered. | `testRepetitionHistory`; fast and slow suites pass. |
| Fail-soft alpha-beta | COMPLETE AND VERIFIED | Alpha-beta and reference use equivalent fixed-depth trees and terminal scoring. | 20 unique positions, depths 1-4; exact scores. |
| MVV-LVA and promotion ordering | COMPLETE AND VERIFIED | Capture and promotion ordering controls are independently configurable. | Isolated benchmark configurations preserve score and best move; see `phase7_audit.md`. |
| PV, killer, and history ordering | COMPLETE AND VERIFIED | Ordering controls can be enabled independently or together. | Each benchmark preserves score, best move, legality, and board/history; see `phase7_audit.md`. |
| Quiescence edge coverage | COMPLETE AND VERIFIED | Fail-soft stand pat, quiet evasions after qply 2, max-qply in-check safeguard, fifty-move draw, stop handling, restoration, and Kiwipete termination are covered. | `testQuiescenceEdgeCases` plus Kiwipete search; fast and slow suites pass. |
| Iterative interruption and completed result state | COMPLETE AND VERIFIED | `SearchResult` exposes completed/interrupted state; incomplete depths preserve the latest completed result or legal fallback. | Pre-stop, 1 ms tactical interruption, and output-triggered stop after depth one pass with legal/restored state. |
| Principal variation | COMPLETE AND VERIFIED | `SearchResult` exposes the PV; every returned PV move is checked against successive legal move lists. | Direct and iterative PV tests pass; diagnostics and bestmove formatting are captured exactly. |
| Mate distance | COMPLETE AND VERIFIED | Losing mate distances are negative; odd/even distances and positive/negative mate formatter output are covered. | `mateDistance` and captured diagnostic output regressions pass. |
| Root terminal handling | COMPLETE AND VERIFIED | Checkmate, stalemate, draw roots, and checkmate precedence over the fifty-move rule are covered; terminal root completes at depth zero. | Terminal and root-draw regressions pass in fast and slow suites. |
| Timing / cancellation | COMPLETE AND VERIFIED | Monotonic clock/budget decisions and cancellation preserve complete results, legal fallback, PV, and board/history. | 1 ms, 100 ms, preset stop, and asynchronous stop after depth one regressions pass. |
| Output and diagnostics | COMPLETE AND VERIFIED | PV and info formatting, mate score sign/distance, bestmove output, and zero-time NPS behavior are covered. | Captured output regressions pass. |
| Test/build integration | COMPLETE AND VERIFIED | Search tests are included in both suite modes and the build script. | Build succeeds; final fast and slow runs pass. |
| Benchmarks and documentation | COMPLETE AND VERIFIED | Baselines and controlled ordering comparisons are recorded. | Latest performance and ordering records are in `phase7_audit.md`; checklist records final suite counts. |

## Work tracking

All remaining entries are now complete and verified by focused regressions
and final fast/slow suites. Other Phase 7 requirements are recorded in
`phase7_audit.md`.

## Final verification (2026-10-21)

- `powershell -ExecutionPolicy Bypass -File .\build_search_tests.ps1` — build
  successful.
- `.\tests\test_runner_search.exe --fast` — 1,716 passed, 0 failed.
- `.\tests\test_runner_search.exe --slow` — 2,430 passed, 0 failed.
- Both suites include 20 unique exact-minimax cases per mode; all 40 passed.
- Ordering benchmark cases all preserved exact score, best move, move legality,
  and board/history state.
- No `.git` directory exists in this workspace, so Git status/diff checks are
  unavailable. No files in `engine/` were changed for this task.

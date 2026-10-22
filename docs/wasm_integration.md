# Knightmare Engine WebAssembly integration

Updated 2026-10-21. The C++ engine builds into a standalone, single-thread
WebAssembly module and is loaded by a Vite module Worker in the React app. The
adapter and UI call the existing engine; they do not implement chess rules,
legal moves, evaluation, or search in JavaScript.

## Toolchain

The configured SDK is `C:\emsdk` (Emscripten 6.0.13). Check the compiler with:

```powershell
$env:EMSDK = 'C:/emsdk'
$env:EMSDK_PYTHON = 'C:/emsdk/python/3.13.3_64bit/python.exe'
& 'C:\emsdk\upstream\emscripten\em++.exe' --version
```

`build_wasm.ps1` resolves `EMSDK` and the SDK Python automatically (or uses
`C:\emsdk` by default). It sets environment variables only in its own
PowerShell process; it does not run the SDK activation script or change
machine-wide settings. It redirects compiler temporary files to the ignored
`wasm/.build-tmp/` directory because this environment denies Emscripten's
default temp path.

## Build and test

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_wasm.ps1
node .\wasm\test_wasm.mjs
```

Generated files are placed in `knightmare-web/public/engine/`:

- `knightmare.js` — Emscripten ES module glue.
- `knightmare.wasm` — compiled C++ engine and adapter.

The Node harness instantiates these generated artifacts and exercises the real
WASM module. The current website harness verifies that starting-position
searches at depths 2, 3, and 4 complete at the matching engine depth and return
a legal move among the 20 generated root moves. It also verifies a valid
opening FEN search at depth 3, rejection of malformed FEN without changing the
loaded position, rejection of invalid C++ API depths 0 and 65 without changing
the position, and a repeated legal search after the invalid FEN request. The
website supports only depths 2 through 4 for responsiveness; the C++ adapter's
general `1..MAX_PLY` capability and native UCI depth support are unchanged.

Native regression commands run after the adapter was added:

```powershell
powershell -ExecutionPolicy Bypass -File .\build_engine.ps1
powershell -ExecutionPolicy Bypass -File .\build_search_tests.ps1
.\tests\test_runner_search.exe --fast
powershell -ExecutionPolicy Bypass -File .\tests\test_uci.ps1
```

Native UCI build succeeded, the fast suite passed 1,753 / 1,753 tests, and the
UCI integration checks passed. `build_engine.ps1` and `src/uci_main.cpp` were
not changed.

## Adapter/API behavior

`wasm/knightmare_adapter.cpp` wraps the APIs found in the engine source:

- `Board::loadFEN(fen, &error)` loads into a temporary board. The adapter then
  calls `validatePosition`; it commits the candidate only if both steps pass.
- `generateLegalMoves(Board&)` supplies legal moves, formatted by the existing
  `moveToString()` helper. The adapter returns these to JavaScript for result
  verification or display.
- `search(Board&, SearchLimits)` performs the existing iterative-deepening
  search. The adapter bounds depth to `1..MAX_PLY`, searches a copy of the
  loaded root board, and validates any returned best move against the legal
  moves generated from that root position.
- `reset()`, `loadFen()`, `fen()`, `legalMoves()`, `searchBestMove(depth)`, and
  `lastError()` are exposed through Embind on `KnightmareEngine`.

The build includes the engine implementation files from `build_engine.ps1`
except `src/uci_main.cpp`, plus the adapter. The native UCI executable remains
separate.

## Limits and cancellation

This first build is single-threaded and `searchBestMove()` is synchronous. A
call made directly on the browser's UI thread will block that thread until the
requested depth finishes. The module is configured for Web Workers, so the
caller can run the whole module inside a dedicated Worker to keep the UI
responsive. However, a synchronous search occupies that Worker and it cannot
process a `stop` message during the call. The engine's global atomic
`g_stopSearch` is reset before each search, but the adapter does not expose a
working browser cancellation method; cancellation has not been tested or
claimed. Do not run multiple searches concurrently: the engine's transposition
table and stop flag are global.

Search info lines are still emitted through the engine's existing C++ output
path. The harness suppresses them using Emscripten's `print` callback. A future
UI integration can route this output to progress reporting or suppress it.

## React Worker integration

`knightmare-web/src/workers/knightmare.worker.ts` is imported with Vite's
`?worker` syntax. It dynamically imports the generated public Emscripten module
from `/engine/knightmare.js` and provides the adjacent `.wasm` URL through
`locateFile`. Initialization errors, worker runtime errors, and engine search
errors are returned to the UI.

Each search request contains a monotonically increasing request ID, the current
`chess.js` FEN, and the selected website search depth (2, 3, or 4; default 4). The Game
Room selector updates the value used by the next request without recreating the
Worker. The Worker rejects values outside the website selector range before
loading a position and forwards the accepted depth unchanged to
`searchBestMove(depth)`. The C++ adapter bounds the integer to `1..MAX_PLY`,
sets `SearchLimits::maxDepth` to that value, and reports the completed depth;
the harness checks that it matches each requested value. The Worker calls `loadFen()` for
every search before invoking `searchBestMove()`, so it cannot silently reuse a
previous position. React accepts a result only when both its ID and FEN still
match the active game. It checks the UCI move format, replays the current PGN
history into `chess.js`, and applies the move with `chess.js`; malformed or
illegal results become visible engine errors. Initialization, thinking, ready,
and error states appear in the game panel. Local Two Player remains independent
of the Worker.

The Worker is created only in Knightmare mode. Leaving that mode terminates its
Worker instance. During a synchronous search, the Worker event loop cannot
process another message. Searches therefore execute serially, but messages can
queue behind an active search. Starting a new game or changing the position
invalidates the old request in React; a late result is discarded by request ID
and FEN. The engine's global `g_stopSearch` has no cooperative JS cancellation
API in this build. An obsolete search may run to completion before the Worker
can process a later search. Switching out of engine mode terminates the Worker
as a whole; this is not graceful search cancellation. Do not report true
in-search cancellation as supported.

No pthreads, cross-origin isolation, or native engine changes are needed. The
WASM harness verifies the Embind API under Node.js; `npm run build` verifies
Vite's Worker bundling. A browser-specific end-to-end Worker session has not
been separately automated in this milestone.

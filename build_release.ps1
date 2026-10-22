
$ErrorActionPreference = "Stop"

Write-Host "`nBuilding Knightmare Engine tests (Release -O2)..." -ForegroundColor Cyan

g++ -std=c++17 -O2 -DNDEBUG -Wall -Wextra `
    tests/run_all.cpp `
    tests/test_fen.cpp `
    tests/test_validate.cpp `
    tests/perft.cpp `
    tests/test_rules.cpp `
    tests/test_mirror.cpp `
    tests/minimax_reference.cpp `
    engine/Board.cpp `
    engine/MoveGenerator.cpp `
    engine/attacks.cpp `
    engine/legal.cpp `
    engine/rules.cpp `
    engine/validate.cpp `
    engine2/search.cpp `
    engine2/ordering.cpp `
    -o tests/test_runner_release.exe

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nRelease build failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "`nRelease build successful!" -ForegroundColor Green

$ErrorActionPreference = "Stop"

Write-Host "`nBuilding Knightmare Engine Search Tests..." -ForegroundColor Cyan

g++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -g `
    tests/run_all.cpp `
    tests/test_fen.cpp `
    tests/test_validate.cpp `
    tests/test_rules.cpp `
    tests/perft.cpp `
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
    -o tests/test_runner_search.exe

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nBuild failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "`nBuild successful: tests/test_runner_search.exe" -ForegroundColor Green

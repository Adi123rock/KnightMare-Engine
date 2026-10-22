
$ErrorActionPreference = "Stop"

Write-Host "`nBuilding Knightmare Engine tests..." -ForegroundColor Cyan

g++ -std=c++17 -Wall -Wextra -g `
    tests/run_all.cpp `
    tests/test_fen.cpp `
    tests/test_validate.cpp `
    tests/test_rules.cpp `
    tests/perft.cpp `
    tests/test_mirror.cpp `
    engine/Board.cpp `
    engine/MoveGenerator.cpp `
    engine/attacks.cpp `
    engine/legal.cpp `
    engine/rules.cpp `
    engine/validate.cpp `
    -o tests/test_runner.exe

if ($LASTEXITCODE -ne 0) {
    Write-Host "`nBuild failed." -ForegroundColor Red
    exit $LASTEXITCODE
}

Write-Host "`nBuild successful!" -ForegroundColor Green

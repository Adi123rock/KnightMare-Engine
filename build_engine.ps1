$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$output = Join-Path $root "knightmare.exe"
Write-Host "Building Knightmare UCI engine..." -ForegroundColor Cyan
g++ -std=c++17 -O2 -DNDEBUG -Wall -Wextra -pthread `
    src/uci_main.cpp `
    engine/Board.cpp `
    engine/MoveGenerator.cpp `
    engine/attacks.cpp `
    engine/legal.cpp `
    engine/rules.cpp `
    engine/validate.cpp `
    engine2/search.cpp `
    engine2/ordering.cpp `
    -o $output
if ($LASTEXITCODE -ne 0) {
    Write-Host "UCI engine build failed." -ForegroundColor Red
    exit $LASTEXITCODE
}
Write-Host "Build successful: $output" -ForegroundColor Green

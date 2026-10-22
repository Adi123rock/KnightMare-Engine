$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$output = Join-Path $root "bench_perft.exe"
g++ -std=c++17 -O2 -DNDEBUG -Wall -Wextra `
    tests/bench_perft.cpp tests/perft.cpp `
    engine/Board.cpp engine/MoveGenerator.cpp engine/attacks.cpp `
    engine/legal.cpp engine/rules.cpp engine/validate.cpp `
    -o $output
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $output
exit $LASTEXITCODE

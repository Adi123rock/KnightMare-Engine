$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$output = Join-Path $root "bench_search.exe"
g++ -std=c++17 -O2 -DNDEBUG -Wall -Wextra -pthread `
    tests/bench_search.cpp `
    engine/Board.cpp `
    engine/MoveGenerator.cpp `
    engine/attacks.cpp `
    engine/legal.cpp `
    engine/rules.cpp `
    engine/validate.cpp `
    engine2/search.cpp `
    engine2/ordering.cpp `
    -o $output
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $output
exit $LASTEXITCODE

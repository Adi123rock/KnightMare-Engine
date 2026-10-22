$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$exe = Join-Path $repoRoot "knightmare.exe"
if (-not (Test-Path -LiteralPath $exe)) { throw "Build knightmare.exe first with build_engine.ps1" }
$start = [System.Diagnostics.ProcessStartInfo]::new()
$start.FileName = $exe
$start.UseShellExecute = $false
$start.RedirectStandardInput = $true
$start.RedirectStandardOutput = $true
$start.RedirectStandardError = $true
$start.CreateNoWindow = $true
$proc = [System.Diagnostics.Process]::new()
$proc.StartInfo = $start
if (-not $proc.Start()) { throw "Could not start UCI engine" }
$writer = $proc.StandardInput
$writer.AutoFlush = $true
$captured = [System.Collections.Generic.List[string]]::new()

function Read-UciLine([int]$timeoutMs = 10000) {
    $task = $proc.StandardOutput.ReadLineAsync()
    if (-not $task.Wait($timeoutMs)) { throw "Timed out waiting for UCI output" }
    if ($null -eq $task.Result) { throw "Engine closed stdout unexpectedly" }
    $captured.Add($task.Result)
    return $task.Result
}
function Wait-UciToken([string]$expected, [int]$timeoutMs = 15000) {
    $clock = [Diagnostics.Stopwatch]::StartNew()
    while ($clock.ElapsedMilliseconds -lt $timeoutMs) {
        $line = Read-UciLine ([Math]::Max(1, $timeoutMs - [int]$clock.ElapsedMilliseconds))
        if ($line -match $expected) { return $line }
    }
    throw "Did not receive expected UCI output: $expected"
}

try {
    $writer.WriteLine("uci")
    Wait-UciToken '^uciok$' | Out-Null
    if (-not ($captured -contains "id name Knightmare Engine")) { throw "Missing UCI identity" }
    $writer.WriteLine("isready")
    Wait-UciToken '^readyok$' | Out-Null

    $writer.WriteLine("ucinewgame")
    $writer.WriteLine("position startpos moves e2e4 e7e5")
    $writer.WriteLine("go depth 2")
    Wait-UciToken '^bestmove (0000|[a-h][1-8][a-h][1-8][qrbn]?)$' | Out-Null
    if (-not ($captured | Where-Object { $_ -match '^info depth 2 score .* nodes \d+ time \d+ nps \d+ pv ' })) {
        throw "Depth search did not emit the required info fields"
    }

    $writer.WriteLine("position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    $writer.WriteLine("go nodes 100")
    Wait-UciToken '^bestmove (0000|[a-h][1-8][a-h][1-8][qrbn]?)$' | Out-Null

    $writer.WriteLine("position fen rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 moves e2e4 e7e5")
    $writer.WriteLine("go depth 1")
    Wait-UciToken '^bestmove (0000|[a-h][1-8][a-h][1-8][qrbn]?)$' | Out-Null

    $writer.WriteLine("setoption name Clear Hash")
    $writer.WriteLine("go movetime 10")
    Wait-UciToken '^bestmove (0000|[a-h][1-8][a-h][1-8][qrbn]?)$' | Out-Null

    $writer.WriteLine("position startpos")
    $writer.WriteLine("go wtime 0 btime 1000 winc 50 binc 50 movestogo 20")
    Wait-UciToken '^bestmove (0000|[a-h][1-8][a-h][1-8][qrbn]?)$' | Out-Null

    $writer.WriteLine("go infinite")
    Start-Sleep -Milliseconds 100
    $writer.WriteLine("isready")
    Wait-UciToken '^readyok$' | Out-Null
    $writer.WriteLine("stop")
    Wait-UciToken '^bestmove (0000|[a-h][1-8][a-h][1-8][qrbn]?)$' | Out-Null

    $writer.WriteLine("quit")
    if (-not $proc.WaitForExit(5000)) { throw "Engine did not exit after quit" }
    $captured | ForEach-Object { Write-Output $_ }
    Write-Output "UCI integration checks passed."
} finally {
    if (-not $proc.HasExited) { $writer.WriteLine("stop"); $writer.WriteLine("quit"); $proc.Kill() }
    $proc.Dispose()
}

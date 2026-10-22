$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$sdkRoot = if ($env:EMSDK) { $env:EMSDK } else { "C:\emsdk" }
$compiler = Join-Path $sdkRoot "upstream\emscripten\em++.exe"
$python = $env:EMSDK_PYTHON

if (-not (Test-Path -LiteralPath $compiler)) {
    throw "Emscripten compiler not found at '$compiler'. Set EMSDK to the installed SDK directory."
}
if (-not $python -or -not (Test-Path -LiteralPath $python)) {
    $python = Get-ChildItem -LiteralPath (Join-Path $sdkRoot "python") -Filter python.exe -Recurse -File -ErrorAction SilentlyContinue |
        Sort-Object FullName -Descending |
        Select-Object -First 1 -ExpandProperty FullName
}
if (-not $python) {
    throw "Emscripten Python was not found. Install/repair the SDK at '$sdkRoot'."
}

# Keep the SDK setup local to this PowerShell process. Do not run emsdk_env.bat,
# which attempts to write a helper script into the SDK installation directory.
$env:EMSDK = ($sdkRoot -replace "\\", "/")
$env:EMSDK_PYTHON = $python
$env:PATH = "$sdkRoot;$sdkRoot\upstream\emscripten;$env:PATH"
$emscriptenTemp = Join-Path $root "wasm\.build-tmp"
New-Item -ItemType Directory -Path $emscriptenTemp -Force | Out-Null
$env:TEMP = $emscriptenTemp
$env:TMP = $emscriptenTemp

$outputDirectory = Join-Path $root "knightmare-web\public\engine"
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$output = Join-Path $outputDirectory "knightmare.js"

$sources = @(
    "wasm/knightmare_adapter.cpp",
    "engine/Board.cpp",
    "engine/MoveGenerator.cpp",
    "engine/attacks.cpp",
    "engine/legal.cpp",
    "engine/rules.cpp",
    "engine/validate.cpp",
    "engine2/search.cpp",
    "engine2/ordering.cpp"
) | ForEach-Object { Join-Path $root $_ }

Write-Host "Building Knightmare WebAssembly module with $compiler" -ForegroundColor Cyan
& $compiler `
    -std=c++17 -O2 -DNDEBUG -fexceptions --bind `
    -sWASM=1 `
    -sMODULARIZE=1 `
    -sEXPORT_ES6=1 `
    -sEXPORT_NAME=createKnightmareModule `
    "-sENVIRONMENT=web,worker,node" `
    -sALLOW_MEMORY_GROWTH=1 `
    -sINITIAL_MEMORY=33554432 `
    -sSTACK_SIZE=1048576 `
    -sEXIT_RUNTIME=0 `
    @sources `
    -o $output

if ($LASTEXITCODE -ne 0) {
    throw "WebAssembly build failed with exit code $LASTEXITCODE."
}

$wasm = Join-Path $outputDirectory "knightmare.wasm"
if (-not (Test-Path -LiteralPath $output) -or -not (Test-Path -LiteralPath $wasm)) {
    throw "Build reported success but expected artifacts are missing from '$outputDirectory'."
}

Write-Host "WebAssembly build successful:" -ForegroundColor Green
Write-Host "  $output"
Write-Host "  $wasm"

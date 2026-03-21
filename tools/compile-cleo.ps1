[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [string]$SannyPath = "D:\GTASAVIDEOCEKME\SannyBuilder\sanny.exe",

    [string]$Mode = "sa"
)

$ErrorActionPreference = "Stop"

function Resolve-ExistingPath {
    param([Parameter(Mandatory = $true)][string]$PathValue)
    return (Resolve-Path -LiteralPath $PathValue).Path
}

if (-not (Test-Path -LiteralPath $SannyPath)) {
    throw "Sanny Builder CLI not found at '$SannyPath'."
}

$resolvedInput = Resolve-ExistingPath -PathValue $InputPath
$resolvedOutput = [System.IO.Path]::GetFullPath($OutputPath)
$outputDir = Split-Path -Parent $resolvedOutput
if ($outputDir -and -not (Test-Path -LiteralPath $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}

$sannyDir = Split-Path -Parent $SannyPath
$startUtc = [DateTime]::UtcNow

$candidateLogs = @(
    (Join-Path (Get-Location) "compile.log"),
    (Join-Path $PSScriptRoot "compile.log"),
    (Join-Path (Split-Path -Parent $resolvedInput) "compile.log"),
    (Join-Path $outputDir "compile.log"),
    (Join-Path $sannyDir "compile.log")
) | Select-Object -Unique

$beforeLogState = @{}
foreach ($candidate in $candidateLogs) {
    if (Test-Path -LiteralPath $candidate) {
        $item = Get-Item -LiteralPath $candidate
        $beforeLogState[$candidate] = $item.LastWriteTimeUtc
    }
}

$beforeOutputWrite = $null
if (Test-Path -LiteralPath $resolvedOutput) {
    $beforeOutputWrite = (Get-Item -LiteralPath $resolvedOutput).LastWriteTimeUtc
}

& $SannyPath --no-splash --mode $Mode --compile $resolvedInput $resolvedOutput

$logMessages = @()
$touchedLogs = @()
foreach ($candidate in $candidateLogs) {
    if (-not (Test-Path -LiteralPath $candidate)) {
        continue
    }

    $item = Get-Item -LiteralPath $candidate
    $modified = $false
    if ($beforeLogState.ContainsKey($candidate)) {
        $modified = $item.LastWriteTimeUtc -gt $beforeLogState[$candidate]
    } else {
        $modified = $item.LastWriteTimeUtc -ge $startUtc
    }

    if ($modified) {
        $touchedLogs += $candidate
        $content = (Get-Content -LiteralPath $candidate -Raw).Trim()
        if ($content.Length -gt 0) {
            $logMessages += [PSCustomObject]@{
                Path = $candidate
                Content = $content
            }
        }
    }
}

$outputFresh = $false
if (Test-Path -LiteralPath $resolvedOutput) {
    $outputItem = Get-Item -LiteralPath $resolvedOutput
    if ($beforeOutputWrite) {
        $outputFresh = $outputItem.LastWriteTimeUtc -gt $beforeOutputWrite
    } else {
        $outputFresh = $true
    }
}

if ($logMessages.Count -gt 0) {
    Write-Output "Compilation failed for $resolvedInput"
    foreach ($entry in $logMessages) {
        Write-Output "compile.log: $($entry.Path)"
        Write-Output $entry.Content
    }
    exit 1
}

if (-not (Test-Path -LiteralPath $resolvedOutput)) {
    Write-Output "Compilation failed: output file was not created: $resolvedOutput"
    if ($touchedLogs.Count -gt 0) {
        Write-Output "Checked logs:"
        $touchedLogs | ForEach-Object { Write-Output $_ }
    }
    exit 1
}

$outputItem = Get-Item -LiteralPath $resolvedOutput
Write-Output "Compiled OK"
Write-Output "Input : $resolvedInput"
Write-Output "Output: $resolvedOutput"
Write-Output ("Size  : {0} bytes" -f $outputItem.Length)
if ($touchedLogs.Count -gt 0) {
    Write-Output ("Logs  : {0}" -f ($touchedLogs -join ", "))
}
if (-not $outputFresh) {
    Write-Output "Warning: output timestamp did not change during this run."
}

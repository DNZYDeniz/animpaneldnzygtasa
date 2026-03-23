$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Path $PSScriptRoot -Parent
$releaseRoot = Join-Path $projectRoot 'release\\AnimPanel-Standalone'

$copies = @(
    @{ Source = Join-Path $projectRoot 'README.md'; Target = Join-Path $releaseRoot 'README.md' },
    @{ Source = Join-Path $projectRoot 'README.txt'; Target = Join-Path $releaseRoot 'README.txt' }
)

foreach ($copy in $copies) {
    $targetDir = Split-Path -Path $copy.Target -Parent
    if (-not (Test-Path $targetDir)) {
        New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    }

    Copy-Item -Path $copy.Source -Destination $copy.Target -Force
}

Write-Host 'AnimPanel README files synced to release package.'

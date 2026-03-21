param(
    [string]$InputPath = ".\data\animgrp.dat",
    [string]$OutputPath = ".\modloader\AnimPanel\data\anim-catalog.json"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-AnimCategory {
    param(
        [string]$GroupName,
        [string]$AnimName
    )

    $value = ("{0} {1}" -f $GroupName, $AnimName).ToLowerInvariant()

    if ($value.Contains("idle") -or $value.Contains("stance")) { return "Idles" }
    if ($value.Contains("sit")) { return "Sit" }
    if ($value.Contains("lean")) { return "Lean" }
    if ($value.Contains("dance")) { return "Dance" }
    if ($value.Contains("chat") -or $value.Contains("kiss") -or $value.Contains("wave")) { return "Social" }
    if ($value.Contains("pose")) { return "Poses" }

    return "Misc"
}

function Get-AnimTags {
    param(
        [string]$GroupName,
        [string]$AnimName
    )

    $tags = New-Object System.Collections.Generic.List[string]
    $value = ("{0} {1}" -f $GroupName, $AnimName).ToLowerInvariant()

    if ($value.Contains("walk")) { $tags.Add("walk") }
    if ($value.Contains("run")) { $tags.Add("run") }
    if ($value.Contains("sprint")) { $tags.Add("sprint") }
    if ($value.Contains("idle") -or $value.Contains("stance")) { $tags.Add("idle") }
    if ($value.Contains("woman")) { $tags.Add("female") }
    if ($value.Contains("man")) { $tags.Add("male") }
    if ($value.Contains("gang")) { $tags.Add("gang") }
    if ($value.Contains("skate")) { $tags.Add("skate") }

    if ($tags.Count -eq 0) {
        $tags.Add("general")
    }

    return [string[]]@($tags | Select-Object -Unique)
}

if (-not (Test-Path $InputPath)) {
    throw "Input file not found: $InputPath"
}

$lines = Get-Content $InputPath
$entries = New-Object System.Collections.Generic.List[object]

for ($i = 0; $i -lt $lines.Count; $i++) {
    $line = $lines[$i].Trim()

    if (-not $line -or $line.StartsWith("#") -or $line -eq "end") {
        continue
    }

    if ($line -match '^[^\s].*,.*,.*,\s*\d+$') {
        $parts = $line.Split(",") | ForEach-Object { $_.Trim() }
        $groupName = $parts[0]
        $ifpFile = $parts[1]
        $count = [int]$parts[3]

        for ($offset = 1; $offset -le $count -and ($i + $offset) -lt $lines.Count; $offset++) {
            $animName = $lines[$i + $offset].Trim()

            if (-not $animName -or $animName -eq "end" -or $animName.StartsWith("#")) {
                continue
            }

            $category = Get-AnimCategory -GroupName $groupName -AnimName $animName
            $tags = Get-AnimTags -GroupName $groupName -AnimName $animName
            $poseFlag = $category -eq "Poses" -or $category -eq "Idles"
            $displayName = (($animName -replace "_", " ") -replace "\s+", " ").Trim()
            if (-not $displayName) {
                $displayName = $animName
            }

            $entryId = "{0}:{1}:{2}" -f ($ifpFile.ToLowerInvariant()), ($groupName.ToLowerInvariant()), ($animName.ToLowerInvariant())

            $entries.Add([pscustomobject]@{
                id = $entryId
                ifp_file = $ifpFile
                block = $groupName
                anim_name = $animName
                display_name = $displayName
                category = $category
                tags = [string[]]@($tags)
                loop_default = $poseFlag
                pose_flag = $poseFlag
                notes = "Seeded from data/animgrp.dat group associations."
            })
        }
    }
}

$targetDir = Split-Path -Parent $OutputPath
if (-not (Test-Path $targetDir)) {
    New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
}

$json = $entries | Sort-Object ifp_file, block, anim_name | ConvertTo-Json -Depth 5
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText((Resolve-Path -LiteralPath $targetDir | ForEach-Object { Join-Path $_ (Split-Path -Leaf $OutputPath) }), $json, $utf8NoBom)

Write-Host ("Generated {0} animation entries into {1}" -f $entries.Count, $OutputPath)

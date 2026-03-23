[CmdletBinding()]
param(
    [string]$SourcePath = "D:\GTASAVIDEOCEKME\animlist.lua",
    [string]$OutputPath = "D:\GTASAVIDEOCEKME\AnimPanel\data\anim-catalog.json"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Split-Words {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return @()
    }

    $normalized = $Value -creplace '([a-z0-9])([A-Z])', '$1 $2'
    $normalized = $normalized -replace '[_\-\/]+', ' '
    $normalized = $normalized -replace '\s+', ' '
    return @([regex]::Matches($normalized.Trim(), '[A-Za-z0-9\.]+') | ForEach-Object { $_.Value })
}

function Humanize-Name {
    param([string]$Value)

    $words = @(Split-Words $Value)
    if ($words.Count -eq 0) {
        return $Value
    }

    return (($words | ForEach-Object {
        if ($_ -match '^\d+$') { $_ }
        elseif ($_.Length -le 4 -and $_ -cmatch '^[A-Z0-9]+$') { $_ }
        else { $_.Substring(0, 1).ToUpperInvariant() + $_.Substring(1).ToLowerInvariant() }
    }) -join ' ')
}

function Shorten-DisplayName {
    param([string]$Value)

    $text = ($Value ?? '').Trim().TrimEnd('.')
    if ([string]::IsNullOrWhiteSpace($text)) {
        return $text
    }

    $text = $text -replace '\banimation loop\b', 'loop'
    $text = $text -replace '\banimation\b', ''
    $text = $text -replace '\banimations\b', ''
    $text = $text -replace '\bAlternative\b', 'Alt'
    $text = $text -replace '\bposition\b', 'pos'
    $text = $text -replace '\bGetting\b', 'Get'
    $text = $text -replace '\bEntering\b', 'Enter'
    $text = $text -replace '\bExiting\b', 'Exit'
    $text = $text -replace '\bStanding up after\b', 'Stand up after'
    $text = $text -replace '\bSitting on the ground\b', 'Ground sit'
    $text = $text -replace '\bSitting\b', 'Sit'
    $text = $text -replace '\bStanding\b', 'Stand'
    $text = $text -replace '\bWalking\b', 'Walk'
    $text = $text -replace '\bRunning\b', 'Run'
    $text = $text -replace '\bLooking\b', 'Look'
    $text = $text -replace '\bFemale\b', 'Female'
    $text = $text -replace '\bMale\b', 'Male'
    $text = $text -replace '\s+', ' '
    return $text.Trim()
}

function Contains-Any {
    param(
        [string]$Text,
        [string[]]$Needles
    )

    foreach ($needle in $Needles) {
        if ($Text.Contains($needle)) {
            return $true
        }
    }

    return $false
}

function Get-AnimCategory {
    param(
        [string]$Library,
        [string]$AnimName,
        [string]$Description
    )

    $combined = ("{0} {1} {2}" -f $Library, $AnimName, $Description).ToLowerInvariant()

    if (Contains-Any $combined @('sex', 'blowjob', 'dildo', 'vibrator', 'lapdance', 'strip', 'spank', 'snm')) {
        return 'Adult'
    }
    if (Contains-Any $combined @('dead', 'death', 'dying', 'wounded', 'injured', 'falling', 'collapse', 'getting hit', 'burning')) {
        return 'Injury & Death'
    }
    if (Contains-Any $combined @('gun', 'rifle', 'sniper', 'rocket', 'chainsaw', 'knife', 'bat', 'pistol', 'shotgun', 'weapon', 'spray')) {
        return 'Weapons'
    }
    if (Contains-Any $combined @('fight', 'fighting', 'punch', 'kick', 'block', 'uppercut', 'melee')) {
        return 'Fighting'
    }
    if (Contains-Any $combined @('exercise', 'workout', 'benchpress', 'gym', 'weight', 'push-up', 'sit-up')) {
        return 'Exercise'
    }
    if (Contains-Any $combined @('smok', 'cigarette', 'cigar')) {
        return 'Smoking'
    }
    if (Contains-Any $combined @('drink', 'drinking', 'eating', 'burger', 'bottle', 'glass', 'pouring', 'beer', 'food')) {
        return 'Eating & Drinking'
    }
    if (Contains-Any $combined @('dance', 'dancing', 'dj', 'breakdance', 'wop', 'gfunk', 'runningman')) {
        return 'Dancing'
    }
    if (Contains-Any $combined @('gang', 'graffiti', 'riot', 'rap', 'greet the player as gang')) {
        return 'Gang'
    }
    if (Contains-Any $combined @('wave', 'salute', 'point', 'gesture', 'clap', 'laugh', 'cry', 'panic', 'cheer', 'hands up', 'beckon')) {
        return 'Gestures'
    }
    if (Contains-Any $combined @('chat', 'talk', 'conversation', 'greet', 'greeting', 'argue', 'hug', 'kiss', 'phone')) {
        return 'Social'
    }
    if (Contains-Any $combined @('lean', 'leaning')) {
        return 'Leaning'
    }
    if (Contains-Any $combined @('sit', 'sitting', 'seated', 'sunbathing', 'lounging', 'kneeling')) {
        return 'Sitting'
    }
    if (Contains-Any $combined @('walk', 'walking')) {
        return 'Walking'
    }
    if (Contains-Any $combined @('run', 'running', 'jog', 'sprint')) {
        return 'Running'
    }
    if (Contains-Any $combined @('idle', 'still', 'stand', 'stance', 'loop', 'wait')) {
        return 'Idle'
    }
    if (Contains-Any $combined @('wash', 'clean', 'cook', 'carry', 'pickup', 'pick up', 'put down', 'camera', 'wardrobe', 'barber', 'tattoo', 'shop', 'house', 'office')) {
        return 'Indoor & Chores'
    }

    return 'Misc'
}

function Is-PlayableAnimation {
    param(
        [string]$Library,
        [string]$AnimName,
        [string]$Description,
        [string]$Category
    )

    if ($Category -eq 'Adult') {
        return $false
    }

    $blockedIds = @(
        'SHOP:SHP_GUN_AIM'
    )

    $entryId = '{0}:{1}' -f $Library, $AnimName
    if ($blockedIds -contains $entryId) {
        return $false
    }

    $combined = ("{0} {1} {2}" -f $Library, $AnimName, $Description).ToLowerInvariant()

    $blockedLibraries = @(
        'BSKTBALL',
        'BF_INJECTION', 'BIKED', 'BIKEH', 'BIKELEAP', 'BIKES', 'BIKEV', 'BIKE_DBZ',
        'BMX', 'BUS', 'CAMERA', 'CAR', 'CAR_CHAT', 'CHOPPA', 'COACH', 'CLOTHES',
        'DOZER', 'DRIVEBYS', 'KART', 'MTB', 'NEVADA', 'PARACHUTE', 'PLAYER_DVBYS',
        'QUAD', 'QUAD_DBZ', 'RUSTLER', 'SHAMAL', 'SWIM', 'TANK', 'TRAIN', 'TRUCK',
        'VAN', 'VORTEX', 'WAYFARER', 'AIRPORT'
    )

    if ($blockedLibraries -contains $Library) {
        return $false
    }

    $blockedKeywords = @(
        'car door', 'door from the', 'closing a car', 'opening a car', 'roll door', 'rolldoor',
        'hatch', 'trunk', 'hood', 'windshield', 'ladder', 'firetruck', 'climb out', 'climbs out',
        'hanging on windshield', 'thrown onto car', 'tied to a car', 'tied to car', 'vehicle',
        'truck driver', 'van', 'bus', 'coach', 'plane', 'aircraft', 'bike', 'bmx', 'motorcycle',
        'quad', 'bicycle', 'kart', 'boat', 'swim', 'parachute', 'getting into a', 'getting out of a',
        'entering a ', 'exiting a ', 'pulling out a', 'pulling out the', 'driver', 'passenger',
        'safe', 'wardrobe', 'camera', 'tattoo', 'haircut', 'shopping', 'bartender', 'dealer',
        'phone inside a car', 'chatting inside a car', 'relaxing while sitting in car'
    )

    return -not (Contains-Any $combined $blockedKeywords)
}

function Get-Tags {
    param(
        [string]$DisplayName,
        [string]$Category,
        [string]$Library,
        [string]$AnimName
    )

    $allWords = @()
    $allWords += Split-Words $DisplayName
    $allWords += Split-Words $Category
    $allWords += Split-Words $Library
    $allWords += Split-Words $AnimName

    $tags = New-Object System.Collections.Generic.List[string]
    $seen = @{}
    foreach ($word in $allWords) {
        $normalized = $word.ToLowerInvariant()
        if ([string]::IsNullOrWhiteSpace($normalized)) { continue }
        if ($seen.ContainsKey($normalized)) { continue }
        $seen[$normalized] = $true
        [void]$tags.Add($normalized)
    }
    return $tags
}

function Escape-Json {
    param([string]$Value)

    $text = if ($null -eq $Value) { '' } else { [string]$Value }
    $text = $text -replace '\\', '\\\\'
    $text = $text -replace '"', '\"'
    $text = $text -replace "`r", '\r'
    $text = $text -replace "`n", '\n'
    $text = $text -replace "`t", '\t'
    return $text
}

function Write-CatalogJson {
    param(
        [System.Collections.Generic.List[object]]$Entries,
        [string]$Path
    )

    $sb = New-Object System.Text.StringBuilder
    [void]$sb.AppendLine('[')
    for ($i = 0; $i -lt $Entries.Count; ++$i) {
        $entry = $Entries[$i]
        [void]$sb.AppendLine('  {')
        [void]$sb.AppendLine(('    "id": "{0}",' -f (Escape-Json $entry.id)))
        [void]$sb.AppendLine(('    "ifp_file": "{0}",' -f (Escape-Json $entry.ifp_file)))
        [void]$sb.AppendLine(('    "block": "{0}",' -f (Escape-Json $entry.block)))
        [void]$sb.AppendLine(('    "anim_name": "{0}",' -f (Escape-Json $entry.anim_name)))
        [void]$sb.AppendLine(('    "display_name": "{0}",' -f (Escape-Json $entry.display_name)))
        [void]$sb.AppendLine(('    "category": "{0}",' -f (Escape-Json $entry.category)))
        [void]$sb.AppendLine('    "tags": [')
        for ($t = 0; $t -lt $entry.tags.Count; ++$t) {
            $comma = if ($t -lt $entry.tags.Count - 1) { ',' } else { '' }
            [void]$sb.AppendLine(('      "{0}"{1}' -f (Escape-Json $entry.tags[$t]), $comma))
        }
        [void]$sb.AppendLine('    ],')
        [void]$sb.AppendLine(('    "loop_default": {0},' -f ($entry.loop_default.ToString().ToLowerInvariant())))
        [void]$sb.AppendLine(('    "ped_flag": {0},' -f ($entry.ped_flag.ToString().ToLowerInvariant())))
        [void]$sb.AppendLine(('    "lock_f": {0},' -f ($entry.lock_f.ToString().ToLowerInvariant())))
        [void]$sb.AppendLine(('    "pose_flag": {0},' -f ($entry.pose_flag.ToString().ToLowerInvariant())))
        [void]$sb.AppendLine(('    "notes": "{0}"' -f (Escape-Json $entry.notes)))
        $objectEnd = if ($i -lt $Entries.Count - 1) { '  },' } else { '  }' }
        [void]$sb.AppendLine($objectEnd)
    }
    [void]$sb.AppendLine(']')

    $utf8NoBom = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($Path, $sb.ToString(), $utf8NoBom)
}

if (-not (Test-Path $SourcePath)) {
    throw "Source file not found: $SourcePath"
}

$lines = Get-Content -Path $SourcePath -Encoding UTF8
$parsed = New-Object System.Collections.Generic.List[object]
$currentLibrary = $null

foreach ($line in $lines) {
    $lineTrim = $line.Trim()
    if ([string]::IsNullOrWhiteSpace($lineTrim) -or $lineTrim -eq 'local animationsWithDescriptions = {') {
        continue
    }

    if ($null -ne $currentLibrary -and $lineTrim -match '^\}\s*,?\s*$') {
        $currentLibrary = $null
        continue
    }

    if ($lineTrim -eq '}') {
        continue
    }

    if ($null -eq $currentLibrary) {
        $match = [regex]::Match($lineTrim, '^\["(?<library>[^"]+)"\]\s*=\s*\{\s*$')
        if ($match.Success) {
            $currentLibrary = $match.Groups['library'].Value
        }
        continue
    }

    $entryMatch = [regex]::Match($lineTrim, '^\["(?<name>[^"]+)"\]\s*=\s*"(?<desc>(?:\\.|[^"])*)"\s*,?\s*$')
    if (-not $entryMatch.Success) {
        continue
    }

    $animName = $entryMatch.Groups['name'].Value
    $description = $entryMatch.Groups['desc'].Value -replace '\\"', '"' -replace '\\\\', '\'
    if ([string]::IsNullOrWhiteSpace($description)) {
        $description = Humanize-Name $animName
    }

    $libraryUpper = $currentLibrary.ToUpperInvariant()
    $animUpper = $animName.ToUpperInvariant()
    $displayName = Shorten-DisplayName $description
    $category = Get-AnimCategory -Library $libraryUpper -AnimName $animUpper -Description $description
    if (-not (Is-PlayableAnimation -Library $libraryUpper -AnimName $animUpper -Description $description -Category $category)) {
        continue
    }
    $notes = "Source: animlist.lua. Description: $description"
    $combined = ("{0} {1}" -f $animUpper, $description).ToLowerInvariant()
    $loopDefault = (Contains-Any $combined @('loop', 'idle', 'still', 'wait', 'stance', 'holding'))
    $poseFlag = (Contains-Any $combined @('idle', 'still', 'stance', 'pose', 'sit', 'lean', 'stand', 'wait'))
    $tags = Get-Tags -DisplayName $displayName -Category $category -Library $libraryUpper -AnimName $animUpper

    $parsed.Add([pscustomobject]@{
        id = ("{0}:{1}" -f $currentLibrary.ToLowerInvariant(), $animName.ToLowerInvariant())
        ifp_file = $libraryUpper
        block = $libraryUpper
        anim_name = $animUpper
        display_name = $displayName
        category = $category
        tags = $tags
        loop_default = $loopDefault
        ped_flag = ($libraryUpper -eq 'PED')
        lock_f = $false
        pose_flag = $poseFlag
        notes = $notes
    }) | Out-Null
}

$groups = $parsed | Group-Object display_name
foreach ($group in $groups) {
    if ($group.Count -le 1) {
        continue
    }

    $index = 1
    foreach ($item in $group.Group) {
        $item.display_name = "{0} {1}" -f $item.display_name, $index
        $item.tags = Get-Tags -DisplayName $item.display_name -Category $item.category -Library $item.ifp_file -AnimName $item.anim_name
        ++$index
    }
}

$entries = New-Object System.Collections.Generic.List[object]
foreach ($item in ($parsed | Sort-Object category, display_name, ifp_file, anim_name)) {
    [void]$entries.Add($item)
}

$outputDir = Split-Path -Path $OutputPath -Parent
if (-not (Test-Path $outputDir)) {
    New-Item -Path $outputDir -ItemType Directory -Force | Out-Null
}

Write-CatalogJson -Entries $entries -Path $OutputPath
Write-Host ("Generated {0} animation entries from animlist.lua to {1}" -f $entries.Count, $OutputPath)

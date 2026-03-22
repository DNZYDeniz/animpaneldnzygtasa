[CmdletBinding()]
param(
    [string]$SourcePath = "D:\GTASAVIDEOCEKME\modloader\animmm\AnimMenu.sc",
    [string]$OutputPath = "D:\GTASAVIDEOCEKME\modloader\AnimPanel\data\anim-catalog.json"
)

$ErrorActionPreference = "Stop"

function Normalize-Category {
    param([string]$Label)
    switch -Regex ($Label) {
        '^general_' { return 'General' }
        '^sitting_' { return 'Sitting' }
        '^dancing_' { return 'Dancing' }
        '^chatting_' { return 'Chatting' }
        '^gang_' { return 'Gang' }
        '^idle_' { return 'Idle' }
        '^weapon_' { return 'Weapon' }
        '^blowjob_' { return 'Adult' }
        default { return '' }
    }
}

function Make-Tags {
    param([string]$DisplayName, [string]$Category)
    $tags = [System.Collections.Generic.List[string]]::new()
    foreach ($part in ($DisplayName -split '[^A-Za-z0-9]+')) {
        if ([string]::IsNullOrWhiteSpace($part)) { continue }
        $value = $part.ToLowerInvariant()
        if (-not $tags.Contains($value)) {
            $tags.Add($value)
        }
    }
    $categoryTag = $Category.ToLowerInvariant()
    if (-not [string]::IsNullOrWhiteSpace($categoryTag) -and -not $tags.Contains($categoryTag)) {
        $tags.Add($categoryTag)
    }
    return @($tags)
}

$lines = Get-Content -Path $SourcePath
$entries = [System.Collections.Generic.List[object]]::new()
$currentLabel = ''
$currentCategory = ''
$pending = $null

foreach ($rawLine in $lines) {
    $line = $rawLine.Trim()

    if ($line -match '^([a-z0-9_]+):$') {
        $currentLabel = $matches[1]
        $normalized = Normalize-Category $currentLabel
        if (-not [string]::IsNullOrWhiteSpace($normalized)) {
            $currentCategory = $normalized
        }
        continue
    }

    if ($line -match '^IF accepteditem = \d+\s+//\s+(.+)$') {
        $pending = [ordered]@{
            display_name = $matches[1].Trim()
            category = $currentCategory
            ifp_file = ''
            anim_name = ''
            loop_default = $false
            ped_flag = $false
            lock_f = $false
        }
        continue
    }

    if ($null -eq $pending) {
        continue
    }

    if ($line -match '^ifpfile = "([^"]+)"$') {
        $pending.ifp_file = $matches[1]
        continue
    }

    if ($line -match '^animname = "([^"]+)"$') {
        $pending.anim_name = $matches[1]
        continue
    }

    if ($line -match '^loopstatus = (\d+)$') {
        $pending.loop_default = ([int]$matches[1]) -ne 0
        continue
    }

    if ($line -match '^lockf = (\d+)$') {
        $pending.lock_f = ([int]$matches[1]) -ne 0
        continue
    }

    if ($line -eq 'GOTO base_anim_play_ped') {
        $pending.ped_flag = $true
        if ([string]::IsNullOrWhiteSpace($pending.ifp_file)) {
            $pending.ifp_file = 'PED'
        }
    }

    if ($line -eq 'GOTO base_anim_play' -or $line -eq 'GOTO base_anim_play_ped') {
        if (-not [string]::IsNullOrWhiteSpace($pending.anim_name) -and -not [string]::IsNullOrWhiteSpace($pending.ifp_file)) {
            $ifpLower = $pending.ifp_file.ToLowerInvariant()
            $animLower = $pending.anim_name.ToLowerInvariant()
            $category = if ([string]::IsNullOrWhiteSpace($pending.category)) { 'Misc' } else { $pending.category }
            $entry = [ordered]@{
                id = "${ifpLower}:${animLower}"
                ifp_file = $pending.ifp_file
                block = $ifpLower
                anim_name = $pending.anim_name
                display_name = $pending.display_name
                category = $category
                tags = @(Make-Tags -DisplayName $pending.display_name -Category $category)
                loop_default = $pending.loop_default
                ped_flag = $pending.ped_flag
                lock_f = $pending.lock_f
                pose_flag = $pending.loop_default
                notes = "Imported from modloader/animmm/AnimMenu.sc"
            }
            $entries.Add($entry)
        }
        $pending = $null
    }
}

$json = $entries | ConvertTo-Json -Depth 6
[System.IO.File]::WriteAllText($OutputPath, $json, (New-Object System.Text.UTF8Encoding($false)))
Write-Output ("Generated {0} entries to {1}" -f $entries.Count, $OutputPath)

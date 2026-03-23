[CmdletBinding()]
param(
    [string]$SourcePath = "",
    [string]$SourceUrl = "https://raw.githubusercontent.com/openmultiplayer/web/master/frontend/src/data/animations.ts",
    [string]$OverridePath = "D:\GTASAVIDEOCEKME\tools\anim-panel-openmp-overrides.json",
    [string]$OutputPath = "D:\GTASAVIDEOCEKME\AnimPanel\data\anim-catalog.json"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-SourceFilePath {
    param([string]$RequestedPath)

    if (-not [string]::IsNullOrWhiteSpace($RequestedPath) -and (Test-Path $RequestedPath)) {
        return (Resolve-Path $RequestedPath).Path
    }

    $localClonePath = Join-Path $env:TEMP "openmp-web\frontend\src\data\animations.ts"
    if (Test-Path $localClonePath) {
        return $localClonePath
    }

    $downloadPath = Join-Path $env:TEMP "openmp-animations.ts"
    Invoke-WebRequest -Uri $SourceUrl -UseBasicParsing -OutFile $downloadPath
    return $downloadPath
}

function ConvertFrom-TsString {
    param([string]$Value)

    return ConvertFrom-Json ('"' + $Value + '"')
}

function ConvertTo-LowerId {
    param([string]$Library, [string]$Name)

    return ("{0}:{1}" -f $Library.ToLowerInvariant(), $Name.ToLowerInvariant())
}

function Get-OverrideKey {
    param([string]$Library, [string]$Name)

    return ("{0}:{1}" -f $Library.ToUpperInvariant(), $Name.ToUpperInvariant())
}

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
    param([string]$Name)

    $words = @(Split-Words $Name)
    if ($words.Count -eq 0) {
        return $Name
    }

    return (($words | ForEach-Object {
        if ($_ -match '^\d+$') { $_ }
        elseif ($_.Length -le 3 -and $_ -cmatch '^[A-Z0-9]+$') { $_ }
        else { $_.Substring(0, 1).ToUpperInvariant() + $_.Substring(1).ToLowerInvariant() }
    }) -join ' ')
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

function Has-MapKey {
    param(
        $Map,
        [string]$Key
    )

    if ($null -eq $Map) {
        return $false
    }
    if ($Map -is [System.Collections.IDictionary]) {
        return $Map.Contains($Key)
    }
    if ($Map.PSObject) {
        foreach ($property in @($Map.PSObject.Properties)) {
            if ($property -is [System.Management.Automation.PSPropertyInfo] -and $property.Name -eq $Key) {
                return $true
            }
        }
    }
    return $false
}

function Get-DisplayName {
    param(
        [hashtable]$Anim,
        [hashtable]$Overrides
    )

    $key = Get-OverrideKey $Anim.library $Anim.name
    if (Has-MapKey $Overrides.displayNameOverrides $key) {
        return [string]$Overrides.displayNameOverrides[$key]
    }

    $description = [string]$Anim.description
    if (-not [string]::IsNullOrWhiteSpace($description) -and
        $description -ne "[missing animation]" -and
        $description.Length -le 44 -and
        $description -notmatch "^One of .+ idle animations$") {
        return $description.Trim().TrimEnd('.')
    }

    return Humanize-Name $Anim.name
}

function Shorten-DisplayName {
    param([string]$Value)

    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $Value
    }

    $text = $Value.Trim().TrimEnd('.')
    $text = $text -replace '\bidle animations\b', 'idle'
    $text = $text -replace '\banimation loop\b', 'loop'
    $text = $text -replace '\banimation\b', ''
    $text = $text -replace '\banimations\b', ''
    $text = $text -replace '\bAlternative\b', 'Alt'
    $text = $text -replace '\bfemale\b', 'Female'
    $text = $text -replace '\bmale\b', 'Male'
    $text = $text -replace '\bFemale park sitting idle\b', 'Female park sit idle'
    $text = $text -replace '\bMale park sitting idle\b', 'Male park sit idle'
    $text = $text -replace '\bMale sitting idle\b', 'Male sit idle'
    $text = $text -replace '\bFemale sitting idle\b', 'Female sit idle'
    $text = $text -replace '\bSitting inside a car\b', 'Car sit'
    $text = $text -replace '\bDance animation\b', 'Dance'
    $text = $text -replace '\bDancing loop\b', 'Dance loop'
    $text = $text -replace '\bGang sign\b', 'Gang sign'
    $text = $text -replace '\bGang handshake\b', 'Gang handshake'
    $text = $text -replace '\bStripper dance\b', 'Strip dance'
    $text = $text -replace '\bStripper lapdance\b', 'Lapdance'
    $text = $text -replace '\bDJ animation left\b', 'DJ left'
    $text = $text -replace '\bDJ animation right\b', 'DJ right'
    $text = $text -replace '\bFace talk animation\b', 'Face talk'
    $text = $text -replace '\bCrackhead dying animation\b', 'Crackhead death'
    $text = $text -replace '\bBasketball dunking animation\b', 'Basketball dunk'
    $text = $text -replace '\bPunch animation\b', 'Punch'
    $text = $text -replace '\bWindow panic animation\b', 'Window panic'
    $text = $text -replace '\bRapping animation loop\b', 'Rap loop'
    $text = $text -replace '\bLooking\b', 'Look'
    $text = $text -replace '\bWalking\b', 'Walk'
    $text = $text -replace '\bRunning\b', 'Run'
    $text = $text -replace '\bSitting\b', 'Sit'
    $text = $text -replace '\bStanding\b', 'Stand'
    $text = $text -replace '\bGetting\b', 'Get'
    $text = $text -replace '\bposition\b', 'pos'
    $text = $text -replace '\s+', ' '
    return $text.Trim()
}

function Is-PlayableAnimation {
    param(
        [hashtable]$Anim,
        [hashtable]$Overrides
    )

    $library = $Anim.library.ToUpperInvariant()
    $name = $Anim.name.ToUpperInvariant()
    $key = Get-OverrideKey $library $name

    if ($Overrides.includeIds -contains $key) {
        return $true
    }

    if ($library -eq "BSKTBALL") {
        return $false
    }

    if ($Overrides.excludeLibraries -contains $library -or $Overrides.excludeIds -contains $key) {
        return $false
    }

    $combined = ("{0} {1} {2} {3}" -f $library, $name, $Anim.description, $Anim.notes).ToLowerInvariant()
    if ($combined.Contains("[missing animation]") -or $combined.Contains("missing animation")) {
        return $false
    }

    $blockedKeywords = @(
        "vehicle", "driving", "driveby", "drive-by", "while driving", "while riding",
        "bike", "motorcycle", "bicycle", "quad", "kart", "train", "helicopter",
        "plane", "aircraft", "swimming", "swim", "parachute", "boat", "vortex",
        "entering vehicle", "exiting vehicle", "boarding aircraft", "pilot",
        "two-person", "two person", "paired", "partner", "with another person",
        "steering left", "steering right", "steering boat", "steering right with", "steering left with",
        "reversing with a truck", "truck driving animation", "truck driver", "driver ", " driver",
        "passanger", "passenger", "lowrider", "windshield", "go-kart", "riding", "ride ",
        "boat driving", "truck ", " truck", "shuffle to the driver's", "shuffling to the driver's",
        "pulling out the driver", "sitting in a lowrider", "driver switches", "switches with driver",
        "kissing", "blowjob", "basketball", "dribbling", "dunking", "shot while jumping",
        "bartender", "taking order", "serving", "cashier", "shop clerk", "shopkeeper",
        "vending machine", "customer", "dealer dealing", "dealer idle", "drug dealer",
        "tattoo order", "tattoo artist", "haircut", "hairdresser", "barber", "buying haircut",
        "camera operator", "taking photo", "office worker", "through a window", "tapping hand on window",
        "opening a door", "closing a door", "car door", "doorlocked", "doorlocked", "trunk",
        "searching through trunk", "arresting someone", "thrown onto car by officer",
        "medic performing cpr", "cpr", "girlfriend kiss", "player kiss animation",
        "holding onto someone's shoulder", "hanging on", "stunned while hanging on", "climb onto",
        "pulling another", "with another", "with someone",
        "lying to sitting position", "lieb2sit",
        "start of ", "end of ", "lay down to ", "get up after ", "getting up from ",
        "standing up after", "sitting to standing", "sitting to standing up", "lying to ",
        "walking and sitting", "correcting position while sitting", "getting off the chair",
        "getting off a ", "getting off the "
    )

    if ($library -eq "PED" -and $name.StartsWith("DRIVE_")) {
        return $false
    }

    if (($library -eq "PED" -and ($name.StartsWith("CAR_") -or $name.StartsWith("DOOR_"))) -or
        ($library -eq "RYDER" -and $name.StartsWith("VAN_")) -or
        ($library -eq "ROB_BANK" -and $name.StartsWith("CAT_SAFE")) -or
        ($library -eq "GHETTO_DB" -and $name.StartsWith("GDB_CAR_")) -or
        ($library -eq "POLICE" -and $name.Contains("GETOUTCAR"))) {
        return $false
    }

    if ($name -match '(^|_)(IN|OUT)(_|$)' -or
        $name -match 'LIEB2SIT|B2SIT|SIT2|GETUP|STANDUP') {
        return $false
    }

    return -not (Contains-Any $combined $blockedKeywords)
}

function Get-AnimCategory {
    param(
        [hashtable]$Anim,
        [hashtable]$Overrides
    )

    $library = $Anim.library.ToUpperInvariant()
    $name = $Anim.name.ToUpperInvariant()
    $key = Get-OverrideKey $library $name
    if (Has-MapKey $Overrides.categoryOverrides $key) {
        return [string]$Overrides.categoryOverrides[$key]
    }

    $combined = ("{0} {1} {2} {3}" -f $library, $name, $Anim.description, $Anim.notes).ToLowerInvariant()

    if ($library -in @("DILDO", "STRIP") -or (Contains-Any $combined @("strip", "lapdance", "sexual", "sex toy", "dildo", "vibrator", "pole"))) {
        return "Adult"
    }
    if (Contains-Any $combined @("dead", "death", "dying", "collapse", "injured", "injury", "wounded", "getting hit", "fall over", "falling", "burning", "panic death")) {
        return "Injury & Death"
    }
    if ($library -in @("BASEBALL", "CHAINSAW", "COLT45", "FLAME", "GRENADE", "KNIFE", "PYTHON", "RIFLE", "ROCKET", "SHOTGUN", "SILENCED", "SNIPER", "SPRAYCAN", "SWORD", "TEC", "UZI", "WEAPONS") -or
        (Contains-Any $combined @("weapon", "gun", "pistol", "rifle", "sniper", "rocket", "grenade", "bat swing", "chainsaw", "knife", "spraycan", "sword"))) {
        return "Weapons"
    }
    if ($library -in @("BOX", "FIGHT_B", "FIGHT_C", "FIGHT_D", "FIGHT_E") -or
        (Contains-Any $combined @("fight", "fighting", "punch", "kick", "block", "uppercut", "melee"))) {
        return "Fighting"
    }
    if ($library -in @("BENCHPRESS", "FREEWEIGHTS", "GYMNASIUM", "MUSCULAR") -or
        (Contains-Any $combined @("exercise", "workout", "weight", "bench press", "push-up", "sit-up", "gym"))) {
        return "Exercise"
    }
    if ($library -eq "SMOKING" -or (Contains-Any $combined @("smoking", "smoke", "cigarette", "cigar"))) {
        return "Smoking"
    }
    if ($library -in @("BAR", "FOOD", "VENDING") -or (Contains-Any $combined @("drink", "drinking", "eating", "burger", "beer", "bottle", "glass", "pouring", "soda"))) {
        return "Eating & Drinking"
    }
    if (Contains-Any $combined @("sit", "sitting", "seated", "sitdown", "sunbathe", "lounging", "lying down")) {
        return "Sitting"
    }
    if (Contains-Any $combined @("lean", "leaning")) {
        return "Leaning"
    }
    if (Contains-Any $combined @("walk", "walking")) {
        return "Walking"
    }
    if (Contains-Any $combined @("run", "running", "sprint", "jog")) {
        return "Running"
    }
    if ($library -in @("DANCING", "GFUNK", "LOWRIDER", "RUNNINGMAN", "WOP") -or (Contains-Any $combined @("dance", "dancing"))) {
        return "Dancing"
    }
    if ($library -in @("GANGS", "GHANDS", "GHETTO_DB", "GRAFFITI", "RAPPING", "RIOT") -or (Contains-Any $combined @("gang", "ghetto", "graffiti", "rap", "riot"))) {
        return "Gang"
    }
    if (Contains-Any $combined @("chat", "talk", "conversation", "greet", "greeting", "argue", "flirt", "hug", "party")) {
        return "Social"
    }
    if (Contains-Any $combined @("wave", "salute", "point", "gesture", "clap", "laugh", "cry", "panic", "cheer", "hands up", "beckon", "scratch")) {
        return "Gestures"
    }
    if ($library -in @("AIRPORT", "ATTRACTORS", "BAR", "CAMERA", "CARRY", "CLOTHES", "HAIRCUTS", "INT_HOUSE", "INT_OFFICE", "INT_SHOP", "MEDIC", "SHOP", "TATTOOS", "VENDING") -or
        (Contains-Any $combined @("door", "opening", "closing", "bartender", "carry", "carrying", "pickup", "shop", "shopping", "tattoo", "haircut", "camera", "office", "house", "clean", "wash", "cook"))) {
        return "Indoor & Chores"
    }
    if (Contains-Any $combined @("idle", "idling", "waiting", "wait", "stance", "standing", "ambient", "looking around")) {
        return "Idle"
    }

    return "Misc"
}

function Get-LoopDefault {
    param(
        [hashtable]$Anim,
        [string]$Category,
        [hashtable]$Overrides
    )

    $key = Get-OverrideKey $Anim.library $Anim.name
    if (Has-MapKey $Overrides.loopDefaultOverrides $key) {
        return [bool]$Overrides.loopDefaultOverrides[$key]
    }

    $combined = ("{0} {1} {2}" -f $Anim.name, $Anim.description, $Anim.notes).ToLowerInvariant()
    if (Contains-Any $combined @("loop", "idle", "idling", "waiting", "stance", "walk", "walking", "run", "running", "lean", "leaning", "sunbathe")) {
        return $true
    }
    if ($Category -in @("Idle", "Sitting", "Leaning", "Smoking")) {
        return $true
    }
    return $false
}

function Make-Tags {
    param(
        [hashtable]$Anim,
        [string]$Category,
        [string]$DisplayName
    )

    $tags = [System.Collections.Generic.HashSet[string]]::new()
    foreach ($value in @($DisplayName, $Anim.description, $Anim.notes, $Anim.library, $Anim.name, $Category)) {
        foreach ($part in (Split-Words $value)) {
            $tag = $part.Trim().ToLowerInvariant()
            if (-not [string]::IsNullOrWhiteSpace($tag)) {
                [void]$tags.Add($tag)
            }
        }
    }

    return @($tags | Sort-Object)
}

function ConvertTo-PlainValue {
    param($Value)

    if ($null -eq $Value) {
        return $null
    }

    if ($Value -is [System.Collections.IDictionary]) {
        $table = @{}
        foreach ($key in $Value.Keys) {
            $table[$key.ToString()] = ConvertTo-PlainValue $Value[$key]
        }
        return $table
    }

    if ($Value -is [System.Collections.IEnumerable] -and -not ($Value -is [string])) {
        $items = New-Object System.Collections.ArrayList
        foreach ($item in $Value) {
            [void]$items.Add((ConvertTo-PlainValue $item))
        }
        return @($items)
    }

    $properties = @()
    if ($Value.PSObject) {
        $properties = @($Value.PSObject.Properties)
    }
    if ($properties.Count -gt 0 -and -not ($Value -is [string])) {
        $table = @{}
        foreach ($property in $properties) {
            $table[$property.Name] = ConvertTo-PlainValue $property.Value
        }
        return $table
    }

    return $Value
}

function Load-Overrides {
    param([string]$Path)

    $base = @{
        excludeLibraries = @()
        excludeIds = @()
        includeIds = @()
        categoryOverrides = @{}
        displayNameOverrides = @{}
        loopDefaultOverrides = @{}
    }

    if (-not (Test-Path $Path)) {
        return $base
    }

    $raw = ConvertTo-PlainValue (Get-Content $Path -Raw | ConvertFrom-Json)
    foreach ($key in $base.Keys) {
        $hasKey = $false
        if ($raw -is [System.Collections.IDictionary]) {
            $hasKey = $raw.Contains($key)
        } elseif ($raw.PSObject) {
            $hasKey = @($raw.PSObject.Properties.Name) -contains $key
        }
        if (-not $hasKey) {
            $raw[$key] = $base[$key]
        }
    }

    $raw.excludeLibraries = @($raw.excludeLibraries | ForEach-Object { $_.ToString().ToUpperInvariant() })
    $raw.excludeIds = @($raw.excludeIds | ForEach-Object { $_.ToString().ToUpperInvariant() })
    $raw.includeIds = @($raw.includeIds | ForEach-Object { $_.ToString().ToUpperInvariant() })
    return $raw
}

function Parse-AnimationSource {
    param([string]$Path)

    $lines = Get-Content -Path $Path
    $entries = [System.Collections.Generic.List[hashtable]]::new()
    $inArray = $false
    $current = $null

    foreach ($rawLine in $lines) {
        $line = $rawLine.Trim()

        if (-not $inArray) {
            if ($line -eq "export const animations: Animation[] = [") {
                $inArray = $true
            }
            continue
        }

        if ($line -eq "];") {
            break
        }

        if ($line -eq "{") {
            $current = @{
                index = 0
                library = ""
                name = ""
                frames = 0
                duration = 0.0
                description = ""
                notes = ""
            }
            continue
        }

        if ($null -eq $current) {
            continue
        }

        if ($line -match '^index:\s*(\d+),$') {
            $current.index = [int]$matches[1]
            continue
        }
        if ($line -match '^library:\s*"((?:[^"\\]|\\.)*)",$') {
            $current.library = ConvertFrom-TsString $matches[1]
            continue
        }
        if ($line -match '^name:\s*"((?:[^"\\]|\\.)*)",$') {
            $current.name = ConvertFrom-TsString $matches[1]
            continue
        }
        if ($line -match '^frames:\s*(\d+),$') {
            $current.frames = [int]$matches[1]
            continue
        }
        if ($line -match '^duration:\s*([0-9.]+),$') {
            $current.duration = [double]$matches[1]
            continue
        }
        if ($line -match '^description:\s*"((?:[^"\\]|\\.)*)",$') {
            $current.description = ConvertFrom-TsString $matches[1]
            continue
        }
        if ($line -match '^notes:\s*"((?:[^"\\]|\\.)*)",$') {
            $current.notes = ConvertFrom-TsString $matches[1]
            continue
        }
        if ($line -eq "}," -or $line -eq "}") {
            if ($current.ContainsKey("library") -and $current.ContainsKey("name")) {
                $entries.Add($current)
            }
            $current = $null
        }
    }

    return $entries
}

function Expand-JsonUnicodeEscapes {
    param([string]$Json)

    return [regex]::Replace(
        $Json,
        '\\u([0-9a-fA-F]{4})',
        {
            param($match)
            return [char]([Convert]::ToInt32($match.Groups[1].Value, 16))
        }
    )
}

$sourceFile = Get-SourceFilePath -RequestedPath $SourcePath
$sourceEntries = Parse-AnimationSource -Path $sourceFile
$overrides = Load-Overrides -Path $OverridePath
$catalog = [System.Collections.Generic.List[object]]::new()

foreach ($anim in $sourceEntries) {
    if (-not (Is-PlayableAnimation -Anim $anim -Overrides $overrides)) {
        continue
    }

    $library = $anim.library.ToUpperInvariant()
    $category = Get-AnimCategory -Anim $anim -Overrides $overrides
    $displayName = Get-DisplayName -Anim $anim -Overrides $overrides
    $displayName = Shorten-DisplayName -Value $displayName
    $loopDefault = Get-LoopDefault -Anim $anim -Category $category -Overrides $overrides
    $pedFlag = $library -eq "PED"
    $combinedNotes = "Source: open.mp animation viewer. Index: $($anim.index). Description: $($anim.description)"
    if (-not [string]::IsNullOrWhiteSpace($anim.notes)) {
        $combinedNotes += " Notes: $($anim.notes)"
    }

    $catalog.Add([ordered]@{
        id = ConvertTo-LowerId -Library $library -Name $anim.name
        ifp_file = $library
        block = $library
        anim_name = $anim.name.ToUpperInvariant()
        display_name = $displayName
        category = $category
        tags = @(Make-Tags -Anim $anim -Category $category -DisplayName $displayName)
        loop_default = $loopDefault
        ped_flag = $pedFlag
        lock_f = $false
        pose_flag = $loopDefault
        notes = $combinedNotes
    })
}

$catalog = $catalog |
    Sort-Object `
        @{ Expression = { $_.category } }, `
        @{ Expression = { $_.display_name } }, `
        @{ Expression = { $_.anim_name } }

$duplicateGroups = $catalog | Group-Object { $_.display_name.ToLowerInvariant() } | Where-Object { $_.Count -gt 1 }
foreach ($group in $duplicateGroups) {
    $ordered = @($group.Group | Sort-Object anim_name, ifp_file, id)
    for ($i = 0; $i -lt $ordered.Count; $i++) {
        $ordered[$i].display_name = "{0} {1}" -f $ordered[$i].display_name, ($i + 1)
    }
}

$outDir = Split-Path -Parent $OutputPath
if (-not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir -Force | Out-Null
}

$json = $catalog | ConvertTo-Json -Depth 6
$json = Expand-JsonUnicodeEscapes -Json $json
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($OutputPath, $json, $utf8NoBom)

Write-Output ("Generated {0} filtered animation entries from {1} into {2}" -f $catalog.Count, $sourceFile, $OutputPath)

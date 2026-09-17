[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$projectPath = Split-Path -Parent $PSScriptRoot
$headerPath = Join-Path $projectPath 'skee64\SKEEHooks.h'
$inventoryPath = Join-Path $projectPath 'docs\vr2\hooks.json'
$inventory = Get-Content -Raw -LiteralPath $inventoryPath | ConvertFrom-Json

if ($inventory.schemaVersion -ne 1) {
    throw "Unsupported hook inventory schema version: $($inventory.schemaVersion)"
}

$headerIds = @{}
$idPattern = 'inline constexpr std::uint32_t (kID_[A-Za-z0-9_]+)\s+=\s+([0-9]+)'
foreach ($match in [regex]::Matches((Get-Content -Raw -LiteralPath $headerPath), $idPattern)) {
    $headerIds[$match.Groups[1].Value] = [uint32]$match.Groups[2].Value
}

$inventoryIds = @{}
foreach ($entry in $inventory.relocations) {
    if ($inventoryIds.ContainsKey($entry.symbol)) {
        throw "Duplicate relocation inventory entry: $($entry.symbol)"
    }
    $inventoryIds[$entry.symbol] = [uint32]$entry.aeId

    if ($entry.status -eq 'unqualified' -and $null -ne $entry.vrId) {
        throw "Unqualified relocation has a VR ID: $($entry.symbol)"
    }
    if (-not $entry.sourceSites -or $entry.sourceSites.Count -eq 0) {
        throw "Relocation has no source site: $($entry.symbol)"
    }
}

$missing = @($headerIds.Keys | Where-Object { -not $inventoryIds.ContainsKey($_) } | Sort-Object)
$extra = @($inventoryIds.Keys | Where-Object { -not $headerIds.ContainsKey($_) } | Sort-Object)
if ($missing.Count -or $extra.Count) {
    throw "Relocation inventory mismatch. Missing: $($missing -join ', '); Extra: $($extra -join ', ')"
}

foreach ($symbol in $headerIds.Keys) {
    if ($headerIds[$symbol] -ne $inventoryIds[$symbol]) {
        throw "AE ID mismatch for ${symbol}: header=$($headerIds[$symbol]), inventory=$($inventoryIds[$symbol])"
    }
}

$groups = @{}
foreach ($group in $inventory.groups) {
    $groups[$group.name] = $true
}
foreach ($site in $inventory.patchSites) {
    if (-not $groups.ContainsKey($site.group)) {
        throw "Patch site '$($site.name)' uses unknown group '$($site.group)'."
    }
    if ($null -ne $site.relocationSymbol -and -not $inventoryIds.ContainsKey($site.relocationSymbol)) {
        throw "Patch site '$($site.name)' uses unknown relocation '$($site.relocationSymbol)'."
    }
    if ([int]$site.overwriteLength -le 0) {
        throw "Patch site '$($site.name)' has an invalid overwrite length."
    }
}

[pscustomobject]@{
    schemaVersion = $inventory.schemaVersion
    relocationCount = $inventoryIds.Count
    patchSiteCount = $inventory.patchSites.Count
    groupCount = $groups.Count
    vrQualifiedRelocations = @($inventory.relocations | Where-Object { $null -ne $_.vrId }).Count
    status = 'valid'
} | ConvertTo-Json

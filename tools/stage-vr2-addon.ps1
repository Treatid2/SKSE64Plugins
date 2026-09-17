# SPDX-License-Identifier: GPL-3.0-or-later
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BaselineRoot,
    [Parameter(Mandatory)][string]$StageRoot,
    [string]$ExpectedPackageVersion = '0.1.53'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$baseline = [IO.Path]::GetFullPath($BaselineRoot)
$stage = [IO.Path]::GetFullPath($StageRoot)
if (Test-Path -LiteralPath $stage) { throw 'StageRoot must not exist. Nothing is overwritten.' }
if ($stage.StartsWith($baseline.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'StageRoot must be outside BaselineRoot.' }
$receipt = Get-Content -Raw -LiteralPath (Join-Path $baseline 'build-receipt.json') | ConvertFrom-Json
if ($receipt.gameRuntime -ne 'VR' -or $receipt.packageVersion -ne $ExpectedPackageVersion) { throw 'Unexpected baseline runtime/version.' }
function Copy-VerifiedRuntime([string]$Relative) {
    $entry = @($receipt.runtimeManifest | Where-Object path -eq $Relative)
    if ($entry.Count -ne 1) { throw "Missing/ambiguous retained runtime manifest: $Relative" }
    $inputPath = Join-Path $baseline $Relative
    $item = Get-Item -LiteralPath $inputPath
    if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'Reparse-point input prohibited.' }
    if ((Get-FileHash -LiteralPath $inputPath -Algorithm SHA256).Hash -ne $entry[0].sha256 -or $item.Length -ne $entry[0].bytes) { throw "Runtime manifest mismatch: $Relative" }
    $destination = Join-Path $stage $Relative
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
    Copy-Item -LiteralPath $inputPath -Destination $destination
}
# No wildcard import of Data: no BSA/ESP/SWF/original INI.
Copy-VerifiedRuntime 'Data/SKSE/Plugins/skee64.dll'
foreach ($module in @('CharGen','NiOverride')) {
    $sourceRoot = Join-Path $projectRoot "skee64/Shaders/$module"
    foreach ($shader in @(Get-ChildItem -LiteralPath $sourceRoot -File -Recurse)) {
        if ($shader.Extension -notin @('.hlsl','.fx')) { continue }
        $relative = $shader.FullName.Substring($sourceRoot.Length + 1).Replace('\','/')
        $runtimePath = "Data/SKSE/Plugins/$module/Shaders/$relative"
        if ((Get-FileHash -LiteralPath $shader.FullName).Hash -ne (Get-FileHash -LiteralPath (Join-Path $baseline $runtimePath)).Hash) { throw "Shader source differs from baseline: $relative" }
        Copy-VerifiedRuntime $runtimePath
    }
    foreach ($entry in @($receipt.runtimeManifest | Where-Object { $_.path -match "^Data/SKSE/Plugins/$module/Shaders/Compiled/[A-Za-z0-9_./-]+\.cso$" })) {
        if ($entry.path.Contains('..')) { throw 'Unsafe compiled shader path.' }
        Copy-VerifiedRuntime $entry.path
    }
}
$customIni = '; RaceMenu VR 2 overrides only. Merge existing custom INI settings.' + "`r`n" +
    (Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'packaging/menu-appearance.ini.inc')) + "`r`n" +
    (Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'packaging/menu-profiles.ini.inc'))
[IO.File]::WriteAllText((Join-Path $stage 'Data/SKSE/Plugins/skee64_custom.ini'), $customIni, [Text.UTF8Encoding]::new($false))
$resourceDir = Join-Path $stage 'Data/ModderResource'
New-Item -ItemType Directory -Force -Path $resourceDir | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'skee64/IPluginInterface.h') -Destination $resourceDir
$docs = Join-Path $stage 'Data/docs/RaceMenuVR2'
$recipe = Join-Path $docs 'AssetPatcher/tools/vr-racesex-patches'
New-Item -ItemType Directory -Force -Path $recipe | Out-Null
foreach ($file in @('LICENSE','THIRD_PARTY_NOTICES.md','docs/release/INSTALLATION.md','docs/release/RELEASE-CHECKLIST.md','docs/menu-customization.md','docs/menu-appearance.md')) {
    Copy-Item -LiteralPath (Join-Path $projectRoot $file) -Destination $docs
}
Copy-Item -LiteralPath (Join-Path $projectRoot 'docs/release/DOWNLOAD-README.md') -Destination (Join-Path $docs 'README.md')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'patch-vr-racesex-swf.ps1') -Destination (Split-Path -Parent $recipe)
foreach ($file in @('Appearance.as.inc','TextEntry.as.inc','InputTrace.as.inc')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "vr-racesex-patches/$file") -Destination $recipe
}
$files = @(Get-ChildItem -LiteralPath (Join-Path $stage 'Data') -File -Recurse)
if (@($files | Where-Object { $_.Extension -in @('.bsa','.esp','.esm','.swf','.png') -or $_.Name -in @('skeevr.dll','RaceMenuPrismaBridge.dll','skee64.ini','SearchWidget.as') }).Count) { throw 'Forbidden upstream/private asset in add-on stage.' }
$manifest = @($files | Sort-Object FullName | ForEach-Object {
    [ordered]@{ path = $_.FullName.Substring($stage.Length + 1).Replace('\','/'); bytes = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName).Hash }
})
[ordered]@{ schema = 1; packageVersion = $ExpectedPackageVersion; nativeVersion = $receipt.nativePluginVersion;
    status = 'prepared-native-addon-requires-local-menu-not-public-release'; builtFromDirtyWorktree = ($receipt.source.workingTreeStatus.Count -gt 0);
    originalAssetsIncluded = $false; localMenuRequired = $true; files = $manifest
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $stage 'addon-receipt.json') -Encoding utf8
Write-Output "Verified asset-free add-on stage: $stage"

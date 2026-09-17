[CmdletBinding()]
param(
    [ValidateSet('AE', 'VR')]
    [string]$GameRuntime = 'VR',

    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',

    [ValidateSet('dynamic', 'static')]
    [string]$Crt = 'dynamic',

    [string]$VcvarsPath,

    [string]$ScratchReuseKey,

    [string]$PromoteDirectory,

    [string]$PromoteBaselineDirectory,

    [string]$BaselineAssetSource,

    [string]$BaselineMovieSource,

    [string]$FfdecCli,

    [string]$BaselineAssetVersion = '0.4.20.0',

    [string]$BaselinePackageVersion = '0.1.0',

    [string]$NativePluginVersion = '0.5.0.14',

    [switch]$Full,

    [switch]$AcknowledgeLowSpace
)

$ErrorActionPreference = 'Stop'

if ($PSVersionTable.PSVersion.Major -lt 7) {
    throw 'tools/vr2-build.ps1 requires PowerShell 7 (pwsh) for the managed scratch contract.'
}

$projectPath = Split-Path -Parent $PSScriptRoot
$scratchTool = $env:CODEX_SCRATCH_TOOL
$scratchRoot = $env:CODEX_FAST_SCRATCH_ROOT

if (-not $scratchTool -or -not (Test-Path -LiteralPath $scratchTool -PathType Leaf)) {
    throw 'CODEX_SCRATCH_TOOL does not identify the managed scratch tool.'
}
if (-not $scratchRoot -or -not (Test-Path -LiteralPath $scratchRoot -PathType Container)) {
    throw 'CODEX_FAST_SCRATCH_ROOT does not identify the managed scratch root.'
}
if ($GameRuntime -eq 'VR' -and $Crt -eq 'static') {
    throw 'The safe-load VR milestone currently supports the dynamic CRT preset only.'
}
if ($PromoteBaselineDirectory -and -not $BaselineAssetSource) {
    throw 'BaselineAssetSource is required when PromoteBaselineDirectory is supplied.'
}
if ($PromoteBaselineDirectory -and $GameRuntime -ne 'VR') {
    throw 'The self-contained RaceMenu NG baseline package is currently defined only for Skyrim VR.'
}
if ($PromoteBaselineDirectory -and (-not $BaselineMovieSource -or -not $FfdecCli)) {
    throw 'Baseline promotion requires BaselineMovieSource and FfdecCli to rebuild current ActionScript; precompiled packaging movies are not accepted.'
}
if ($VcvarsPath) {
    $resolvedVcvarsPath = [IO.Path]::GetFullPath($VcvarsPath)
    if (-not (Test-Path -LiteralPath $resolvedVcvarsPath -PathType Leaf)) {
        throw "Visual Studio environment script not found: $resolvedVcvarsPath"
    }
    $env:SKEE_VCVARS = $resolvedVcvarsPath
}

$reuseKey = if ($ScratchReuseKey) {
    $ScratchReuseKey
} else {
    "racemenu-vr2-$($GameRuntime.ToLowerInvariant())-$($Configuration.ToLowerInvariant())-$Crt"
}
$acquireParams = @{
    Kind = 'build'
    ProjectPath = $projectPath
    ExpectedGiB = 20
    ReuseKey = $reuseKey
    Compact = $true
}
if ($AcknowledgeLowSpace) {
    $acquireParams.AcknowledgeLowSpace = $true
    $acquireParams.LowSpaceReason = 'Reconstructible CMake/vcpkg build; release is immediate and current free space was reviewed before this bounded run.'
}
$acquireJson = & $scratchTool acquire @acquireParams
$acquisition = try { $acquireJson | ConvertFrom-Json } catch { $null }
if (-not $acquisition.ok -or $acquisition.state -ne 'active') {
    throw "Managed scratch acquisition did not become active: $acquireJson"
}

$allocationId = $acquisition.data.id
$workPath = [IO.Path]::GetFullPath($acquisition.data.workPath)
$resolvedScratchRoot = [IO.Path]::GetFullPath($scratchRoot).TrimEnd('\') + '\'
if (-not $workPath.StartsWith($resolvedScratchRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Managed work path is outside the configured scratch root: $workPath"
}

$buildSucceeded = $false
try {
    $commonLibSource = Join-Path $projectPath 'CommonLibSSE-NG'
    $commonLibStage = Join-Path $workPath 'source\CommonLibSSE-NG'
    $buildRoot = Join-Path $workPath 'build'
    $toolchainRoot = Join-Path $workPath 'toolchain'
    $latentPatch = Join-Path $projectPath 'evidence\commonlibsse-ng-latent-vr.patch'

    if ($PromoteBaselineDirectory) {
        $baselineMovie = Join-Path $workPath 'movie\RaceSex_menu.swf'
        & (Join-Path $PSScriptRoot 'patch-vr-racesex-swf.ps1') -InputSwf $BaselineMovieSource -OutputSwf $baselineMovie -FfdecCli $FfdecCli -WorkingDirectory (Join-Path $workPath 'movie\compile')
        if (-not (Test-Path -LiteralPath $baselineMovie -PathType Leaf)) {
            throw 'Current ActionScript compilation did not produce the baseline movie.'
        }
    }

    New-Item -ItemType Directory -Force -Path $commonLibStage, $buildRoot, $toolchainRoot | Out-Null
    # Export the pinned commit, never an unrecorded dirty dependency worktree.
    $commonLibArchive = Join-Path $workPath 'commonlib-source.tar'
    # Exclude repository-only symlinked agent metadata, which Windows tar cannot
    # materialise without symlink privileges. All build sources/notices remain.
    & git -C $commonLibSource archive --format=tar "--output=$commonLibArchive" HEAD CMakeLists.txt CMakePresets.json CommonLibSSE.natvis COPYING.txt EXCEPTIONS.md README.md cmake include src extern res licenses tests vcpkg.json vcpkg-configuration.json
    if ($LASTEXITCODE -ne 0) { throw 'CommonLib pinned source export failed.' }
    & tar.exe -xf $commonLibArchive -C $commonLibStage
    if ($LASTEXITCODE -ne 0) { throw 'CommonLib pinned source extraction failed.' }

    $stagedGitPointer = Join-Path $commonLibStage '.git'
    if (Test-Path -LiteralPath $stagedGitPointer -PathType Leaf) {
        Remove-Item -LiteralPath $stagedGitPointer -Force
    } elseif (Test-Path -LiteralPath $stagedGitPointer) {
        throw "Unexpected .git directory in managed dependency staging: $stagedGitPointer"
    }

    Push-Location $commonLibStage
    try {
        & git apply --check $latentPatch
        if ($LASTEXITCODE -ne 0) {
            throw 'The pinned CommonLibSSE-NG latent-function correction no longer applies cleanly.'
        }
        & git apply $latentPatch
        if ($LASTEXITCODE -ne 0) {
            throw 'Failed to apply the CommonLibSSE-NG latent-function correction to staged source.'
        }
        $runtimePatch = Join-Path $projectPath 'evidence\commonlibsse-ng-racesex-vr.patch'
        & git apply --check $runtimePatch
        if ($LASTEXITCODE -ne 0) { throw 'CommonLib VR RaceSexMenu layout correction no longer applies.' }
        & git apply $runtimePatch
        if ($LASTEXITCODE -ne 0) { throw 'CommonLib VR RaceSexMenu layout correction failed.' }
    } finally {
        Pop-Location
    }

    $env:SKEE_BUILD_ROOT = $buildRoot
    $env:SKEE_TOOLCHAIN_ROOT = $toolchainRoot
    $env:SKEE_COMMONLIB_SOURCE_DIR = $commonLibStage
    $env:SKEE_GAME_RUNTIME = $GameRuntime
    $env:SKEE_VR2_PACKAGE_VERSION = $BaselinePackageVersion
    $env:SKEE_NATIVE_PLUGIN_VERSION = $NativePluginVersion
    $env:SKEE_BUILD_PRISMA_BRIDGE = if ($PromoteDirectory) { '1' } else { '0' }

    $buildArgs = @($Configuration.ToLowerInvariant(), "--$Crt", "--$($GameRuntime.ToLowerInvariant())")
    if ($Full) {
        $buildArgs += '--full'
    }

    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $projectPath 'build.ps1') @buildArgs
    if ($LASTEXITCODE -ne 0) {
        throw "RaceMenu build failed with exit code $LASTEXITCODE."
    }

    $runtimeTag = if ($Crt -eq 'static') { "static-$($GameRuntime.ToLowerInvariant())" } else { $GameRuntime.ToLowerInvariant() }
    $preset = "$($Configuration.ToLowerInvariant())-msvc-vcpkg-$runtimeTag"
    $dll = Join-Path $buildRoot "$preset\skee64.dll"
    if (-not (Test-Path -LiteralPath $dll -PathType Leaf)) {
        throw "Build completed without the expected DLL: $dll"
    }

    $bridgeDll = Join-Path $buildRoot "$preset\racemenu-prisma-bridge\RaceMenuPrismaBridge.dll"
    if ($PromoteDirectory -and -not (Test-Path -LiteralPath $bridgeDll -PathType Leaf)) {
        throw "Build completed without the expected separate bridge DLL: $bridgeDll"
    }

    $hash = Get-FileHash -Algorithm SHA256 -LiteralPath $dll
    Write-Host "Verified compile artifact (not installed or distributed): $dll"
    Write-Host "SHA-256: $($hash.Hash)"

    if ($PromoteDirectory) {
        $bridgeHash = Get-FileHash -Algorithm SHA256 -LiteralPath $bridgeDll
        $promoteRoot = [IO.Path]::GetFullPath($PromoteDirectory)
        $authoritativeRoot = [IO.Path]::GetFullPath('L:\Codex').TrimEnd('\') + '\'
        if (-not $promoteRoot.StartsWith($authoritativeRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Promotion destination must be beneath the authoritative L:\Codex root: $promoteRoot"
        }

        $pluginDirectory = Join-Path $promoteRoot 'Data\SKSE\Plugins'
        $viewDirectory = Join-Path $promoteRoot 'Data\PrismaUI\views\RaceMenuPrismaBridge'
        New-Item -ItemType Directory -Force -Path $pluginDirectory, $viewDirectory | Out-Null

        Copy-Item -LiteralPath $bridgeDll -Destination (Join-Path $pluginDirectory 'RaceMenuPrismaBridge.dll') -Force
        $bridgePdb = [IO.Path]::ChangeExtension($bridgeDll, '.pdb')
        if (Test-Path -LiteralPath $bridgePdb -PathType Leaf) {
            Copy-Item -LiteralPath $bridgePdb -Destination (Join-Path $pluginDirectory 'RaceMenuPrismaBridge.pdb') -Force
        }
        Get-ChildItem -LiteralPath (Join-Path $projectPath 'racemenu-prisma-bridge\ui') -File |
            Copy-Item -Destination $viewDirectory -Force
        Copy-Item -LiteralPath (Join-Path $projectPath 'racemenu-prisma-bridge\README.md') -Destination $promoteRoot -Force
        Copy-Item -LiteralPath (Join-Path $projectPath 'LICENSE') -Destination $promoteRoot -Force
        Copy-Item -LiteralPath (Join-Path $projectPath 'THIRD_PARTY_NOTICES.md') -Destination $promoteRoot -Force

        $promotedDll = Join-Path $pluginDirectory 'RaceMenuPrismaBridge.dll'
        $promotedHash = Get-FileHash -Algorithm SHA256 -LiteralPath $promotedDll
        if ($promotedHash.Hash -ne $bridgeHash.Hash) {
            throw "Promoted bridge DLL hash mismatch: $promotedDll"
        }

        $receipt = [ordered]@{
            schema = 1
            target = 'RaceMenuPrismaBridge'
            version = '0.1.7.0'
            gameRuntime = $GameRuntime
            configuration = $Configuration
            crt = $Crt
            dll = 'Data/SKSE/Plugins/RaceMenuPrismaBridge.dll'
            sha256 = $promotedHash.Hash
            builtUtc = [DateTimeOffset]::UtcNow.ToString('o')
            status = 'compile-verified-not-runtime-tested'
            prismaRuntimeBundled = $false
        }
        $receipt | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $promoteRoot 'build-receipt.json') -Encoding utf8
        Write-Host "Promoted verified bridge package: $promoteRoot"
    }

    if ($PromoteBaselineDirectory) {
        $baselineSourceRoot = [IO.Path]::GetFullPath($BaselineAssetSource)
        if (-not (Test-Path -LiteralPath $baselineSourceRoot -PathType Container)) {
            throw "Baseline asset source is not a directory: $baselineSourceRoot"
        }

        $baselineInputs = [ordered]@{
            'RaceMenu.bsa' = Join-Path $baselineSourceRoot 'RaceMenu.bsa'
            'RaceMenu.esp' = Join-Path $baselineSourceRoot 'RaceMenu.esp'
            'RaceMenuPlugin.esp' = Join-Path $baselineSourceRoot 'RaceMenuPlugin.esp'
            'SKSE/Plugins/skee64.ini' = Join-Path $baselineSourceRoot 'SKSE\Plugins\skee64.ini'
            'Interface/VR/RaceSex_menu.swf' = $baselineMovie
        }
        foreach ($input in $baselineInputs.GetEnumerator()) {
            if (-not (Test-Path -LiteralPath $input.Value -PathType Leaf)) {
                throw "Required RaceMenu baseline asset is missing ($($input.Key)): $($input.Value)"
            }
        }

        $baselineRoot = [IO.Path]::GetFullPath($PromoteBaselineDirectory)
        $authoritativeRoot = [IO.Path]::GetFullPath('L:\Codex').TrimEnd('\') + '\'
        if (-not $baselineRoot.StartsWith($authoritativeRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Baseline promotion destination must be beneath the authoritative L:\Codex root: $baselineRoot"
        }
        if (Test-Path -LiteralPath $baselineRoot) {
            $existingEntries = @(Get-ChildItem -LiteralPath $baselineRoot -Force)
            if ($existingEntries.Count -ne 0) {
                throw "Baseline promotion destination already exists and is not empty: $baselineRoot"
            }
        }

        $baselineData = Join-Path $baselineRoot 'Data'
        $baselinePlugins = Join-Path $baselineData 'SKSE\Plugins'
        $baselineVrInterface = Join-Path $baselineData 'Interface\VR'
        $baselineModderResource = Join-Path $baselineData 'ModderResource'
        $baselineSymbols = Join-Path $baselineRoot 'symbols'
        New-Item -ItemType Directory -Force -Path $baselineData, $baselinePlugins, $baselineVrInterface, $baselineModderResource, $baselineSymbols | Out-Null

        Copy-Item -LiteralPath $baselineInputs['RaceMenu.bsa'] -Destination (Join-Path $baselineData 'RaceMenu.bsa') -Force
        Copy-Item -LiteralPath $baselineInputs['RaceMenu.esp'] -Destination (Join-Path $baselineData 'RaceMenu.esp') -Force
        Copy-Item -LiteralPath $baselineInputs['RaceMenuPlugin.esp'] -Destination (Join-Path $baselineData 'RaceMenuPlugin.esp') -Force
        Copy-Item -LiteralPath $baselineInputs['Interface/VR/RaceSex_menu.swf'] -Destination (Join-Path $baselineVrInterface 'RaceSex_menu.swf') -Force
        Copy-Item -LiteralPath $baselineInputs['SKSE/Plugins/skee64.ini'] -Destination (Join-Path $baselinePlugins 'skee64.ini') -Force
        $appearanceIni = Join-Path $baselinePlugins 'skee64.ini'
        if ((Get-Content -Raw -LiteralPath $appearanceIni) -notmatch '(?m)^\[Menu Appearance\]') {
            [IO.File]::AppendAllText($appearanceIni, "`r`n" + (Get-Content -Raw -LiteralPath (Join-Path $projectPath 'packaging\menu-appearance.ini.inc')))
        }
        if ((Get-Content -Raw -LiteralPath $appearanceIni) -notmatch '(?m)^\[Menu Profile VR Normal\]') {
            [IO.File]::AppendAllText($appearanceIni, "`r`n" + (Get-Content -Raw -LiteralPath (Join-Path $projectPath 'packaging\menu-profiles.ini.inc')))
        }
        Copy-Item -LiteralPath $dll -Destination (Join-Path $baselinePlugins 'skee64.dll') -Force
        Copy-Item -LiteralPath (Join-Path $projectPath 'skee64\IPluginInterface.h') -Destination (Join-Path $baselineModderResource 'IPluginInterface.h') -Force

        $corePdb = [IO.Path]::ChangeExtension($dll, '.pdb')
        if (Test-Path -LiteralPath $corePdb -PathType Leaf) {
            Copy-Item -LiteralPath $corePdb -Destination (Join-Path $baselineSymbols 'skee64.pdb') -Force
        }
        $coreMap = [IO.Path]::ChangeExtension($dll, '.map')
        if (Test-Path -LiteralPath $coreMap -PathType Leaf) {
            Copy-Item -LiteralPath $coreMap -Destination (Join-Path $baselineSymbols 'skee64.map') -Force
        }

        $shaderModules = @(Get-ChildItem -LiteralPath (Join-Path $projectPath 'skee64\Shaders') -Directory)
        foreach ($module in $shaderModules) {
            $moduleDestination = Join-Path $baselinePlugins "$($module.Name)\Shaders"
            New-Item -ItemType Directory -Force -Path $moduleDestination | Out-Null
            Get-ChildItem -LiteralPath $module.FullName -Force |
                Copy-Item -Destination $moduleDestination -Recurse -Force

            $compiledSource = Join-Path $buildRoot "$preset\Shaders\$($module.Name)\Compiled"
            if (-not (Test-Path -LiteralPath $compiledSource -PathType Container)) {
                throw "Compiled shader module is missing: $compiledSource"
            }
            $compiledDestination = Join-Path $moduleDestination 'Compiled'
            New-Item -ItemType Directory -Force -Path $compiledDestination | Out-Null
            Get-ChildItem -LiteralPath $compiledSource -Force |
                Copy-Item -Destination $compiledDestination -Recurse -Force
        }

        Copy-Item -LiteralPath (Join-Path $projectPath 'LICENSE') -Destination $baselineRoot -Force
        Copy-Item -LiteralPath (Join-Path $projectPath 'THIRD_PARTY_NOTICES.md') -Destination $baselineRoot -Force
        Copy-Item -LiteralPath (Join-Path $projectPath 'docs\vr2\baseline-package.md') -Destination (Join-Path $baselineRoot 'README.md') -Force

        $gitCommit = (& git -C $projectPath rev-parse HEAD).Trim()
        if ($LASTEXITCODE -ne 0) {
            throw 'Unable to record the RaceMenu source commit for the baseline receipt.'
        }
        $gitBranch = (& git -C $projectPath branch --show-current).Trim()
        if ($LASTEXITCODE -ne 0) {
            throw 'Unable to record the RaceMenu source branch for the baseline receipt.'
        }
        $gitStatus = @(& git -C $projectPath status --short)
        if ($LASTEXITCODE -ne 0) {
            throw 'Unable to record the RaceMenu source status for the baseline receipt.'
        }

        $runtimeFiles = @(Get-ChildItem -LiteralPath $baselineData -File -Recurse | Sort-Object FullName)
        $runtimeManifest = @($runtimeFiles | ForEach-Object {
            [ordered]@{
                path = $_.FullName.Substring($baselineRoot.Length + 1).Replace('\', '/')
                bytes = $_.Length
                sha256 = (Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName).Hash
            }
        })
        $promotedCoreHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $baselinePlugins 'skee64.dll')).Hash
        if ($promotedCoreHash -ne $hash.Hash) {
            throw "Promoted RaceMenu NG DLL hash mismatch: $(Join-Path $baselinePlugins 'skee64.dll')"
        }

        $receipt = [ordered]@{
            schema = 1
            target = 'RaceMenuNG-VR2-baseline'
            packageVersion = $BaselinePackageVersion
            nativePluginVersion = $NativePluginVersion
            gameRuntime = $GameRuntime
            gameVersion = '1.4.15.0'
            configuration = $Configuration
            crt = $Crt
            source = [ordered]@{
                repository = 'https://github.com/expired6978/SKSE64Plugins'
                branch = $gitBranch
                commit = $gitCommit
                workingTreeStatus = $gitStatus
                commonLibSseNgVersion = '8.0.1'
                skyrimVrAddressLibraryBaseline = '0.264.0'
            }
            baseAssets = [ordered]@{
                version = $BaselineAssetVersion
                sourcePath = $baselineSourceRoot
                included = @(
                    'RaceMenu.bsa'
                    'RaceMenu.esp'
                    'RaceMenuPlugin.esp'
                    'SKSE/Plugins/skee64.ini'
                    'Interface/VR/RaceSex_menu.swf'
                    'ModderResource/IPluginInterface.h'
                )
            }
            composition = [ordered]@{
                companionDllIncluded = $false
                legacySkeevrDllIncluded = $false
                looseVrRaceSexMenuIncluded = $true
                looseVrRaceSexMenuPurpose = 'Cap-aligned translucent slider rails, reduced-frequency search/scrollbar artwork, INI category filtering, enlarged VR ColorField, synchronized projected-UI yaw and filter/name text entry. Optional aspect-preserving PNG main-panel background and INI neutral text/background colours. Explicitly armed bounded native/movie input tracing is diagnostic only; compilation is not runtime qualification.'
                optionalPatchesIncluded = $false
                vrPlacementSettingsIncluded = 'skee64.dll applies RaceSexMenu-local native VRUI settings; values are configurable through SKSE/Plugins/skee64_custom.ini'
                excluded = @(
                    'RaceMenuPrismaBridge.dll',
                    'skeevr.dll',
                    'RaceMenu VR Layout Fix',
                    'RaceMenu VR Position Tweaks',
                    'RaceMenu Undress',
                    'ShowRaceMenu-NG',
                    'RaceMenu High Heels'
                )
            }
            runtimeManifest = $runtimeManifest
            builtUtc = [DateTimeOffset]::UtcNow.ToString('o')
            status = 'compile-verified-not-runtime-tested'
        }
        $receipt | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $baselineRoot 'build-receipt.json') -Encoding utf8
        Write-Host "Promoted verified self-contained RaceMenu NG baseline package: $baselineRoot"

        $mo2Archive = "$baselineRoot-MO2.7z"
        & (Join-Path $projectPath 'tools\package-vr2-mo2.ps1') -ArtifactRoot $baselineRoot -OutputPath $mo2Archive
        if ($LASTEXITCODE -ne 0) {
            throw "MO2-ready baseline archive creation failed with exit code $LASTEXITCODE"
        }
    }
    $buildSucceeded = $true
} finally {
    $releaseJson = & $scratchTool release -Id $allocationId -Disposition reclaimable -Compact
    $release = try { $releaseJson | ConvertFrom-Json } catch { $null }
    if (-not $release.ok -or $release.state -ne 'reclaimable') {
        Write-Warning "Managed scratch release failed: $releaseJson"
    } elseif ($buildSucceeded) {
        Write-Host "Released managed build allocation $allocationId as reclaimable."
    }
}

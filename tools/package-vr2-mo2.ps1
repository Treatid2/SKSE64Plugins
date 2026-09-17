[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string]$ArtifactRoot,

    [Parameter(Mandatory)]
    [string]$OutputPath,

    [string]$SevenZipPath = 'C:\Program Files\7-Zip\7z.exe',

    [switch]$VerifyOnly
)

$ErrorActionPreference = 'Stop'

$artifactPath = [IO.Path]::GetFullPath($ArtifactRoot)
$dataPath = Join-Path $artifactPath 'Data'
$archivePath = [IO.Path]::GetFullPath($OutputPath)
$sevenZip = [IO.Path]::GetFullPath($SevenZipPath)

if (-not (Test-Path -LiteralPath $dataPath -PathType Container)) {
    throw "Baseline artifact has no Data directory: $dataPath"
}
if (-not (Test-Path -LiteralPath $sevenZip -PathType Leaf)) {
    throw "7-Zip executable not found: $sevenZip"
}
if ((Test-Path -LiteralPath $archivePath) -and -not $VerifyOnly) {
    throw "Refusing to overwrite an existing archive: $archivePath"
}
if (-not (Test-Path -LiteralPath $archivePath) -and $VerifyOnly) {
    throw "Archive does not exist for verification: $archivePath"
}

$requiredRoots = @(
    'RaceMenu.bsa',
    'RaceMenu.esp',
    'RaceMenuPlugin.esp',
    'Interface',
    'SKSE'
)
foreach ($requiredRoot in $requiredRoots) {
    if (-not (Test-Path -LiteralPath (Join-Path $dataPath $requiredRoot))) {
        throw "MO2 package input is missing required root entry: $requiredRoot"
    }
}

if (-not $VerifyOnly) {
    Push-Location $dataPath
    try {
        & $sevenZip a -t7z -mx=9 $archivePath '.\*'
        if ($LASTEXITCODE -ne 0) {
            throw "7-Zip archive creation failed with exit code $LASTEXITCODE"
        }
    } finally {
        Pop-Location
    }
}

& $sevenZip t $archivePath
if ($LASTEXITCODE -ne 0) {
    throw "7-Zip archive verification failed with exit code $LASTEXITCODE"
}

$listing = @(& $sevenZip l -slt $archivePath)
if ($LASTEXITCODE -ne 0) {
    throw "7-Zip archive listing failed with exit code $LASTEXITCODE"
}
if ($listing -match '^Path = Data[\\/]') {
    throw 'MO2 archive incorrectly contains a top-level Data directory.'
}
foreach ($requiredFile in @('RaceMenu.bsa', 'RaceMenu.esp', 'RaceMenuPlugin.esp')) {
    if (-not ($listing -match "^Path = $([regex]::Escape($requiredFile))$")) {
        throw "MO2 archive does not expose $requiredFile at its root."
    }
}

$archive = Get-Item -LiteralPath $archivePath
$hash = Get-FileHash -Algorithm SHA256 -LiteralPath $archivePath
[ordered]@{
    schema = 1
    status = 'valid'
    artifactRoot = $artifactPath
    modRoot = $dataPath
    archive = $archive.FullName
    bytes = $archive.Length
    sha256 = $hash.Hash
    layout = 'MO2-ready; former Data contents are at archive root'
} | ConvertTo-Json

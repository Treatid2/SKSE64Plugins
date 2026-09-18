# Building RaceMenu VR 2

## Public source release

The 0.1.79 VR beta and pinned source bundle are available at
https://github.com/Treatid2/SKSE64Plugins/releases/tag/racemenu-vr2-v0.1.79 .
Native version is `0.5.0.89`. The release retains the exact headset-tested
candidate DLL and menu patch, not a later rebuild. Its build receipt records
the development worktree based on `3eef322`; the release tag commits those
corresponding sources. Documentation and packaging notes were refreshed after
headset testing without changing the native binary or runtime patch.
The source bundle includes CommonLibSSE-NG and OpenVR at the pinned revisions,
with both focused CommonLib corrections already applied. Its
`SOURCE-RELEASE.md` gives portable build instructions: no Codex, managed
scratch tool, MO2, original Nexus assets or JPEXS is needed to compile the DLL.

In a Windows x64 MSVC native-tools shell with Windows SDK/`fxc`, Git, Ninja,
CMake >=3.21 and bootstrapped vcpkg, use explicit writable paths outside the
extracted source tree:

```powershell
$env:VCPKG_ROOT = 'C:/development/vcpkg'
$env:SKEE_BUILD_ROOT = 'C:/development/racemenu-build'
cmake --preset release-msvc-vcpkg-vr `
  -DSKEE_VR2_PACKAGE_VERSION=0.1.79 -DSKEE_NATIVE_PLUGIN_VERSION=0.5.0.89
cmake --build --preset release-msvc-vcpkg-vr --parallel 4
```

`vcpkg.json` pins the dependency baseline and the ports identify upstream
source locations, hashes and licenses. See `THIRD_PARTY_NOTICES.md`.
For a recursive Git checkout, apply the two `evidence/` CommonLib patches to
a separate copy of the pinned dependency and pass its path as
`-DSKEE_COMMONLIB_SOURCE_DIR=...` at configure time. Do not apply those patches
again to the already-patched source bundle.

This is a VR beta, not complete cross-runtime qualification. The clean build
and keyboard/camera/SWF policy tests and all 23 production shader compiler
checks pass. Human OCU testing covers menu placement, wand alignment,
Sculpt deformation, History undo/redo, head export/clear/import, presets,
canvas expansion and viewpoint rotation. The warning has wrong/correct-original
acceptance. Performance stability was observed, not measured. Broader SteamVR
Sculpt, cross-session persistence, SE/AE checks and independent review remain
outstanding. The redundant Camera tab is removed in VR.

## Maintainer managed build

The supported Windows build wrapper is `tools/vr2-build.ps1`. It acquires a
managed `Kind=build` allocation through `CODEX_SCRATCH_TOOL`, stages the pinned
CommonLibSSE-NG source and its focused latent-function correction there, builds
entirely under `D:\CodexScratch`, verifies the output, and releases the
allocation as reclaimable.

The safe-load milestone supports clean dynamic-CRT AE and VR builds:

```powershell
pwsh -NoProfile -File .\tools\vr2-build.ps1 -GameRuntime AE -Configuration Release -Crt dynamic -Full
pwsh -NoProfile -File .\tools\vr2-build.ps1 -GameRuntime VR -Configuration Release -Crt dynamic -Full
```

The public build excludes the private Prisma bridge by default. In the private
development checkout only, to retain that separate experimental bridge,
pass an explicit destination beneath `L:\Codex`:

```powershell
pwsh -NoProfile -File .\tools\vr2-build.ps1 `
  -GameRuntime VR -Configuration Release -Crt dynamic -Full `
  -PromoteDirectory 'L:\Codex\artifacts\RaceMenu-VR-2\prisma-bridge\0.1.0-vr'
```

For private development only, to produce a self-contained Skyrim VR baseline, provide the
installed official RaceMenu data package as an explicit immutable asset source.
The result contains that package's BSA, ESPs, and `skee64.ini`, overlaid by the
newly compiled VR `skee64.dll`, shaders and VR menu. Baseline promotion also
requires the original source-pinned movie and JPEXS CLI; it compiles and verifies
the current menu patches in managed scratch rather than copying a stale movie.
It deliberately excludes the
separate Prisma bridge, legacy `skeevr.dll`, external VR layout fixes, and optional
RaceMenu patch mods:

```powershell
pwsh -NoProfile -File .\tools\vr2-build.ps1 `
  -GameRuntime VR -Configuration Release -Crt dynamic -Full `
  -BaselineAssetSource 'D:\path\to\installed\RaceMenu' `
  -BaselineMovieSource 'D:\path\to\original\Interface\VR\RaceSex_menu.swf' `
  -FfdecCli 'D:\path\to\ffdec\ffdec-cli.exe' `
  -BaselineAssetVersion '0.4.20.0' `
  -PromoteBaselineDirectory 'L:\Codex\artifacts\RaceMenu-VR-2\baseline\0.1.0-vr'
```

The promotion fails closed when a required base asset is missing, when the
destination is outside `L:\Codex`, or when the destination already contains
files. The generated `build-receipt.json` records source provenance, exclusions,
and a SHA-256 manifest of every deployable runtime file.

**Do not distribute that baseline:** it includes original Nexus assets. Public
packaging uses `tools/stage-vr2-addon.ps1` with a verified retained native build and
a new staging directory. Package the resulting `Data` contents directly at ZIP
root. That stage omits original assets and includes the runtime byte patch;
see `docs/release/INSTALLATION.md`. Use managed build scratch for staging.

For the runtime-patch candidate, use `tools/vr2-build.ps1` with
`-BaselinePackageVersion 0.1.54-runtime-swf-patch -NativePluginVersion 0.5.0.64`
and `-PromoteNativeDirectory` pointing to a new L:\Codex artifact directory.
This produces an asset-free native manifest suitable for the staging tool.
Do not use the legacy private baseline promotion for the runtime route: its
generated loose SWF conflicts with the exact-original input requirement.
Maintainer patch generation/auditing is documented in `docs/runtime-swf-patch.md`.

PowerShell 7 is required because the managed scratch controller uses current
.NET filesystem APIs. An installed Visual Studio x64 environment may be
selected explicitly:

```powershell
pwsh -NoProfile -File .\tools\vr2-build.ps1 `
  -GameRuntime VR -Configuration Release -Crt dynamic -Full `
  -VcvarsPath 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
```

Do not run CMake or the lower-level `build.ps1` in a way that creates
`build`, `bin`, `obj`, or `out` trees in this checkout. Do not use `--install`
for the VR safe-load build. A successful VR link proves only compilation; the
DLL must not be described as fully runtime-qualified merely because it links.
The published VR beta discloses its accepted live checks and outstanding
regressions; see the immutable release verification and release notes.

## Pinned inputs

- RaceMenu source: this repository revision.
- CommonLibSSE-NG: project version `8.0.1`, pinned submodule commit `d13d10a0`
  (full commit recorded by the gitlink). Exported from Git, not its dirty tree.
- CommonLib latent-function correction:
  `evidence/commonlibsse-ng-latent-vr.patch`, applied only to the managed
  scratch staging copy.
- CommonLib VR RaceSexMenu layout correction:
  `evidence/commonlibsse-ng-racesex-vr.patch`, also applied to staged source.
- Skyrim VR Address Library baseline: `v0.264.0`, commit
  `710f98387257885c963d749096a26879cface89c`.
- vcpkg baseline: the `builtin-baseline` in `vcpkg.json`.

The Address Library version is a qualification baseline, not permission to
enable an address. `docs/vr2/hooks.json` remains authoritative for which custom
relocations and patch sites have completed verification.

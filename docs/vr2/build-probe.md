# Skyrim VR compile probe

## Result

A Release/dynamic-CRT Skyrim VR build completed from a temporary clone after
updating CommonLibSSE-NG and applying the minimal project patch in
`evidence/racemenu-vr-build-probe.patch` plus the dependency-only correction in
`evidence/commonlibsse-ng-latent-vr.patch`.

The resulting file was:

- name: `skee64.dll`
- size: `7,098,880` bytes
- SHA-256:
  `168C362E227961DABE659B0BC4DDEF6D6ED9FFB64F49A6FB1D4B8B1A9C521706`
- build time: 2026-09-09 00:41:14 UTC

This file is compile evidence only. It must not be installed or distributed,
because the custom hook relocations have not been qualified for Skyrim VR.

## Probe revisions

- RaceMenu source:
  `7ceab706e0f4fdd1816f8c61390ff33ecdeda1f8`
- CommonLibSSE-NG: `v8.0.1`,
  `9b17b42fc9db23aea2b60f92690e778784e612b1`

The dependency recheck confirmed CommonLibSSE-NG `v8.0.1` is
still the newest release (the `ng` head is one documentation-only commit
later). Skyrim VR Address Library advanced to `v0.264.0`, commit
`710f98387257885c963d749096a26879cface89c`. That update does not qualify any
RaceMenu hook by itself.

The branch originally pinned CommonLibSSE-NG `v7.2.0`, commit
`7a60f4de794095d7b0f8928d1b930a52e9a7da83`.

## Minimal compile changes

1. Configure CommonLib for VR rather than AE.
2. Exclude three accesses to the flat-runtime-only `NiSkinInstance::lock` field
   when building VR.
3. Correct a CommonLib `NativeLatentFunction` template expression from
   `GetRawType<latentR>()` to `GetRawType<latentR>{}()`.
4. Update the CommonLibSSE-NG submodule to `v8.0.1`.

The CMake switch in the evidence patch is deliberately a probe, not the final
design. The implementation should add AE/VR presets or options rather than
changing the shared branch to VR-only.

## Build environment

The probe used the repository's `build.ps1`, Release configuration, dynamic
CRT, and Visual Studio 2026 Community's x64 environment:

```powershell
$env:SKEE_VCVARS = `
  'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
.\build.ps1 release --dynamic --full
```

The repository's portable-MSVC fallback failed before compilation because its
Windows SDK download URL contained an unescaped space (`w kits2`). Explicitly
selecting the installed `vcvars64.bat` avoided that unrelated tooling defect.
The project should either repair the fallback separately or document a tested
toolchain requirement.

## Storage rule

All configure, compile, link, package, and test output must live in a managed
`Kind=build` allocation obtained through `CODEX_SCRATCH_TOOL`. Source and final
artifacts remain authoritative on `L:\Codex`; generated trees belong on
`D:\CodexScratch`. A new task must read the scratch manager documentation before
its first allocation.

# RaceMenu VR 2 development hand-off

## Mission

Develop a first-class Skyrim VR build of RaceMenu from Expired's current,
GPL-covered `CommonLibSSE-NG` branch. The result must be maintainable upstream,
fail safely when a VR hook is unavailable, and be distributable as a
compatibility package that keeps RaceMenu NG as a prerequisite.

This is a new development project. It is deliberately separate from the crash
diagnosis work that discovered the opportunity.

## Non-negotiable boundaries

- Base all implementation work on this repository's `CommonLibSSE-NG` branch.
- Do not copy source, binaries, offsets, patches, or derived implementation
  details from Nightfallstorm's separately licensed RaceMenuVR adaptation.
- Treat RaceMenu NG as a prerequisite for the user-facing package. Ship only
  the VR compatibility payload that must override the incompatible DLL and any
  genuinely new project-owned files.
- Publish complete corresponding GPL source, reproducible build instructions,
  notices, and exact source revision for every distributed DLL.
- Never deploy the compile-probe DLL described below. It proves that the source
  can compile for VR; it does not prove that the installed hooks are safe.
- Do not modify installed modlists while developing. Runtime validation must use
  a separate MO2 patch package and controlled session.
- Preserve AE behavior. Upstream work should add a VR configuration and localize
  runtime divergence, not permanently turn an AE branch into VR-only source.

See [docs/vr2/licensing-and-distribution.md](docs/vr2/licensing-and-distribution.md)
for the provenance and release policy.

## Authoritative starting points

- Upstream source: <https://github.com/expired6978/SKSE64Plugins/tree/CommonLibSSE-NG>
- Checked-out upstream commit:
  `7ceab706e0f4fdd1816f8c61390ff33ecdeda1f8`
- GPL license introduction by the owner:
  `c7237ada0ee5b4ad344790f579e35aed4103b06f`
- CommonLibSSE-NG upstream: <https://github.com/alandtse/CommonLibSSE-NG>
- Skyrim VR Address Library:
  <https://github.com/alandtse/skyrim_vr_address_library>

At hand-off time, the newest investigated dependency revisions were:

- CommonLibSSE-NG `v7.4.0`, commit
  `9b17b42fc9db23aea2b60f92690e778784e612b1` (2026-09-07).
- Skyrim VR Address Library `v0.263.0`, commit
  `a7faef64f78f799321334963c41e52d0056b5d7a` (2026-09-08).

Verify those versions again before opening a PR or publishing a build.

## What is already established

The source branch can compile as a Skyrim VR/CommonLibSSE-NG target after four
small build-compatibility changes and updating CommonLibSSE-NG to `v7.4.0`.
The probe produced a DLL, but the current hook table is not VR-safe.

The decisive blocker is `skee64/SKEEHooks.h`: its custom relocations use
`REL::RelocationID(0, kID_...)`. Under VR the first argument becomes the VR ID,
so every such relocation currently resolves from zero. Some hook installation
also performs offset scans unconditionally. A successful link therefore does
not imply a loadable plugin.

The GPL branch already contains several fixes that existed in the local binary
replacement investigation, including:

- `NIOVTaskDeferredMask::Dispose()` deleting the task;
- body-morph worker join/publication behavior; and
- dynamic/default GPU buffer update behavior.

Do not recreate or port those fixes from another source tree.

The compile evidence and exact minimal patches are recorded in
[docs/vr2/build-probe.md](docs/vr2/build-probe.md) and
[evidence/racemenu-vr-build-probe.patch](evidence/racemenu-vr-build-probe.patch).
The dependency-only correction is kept separately in
[evidence/commonlibsse-ng-latent-vr.patch](evidence/commonlibsse-ng-latent-vr.patch).

## Development order

1. Establish clean, repeatable AE and VR presets using current CommonLibSSE-NG.
   Keep all generated build trees in a managed `Kind=build` allocation on
   `D:\CodexScratch`; do not build beneath this `L:` checkout.
2. Make a VR build load safely with every unqualified custom hook disabled.
   Missing or mismatched hooks must fail closed with a precise log message.
3. Split hook installation into independent feature groups. One unavailable
   face/sculpt hook must not disable serialization, Papyrus, body morphs, or
   other independently safe services.
4. Qualify core non-face services: SKSE messaging, serialization, Papyrus,
   plugin interfaces, BodyGen/body morph, skeleton work, and body overlays.
5. Qualify the minimum useful face feature set: native slider callbacks and
   morph update/extended TRI hooks.
6. Add sculpt rendering, then head preprocessing/presets and tint handling.
7. Qualify face-overlay allocation and update hooks last because they are the
   most invasive group.
8. Validate AE regressions and VR runtime behavior, package the dependency-style
   mod, and prepare focused upstream PRs.

The hook groups and evidence requirements are in
[docs/vr2/hook-qualification.md](docs/vr2/hook-qualification.md).

## Upstream split

- RaceMenu implementation and build work: PR to
  `expired6978/SKSE64Plugins`, targeting `CommonLibSSE-NG`.
- Verified missing VR addresses: focused PRs to
  `alandtse/skyrim_vr_address_library` with provenance and instruction checks.
- The latent-function template correction, if still present upstream: a
  separate focused PR to `alandtse/CommonLibSSE-NG`.

Do not bundle dependency corrections into the RaceMenu PR unless upstream
explicitly requests that arrangement.

## Minimum release acceptance

- Clean AE and VR builds from documented commands and pinned revisions.
- The VR DLL loads without attempting a zero, unverified, or mismatched hook.
- Every enabled hook has an exact VR entry point, verified instruction window,
  documented overwrite size, and tested original-call/trampoline contract.
- Missing optional groups disable only their own feature and say why in the log.
- Core BodyGen/body-morph persistence survives save, load, cell change, and
  actor unload/reload.
- Face features advertised by the release pass a documented RaceSexMenu test;
  unavailable features are not presented as working.
- AE regression checks cover the same touched hook and interface paths.
- The distributable package requires RaceMenu NG and contains no third-party
  assets that the project is not entitled to redistribute.
- A release tag/source archive, build recipe, dependency revisions, GPL license,
  notices, and binary checksum are published together.

## First task actions

1. Read this document and all files under `docs/vr2/` and `evidence/`.
2. Confirm the task has unrestricted access with approval policy `never` before
   using shared `L:\Codex` state. Stop and report if it does not.
3. Inspect the repository's current branch, remotes, submodule state, and user
   changes. Do not overwrite the hand-off package.
4. Recheck the latest upstream and dependency heads.
5. Design the feature-gated hook installer and a machine-readable qualification
   inventory before adding any runtime address.
6. Start with the safe-load milestone; do not jump directly to face overlays.

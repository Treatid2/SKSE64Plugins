# Proposed upstream contribution

Target: `expired6978/SKSE64Plugins`, base `CommonLibSSE-NG`.
Owner/head fork: `Treatid2/SKSE64Plugins`.
Proposed title: **Add qualified Skyrim VR runtime and character-creation UI support**.

One integrated VR PR keeps the constructor/input/keyboard/name callbacks,
runtime layouts, face view, placement and menu recipe consistent. Split it on
upstream request, rather than proposing independently installable pieces whose
movie/native contracts depend on each other. The experimental Prisma bridge
and unlicensed complete assets are excluded. Original assets are local inputs.

## Scope

- Runtime-selectable build and version identities; VR 1.4.15-qualified hooks
  with address/instruction checks and fail-closed feature groups.
- VR runtime layout and mesh/lifecycle adaptations without importing legacy
  SKEEVR implementation.
- Aligned projected pointer input, native virtual keyboard lifecycle,
  accepted-name update distinct from final creation completion.
- Stable normal/face placement, independent picker placement, INI appearance,
  native PNG registration and projection-aware aspect preservation.
- Backward-compatible CharacterCreation vtable prefix and separately versioned
  MenuExtensions API; configuration and bounded read-only/opt-in diagnostics.
- Exact-input automatic in-memory COPY/INSERT SWF patch, audited literals,
  native/reference parser tests, maintainer recipe and additive
  packaging documentation. No full Nexus menu/class/artwork redistribution.

## Dependency PRs (already submitted)

- https://github.com/alandtse/CommonLibSSE-NG/pull/363 — RaceSexMenu VR runtime data.
- https://github.com/alandtse/CommonLibSSE-NG/pull/364 — latent return type mapper call.

Keep the currently tested CommonLib **8.0.1** revision for initial reproduction;
focused local source patches are retained. The upstream CommonLib PRs were
ported/tested against its current 8.1 branch; do not silently change dependency
versions in the release source. Remove backports only after retesting the bump.

## Evidence and review limitations

The 0.1.53 development build passed the reported OCU headset checks including
name save/load. SteamVR with the separately installed Mouse Fix was reported
workable. The public asset recipe reproduces the exact accepted movie SHA-256
`E68FCBB62E26271FBD925761DFB77A194AB9856F13F703EA8BFF975DDDE24FC2`.
This is offline equivalence, not a new live test.

The runtime-patch candidate serves the equivalent uncompressed FWS from
Skyrim's own memory-file object. Only 6,308 literal bytes are carried in the
patch. See `docs/runtime-swf-patch.md`; the new adapter still requires live
qualification before merge/release. Player-side JPEXS/extraction is removed.

The clean public-source checkout at `caeedf0` now compiles for VR Release,
and the native UTF-8 keyboard policy test passes. No LNK/unresolved-symbol
diagnostics were found in the retained build logs. Dependency licence texts
are included in the asset-free staging recipe. See
`docs/release/PREPARATION-EVIDENCE.md` for hashes and boundaries.

Before calling this merge/release-ready: rebuild/qualify the final immutable
release tag, recheck AE regression and native consumer ABI/lifecycle, and run broader
SteamVR text/modal/reopen qualification. Upstream's existing
`/FORCE:UNRESOLVED` linker setting is inherited; audit actual unresolved-symbol
diagnostics rather than treating forced-link success as proof of validity.

The PR should be **draft** during this remaining release audit. No Nexus
publication or upstream acceptance is claimed by preparing this contribution.

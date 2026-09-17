# Preparation evidence — 17 September 2026

This is preparation evidence, not a release certification or new headset test.

- Source fork: Treatid2/SKSE64Plugins, `codex/racemenu-vr-addon`.
- Draft upstream integration: expired6978/SKSE64Plugins PR #66, NG base.
- Separate CommonLibSSE-NG PRs: alandtse/CommonLibSSE-NG #363 and #364.
- Clean detached public checkout `caeedf0`: Windows MSVC Release VR build
  succeeded, 52.747 seconds. No LNK/unresolved-symbol diagnostics in the
  retained build stdout/stderr. `/FORCE:UNRESOLVED` remains inherited, not removed.
- Native keyboard UTF-8 policy test passed. This is not runtime modal testing.
- Clean-source compile artifact SHA-256:
  `6CDE473A142615E031876B1B5158636352A1A620E2517D74A23FA52D2689CCD7`.
  This binary has not been substituted for the headset-tested development one.
- Four Node production-AS2 surrogate suites passed: appearance, wishlist,
  input tracing and text entry. Surrogates do not establish engine behaviour.
- Local JPEXS 26.2.1 recipe output matches the accepted 0.1.53 movie exactly:
  `E68FCBB62E26271FBD925761DFB77A194AB9856F13F703EA8BFF975DDDE24FC2`.
  Neither original nor generated movie is included in public distribution.
- Candidate native payload retains the headset-tested development binary:
  `3CB289C2338C8397038272BDAC9F96A4CB783652A4B2DDE1CF18A95939F2263E`.
  It was built from a dirty development worktree; it is explicitly not tagged
  public-release provenance.
- Packaging uses an explicit asset-free allowlist and the MO2 deterministic
  package controller. Its archive root is the contents of Data, with no
  installer flattening required. The local menu recipe is still mandatory.

Windows clean-source reproduction exposed archive handling gaps for repository
metadata symlinks and the nested OpenVR gitlink. The wrapper now exports pinned
build roots plus exact OpenVR Windows headers/import library and licence.

Independent review enrolment is pending; no review pass is claimed. Broader
SteamVR qualification, AE regression, native extension ABI/lifecycle checks,
installer UX and Nexus DP allocation remain in RELEASE-CHECKLIST.md.

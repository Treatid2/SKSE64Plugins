# VR beta release checklist

Current beta: headset-accepted package 0.1.80, native 0.5.0.90.
Historical checks below retain their original source/build scope where stated.

- [x] OCU human validation: layout, face view, picker, name/filter, name save/load.
- [x] User accepts VR Menu Mouse Fix as the SteamVR-specific dependency.
- [x] Background fill ratio and coordinate definitions are documented in INI.
- [x] Runtime recipe reconstructs the accepted 0.1.79 movie from exact original input.
- [x] Native add-on packaging uses an explicit allowlist, not the private baseline.
- [x] No original BSA/ESP/SWF/full upstream class/custom artwork in public payload.
- [x] Separate independent CommonLib PRs submitted (#363, #364).
- [x] Treatid2 source branch pushed; draft Expired upstream PR #66 opened.
- [x] Clean public-source VR checkout compiles (caeedf0); native keyboard policy test passes.
- [x] Dependency licence texts copied from pinned build inputs into the add-on.
- [x] Retain exact headset-tested 0.1.80 DLL and unchanged 0.1.79 menu patch; refresh documentation only.
- [x] Inventory-preview layout/bounds regression tests and human armour/clothing previews.
- [ ] Independently rebuild the final immutable source tag and qualify that rebuild.
- [ ] Broader SteamVR normal/face, Filter/Name, picker, close/reopen checks.
- [ ] AE regression and native consumer ABI/lifecycle checks for final source.
- [ ] Independent PR-scale review returned and evaluated.
- [x] Runtime COPY/INSERT patch generated; 8,300 literal bytes, no original four-byte windows in literal runs.
- [x] JavaScript exact reconstruction, wrong-input, corruption and malformed-record tests pass.
- [x] Native exact reconstruction and fail-closed parser tests qualified (0.1.54 candidate); both private input files unchanged.
- [x] Runtime file adapter accepted live with original BSA only (no generated SWF).
- [ ] Menu reopen/cache, unrelated menus, Filter/Name and PNG regression checks for this new route.
- [x] Repaired camera callbacks retain live Face View and close/reopen operation.
- [x] Redundant Camera tab removed in VR; Sculpt retains its semantic tab ID.
- [x] Human Sculpt deformation, History undo/redo, export/clear/import and preset cycle.
- [x] Human OCU workspace, expansion, turn/centre controls and Face/Normal rotation retention.
- [ ] Cross-session Sculpt persistence and broader SteamVR Sculpt regression.
- [x] Wrong/correct original notice checked live; parser failure tests pass.
- [ ] Recipient/account confirmation and Nexus DP allocation to Expired.
- [ ] Public source/tag/archive and exact source link in Nexus/download README.
- [x] User explicitly requested publishing to Nexus and PRs on 2026-09-17.

Do not upload the older development baseline archives: they include original
Nexus assets. Do not claim prepared source/binaries have passed unperformed
release checks. The VR beta is not an SE/AE standalone qualification.
Independent automatic review is unavailable because the registered service
cannot issue the documented bounded review-only credential. No access bypass
was attempted. Outstanding checks remain visible rather than marked passed.

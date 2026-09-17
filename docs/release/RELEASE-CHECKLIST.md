# Release gate — prepared, not publicly published

Baseline: accepted development package 0.1.53, native 0.5.0.63.

- [x] OCU human validation: layout, face view, picker, name/filter, name save/load.
- [x] User accepts VR Menu Mouse Fix as the SteamVR-specific dependency.
- [x] Background fill ratio and coordinate definitions are documented in INI.
- [x] Asset recipe generates the exact accepted 0.1.53 movie from original input.
- [x] Native add-on packaging uses an explicit allowlist, not the private baseline.
- [x] No original BSA/ESP/SWF/full upstream class/custom artwork in public payload.
- [x] Separate independent CommonLib PRs submitted (#363, #364).
- [x] Treatid2 source branch pushed; draft Expired upstream PR #66 opened.
- [x] Clean public-source VR checkout compiles (caeedf0); native keyboard policy test passes.
- [x] Dependency licence texts copied from pinned build inputs into the add-on.
- [ ] Rebuild/package final immutable public release tag and qualify that binary.
- [ ] Broader SteamVR normal/face, Filter/Name, picker, close/reopen checks.
- [ ] AE regression and native consumer ABI/lifecycle checks for final source.
- [ ] Independent PR-scale review returned and evaluated.
- [x] Runtime COPY/INSERT patch generated; 6,308 literal bytes, no original four-byte windows in literal runs.
- [x] JavaScript exact reconstruction, wrong-input, corruption and malformed-record tests pass.
- [x] Native exact reconstruction and fail-closed parser tests qualified (0.1.54 candidate); both private input files unchanged.
- [ ] Runtime file adapter qualified live with original BSA only (no generated SWF).
- [ ] Menu reopen/cache, unrelated menus, Filter/Name and PNG regression checks for this new route.
- [ ] VR Camera-tab position/movement, Face View interaction, close/reopen cleanup and Sculpt checks with the repaired camera and shader callbacks.
- [ ] Wrong-version/missing-patch refusal checked through the deployed entry point.
- [ ] Recipient/account confirmation and Nexus DP allocation to Expired.
- [ ] Public source/tag/archive and exact source link in Nexus/download README.
- [ ] Final upload/visibility/version approval before publishing to Nexus.

Do not upload the older development baseline archives: they include original
Nexus assets. Do not claim prepared source/binaries have passed unperformed
release checks. No installation/profile change is part of packaging preparation.

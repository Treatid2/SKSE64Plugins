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
- [ ] Clean source checkout build matching an immutable public release tag.
- [ ] Complete third-party licence texts for statically linked dependencies.
- [ ] Broader SteamVR normal/face, Filter/Name, picker, close/reopen checks.
- [ ] AE regression and native consumer ABI/lifecycle checks for final source.
- [ ] Independent PR-scale review returned and evaluated.
- [ ] Local asset preparation UX independently tried from the public instructions.
- [ ] Recipient/account confirmation and Nexus DP allocation to Expired.
- [ ] Public source/tag/archive and exact source link in Nexus/download README.
- [ ] Final upload/visibility/version approval before publishing to Nexus.

Do not upload the older development baseline archives: they include original
Nexus assets. Do not claim prepared source/binaries have passed unperformed
release checks. No installation/profile change is part of packaging preparation.

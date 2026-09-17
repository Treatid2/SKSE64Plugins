# RaceMenu VR 2 public beta publication

User explicitly requested Nexus publication and upstream PRs on 2026-09-17.

- Nexus: https://www.nexusmods.com/skyrimspecialedition/mods/192158
  Published 2026-09-17; version 0.1.58, category VR, one Main/Primary ZIP.
- GitHub: https://github.com/Treatid2/SKSE64Plugins/releases/tag/racemenu-vr2-v0.1.58
  Public prerelease with binary, pinned source bundle, hashes and verification.
- Annotated tag targets `6d44b35f7a4bb17a1fb955a9378021d27657477d`.
  Native version `0.5.0.68`; clean checkout build receipt has no dirty entries.
- Native ZIP: 2,888,005 bytes; SHA-256
  `1604DF1FF8E7C5E556D71FCBA0C6E64D5DC0E11FB27658697AE930E3CA1EC911`.
  All 77 members independently stream-hashed against the staged manifest.
- Source ZIP: 108,370,616 bytes; SHA-256
  `E8D51AF143BA115D1EE648DC1E2C94AFAEF6D011C025A221F25593BC149DA710`.
  All 3,721 source files independently checked. Includes the pinned CommonLib
  and OpenVR source, already-applied CommonLib corrections and portable CMake
  instructions. Dependency source resolution is pinned by the vcpkg manifest.
- No original RaceMenu BSA, ESP, scripts, complete SWF or background artwork
  is included. RaceMenu SE 0.4.20.0 is a genuine Nexus prerequisite.
  SteamVR's VR Menu Mouse Fix is documented as a conditional prerequisite.
- Nexus permissions use the inherited GPL-3.0-or-later license with dependency
  notices and explicit exclusion of original Nexus assets/custom artwork.
- Expired PR66 is open and ready for review, not draft. CommonLib PR363 and
  PR364 remain open and non-draft. PR66 received the publication links and
  qualification boundaries, not an assertion of unperformed regression tests.

Offline tests passed: native keyboard policy, camera dispatch/coordinates,
SWF fail-closed parser and production compilation of all 23 shader sources.
The functional predecessor received human acceptance for Face View, name/filter,
save/load, PNG aspect and reopen. The final version-only rebuild was not
installed for a separate headset test. Camera-tab movement is ineffective;
broader Sculpt/SteamVR/SE/AE regression and independent automatic review are
outstanding and disclosed in the release.

Nexus rewards are **not opted in**. The user must complete the financial
allocation to Expired in the rewards UI; no allocation success is claimed.
The published description states the intention without claiming configuration.

Permanent receipts and artifacts:
`L:/Codex/artifacts/RaceMenu-VR-2/publication/`.
The final clean build is retained under `native-0158/`; managed-process logs
are under `L:/Codex/logs/managed-process/20260917-134400.450-racemenu-public-beta-0158-clean-export/`.
An initial clean dependency export failure (missing `.clang-format` listed by
CommonLib CMake) was diagnosed and corrected in 6d44b35, not blindly retried.

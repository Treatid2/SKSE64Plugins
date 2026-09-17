# Incompatible movie warning candidate

0.1.59 / native 0.5.0.69 is built from
`bea09eccbd0a28a24413f643c8b111e620901962`. It adds a typed original-SWF
incompatibility diagnostic, and a one-time deferred Skyrim MessageBoxMenu
warning with a null OK callback. Loader/recipe errors receive a distinct
installation warning. All existing patch hash checks and fail-closed behavior
remain; original files are untouched, and restart is required after fixing
the winning resource because preparation is cached once per process.

The SKSE game-thread task captures only the failure enum, no menu/movie or
exception references. Installed VR Address Library maps CommonLib Create ID
51420 to RVA 0x08d7fe0. This confirms address availability, not live rendering
or interaction acceptance.

Managed VR Release build passed in 51.668 seconds. Updated native SWF parser
tests cover original size/hash/canonical incompatibility versus output/recipe
errors and message content. All 23 production shader sources and camera
policy tests passed. Private original/accepted movie equivalence was not run
in this candidate; the recipe bytes are unchanged.

Retained logs:
`L:/Codex/logs/managed-process/20260917-223234.584-racemenu-swf-player-warning-0159/`.
Native DLL: 7,762,432 bytes, SHA-256
`D0CA183FD096D19A141762CCD539A45C839A648F698FBD01E6BB743323B89E17`.

The test ZIP is direct Data-root with 78 files, independently stream-hashed
against the exact stage manifest plus candidate notes:
`L:/Codex/artifacts/RaceMenu-VR-2/swf-warning-0159/RaceMenu-VR2-0.1.59-test.zip`.
2,891,749 bytes, SHA-256
`7BBAFF3462B394FAFC26C3D4C419E1013A45565D3052FDB37CDAB18B2B5D4C11`.
No original asset payload. Candidate notes and verification receipts are
retained beside it. Unrelated dirty/untracked worktree is disclosed in the
native build receipt; the build wrapper exports pinned dependency sources,
not dirty dependency contents. Those unrelated files are not in the ZIP.

**Not installed, not headset-qualified, not published.** Actual warning
appearance/dismissal, no repeat spam, corrected-original restart and working
menu regression still need live acceptance. Public 0.1.58/tag is unchanged.
Independent review RMVR2-0158-20260917-01 covers the previous d1f63fa snapshot,
not this later warning; it was not resubmitted or claimed to cover new code.

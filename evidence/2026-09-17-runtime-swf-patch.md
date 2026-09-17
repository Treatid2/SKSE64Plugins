# Runtime SWF patch candidate — 17 September 2026

Scope: automatic, exact-input, in-memory reconstruction of the accepted VR
menu without distributing a whole movie or original class/artwork block.
Candidate package 0.1.54-runtime-swf-patch; native 0.5.0.64. Not a public release.

The checked-in RMSWFP01 payload is 148,722 bytes, SHA-256
`bdfa091f19b2601d1314c740361949cbd957f2cb90799a2299d78bfafb735876`.
Of 417,394 output bytes, 411,086 are copied from the verified canonical original
and 6,308 are literals. Longest literal run: 13 bytes. No original four-byte
window is carried inside any literal run. This is byte-reuse evidence, not
authorship classification. Full original/accepted movies remain private.

Eight Node reference tests pass with private original/accepted inputs, including
deterministic regeneration of the checked-in patch and complete exact output.
Independent Windows native tests pass, including compressed/truncated input,
record bounds, wrong originals, output hashes and release-payload corruption.
The native reconstructed output exactly matches the accepted canonical FWS.
Both private original and accepted file hashes were unchanged after testing.

Skyrim's file opener and its GMemoryFile constructor/read/seek/destructor were
statically qualified from a retained immutable dump, without a live debugger.
Disk SteamStub-encrypted code was not used as plaintext qualification evidence.
The adapter checks exact runtime signatures and opener identity, refuses an
existing conflicting opener hook, forwards unrelated paths and checks that the
verified movie was actually served. Source/patch/output limits and hashes are
enforced before Scaleform parsing. It never writes or restores an original SWF.

Build qualification exposed unavailable BCryptHash under the pinned Windows 7
API surface; the implementation uses supported provider/hash-handle operations
with exception-safe cleanup. Native promotion inspection caught nested shader
source directories; promotion now preserves exact shader paths, and staging
requires every source's corresponding compiled shader. Neither failed precursor
is the current test candidate.

Current asset-free archive: `RaceMenu-VR2-0.1.54-runtime-swf-patch-TEST-r5.zip`.
SHA-256 `5c7c8bfc5199682b41e47a36952513ea0b082c0dc7e863d2a956c33b689ead4b`.
Native DLL SHA-256
`19300f53a368c95c32fa4f156b634f471efbb78e4396b24c41922a18fd5318bc`.
All 77 archive members were independently read and SHA-256 checked against the
explicit staging manifest. Direct Data-root; no installer flattening. Payload
contains no original BSA/ESP/SWF/scripts/PNG, original INI, full upstream class,
legacy binary or manual asset-patching toolkit. Required dependency notices
include zlib 1.3.1. Existing original assets remain an installation prerequisite.

Read-only provider verification also passed for the installed original
RaceMenu.bsa: 16,592,056 bytes, SHA-256
`2e4a47aefa8a12dad1b3d3dfa58f3b39e9672b24e66e4d0a3c2b0b7ddcc6c6ab`.
Its sole required movie expands to the exact original SWF hash above, with no
asset extraction. Archive hash/mtime were unchanged. The permanent receipt is
`original-bsa-member-proof.json` in the evidence directory below.

Retained private evidence and candidate are under
`L:\Codex\artifacts\RaceMenu-VR-2\runtime-swf-patch`: `patch-audit.json`,
`reference-tests.log`, `private-loader-qualification.log`, `native-build-r4.log`,
`native-r4/build-receipt.json`, `addon-receipt-r5.json` and `package-r5.json`.
The receipt records the dirty development worktree; no immutable release
provenance or live loader success is claimed. Existing profile owner was asked
to install additively with the original asset provider and no generated SWF,
preserving the user's settings/artwork and not automatically launching Skyrim.

Remaining qualification: live original-BSA menu load, reopen/cache and unrelated
menus; name/filter, face view, picker and background regressions; wrong-input
and missing-patch refusal through the deployed entry point. SteamVR/release/AE
gates remain in the release checklist. No Nexus publication was performed.

# Automatic runtime SWF patch — candidate contract

## Player experience and boundaries

Install original RaceMenu SE 0.4.20.0 assets and our matching add-on. On the
first RaceSexMenu load, native code reads the winning original resource
through Skyrim's BSA/loose-file resolver. It verifies the exact compressed
file, decompresses it, performs deterministic COPY/INSERT operations, and
verifies the complete reconstructed FWS movie before Scaleform parses it.
Later opens reuse the immutable bytes. No launcher, Java/JPEXS, extraction,
generated-file installation, disk cache or shutdown restoration is needed.

Existing unrelated files, imported movies, fonts, button/bottom-bar resources,
scripts and artwork still come from the player's original installation.
The add-on does not bundle them. A modified loose VR SWF is a conflict, not
another supported input. No heuristic matching or best-effort patching occurs.

## Exact release identities

| Item | Bytes | SHA-256 |
| --- | ---: | --- |
| Original compressed CWS | 104279 | `3a012da4fed80637ce3257b9b2b89243befab29a4bec5316a935cea87c889963` |
| Original canonical FWS | 372756 | `647d722b15becb418cf07af09c6e2f1d5fa5a0d2b308e23c0db12339eea4ae4e` |
| Candidate 0.1.73 canonical output FWS | 432677 | `10a5f9ab649672884f0f8cfb8dec4616c7e345c40c288c4f7ab31868c19af66a` |
| Runtime patch RMSWFP01 | 203334 | `61e27ca3678d42127e4fbbc33b00b8be26bdafdb21abcc79579a4c1a77446c42` |

The output is the decompressed, byte-exact program/artwork of the private
0.1.73 Sculpt single-column candidate. Its compressed counterpart has SHA-256
`376e42dd7e2cb32a67a7edfd01711e0bf644cc4c9fd2c361d9e718c2152f6819`.
This supersedes the original 0.1.53 recipe; headset layout qualification is pending.
Different container compression is not an image/layout/code change.

Native code pins the **whole patch hash**, so changing a hash inside a patch
cannot authorize a different original or output. Missing/unknown/corrupted
inputs fail before menu parsing. Bounds: 8 MiB each input, patch and output;
200,000 operations; nonzero lengths; exact input consumption and output size;
no overflowing ranges, unknown opcodes, trailing records or compressed data.

## What the patch carries

24,385 operations copy 424,520 output bytes from offsets in the original and
insert only 8,157 literal bytes: **98.11% original-byte reuse**. The longest
literal run is 13 bytes. Every literal run is audited: none contains a
contiguous four-byte sequence found anywhere in the original. Offset/length
instructions and hashes account for most of the patch's file size. No whole recompiled class, movie or graphical asset is
shipped as an INSERT block.

This audit measures byte reuse, not authorship of every short compiler encoding.
The literals include new or changed encodings as well as our changes. The
four-byte threshold avoids copied upstream blocks in literals; it is not a
claim that matching single bytes can or should be eliminated.

## Format and maintainer workflow

All integers are unsigned little-endian. Header: magic `RMSWFP01` (8 bytes),
original file length, canonical source length, canonical target length,
operation count (four uint32s), then three 32-byte SHA-256 hashes in that order.
Header is 120 bytes. COPY: opcode 0, uint32 source offset, uint32 length.
INSERT: opcode 1, uint32 length, that many literal bytes. COPY always references
the immutable canonical original, never a partially assembled output.

`tools/swf-byte-patch.cjs make original.swf accepted.swf new.rmp` generates and
audits a deterministic patch and checks its exact round trip. `audit` checks an
existing patch; `apply` writes a **new**, private FWS file for offline inspection.
Both original and accepted movie remain private maintainer inputs. The existing
JPEXS recipe remains a maintainer build tool, not an installation dependency.
Build a new patch from every accepted movie change; update native/package pins
and retain independent native/reference equivalence evidence. Never publish
temporary decompiled originals or generated full movies with the patch.

## Runtime adapter

Skyrim VR 1.4.15's `BSScaleformFileOpener` slot 3 is OpenFileEx; slot 1 forwards
to it. The adapter accepts only `Interface/VR/RaceSex_menu.swf`, normalizing
ASCII case and slash direction. Other URLs and opener methods are untouched.
It allocates Skyrim's own 0x30-byte GMemoryFile on the GFx heap and calls the
statically qualified engine constructor. The engine retains/release-destroys
each file normally; its backing movie bytes remain resident for the process.

The vanilla opener function and constructor/opener code signatures must match.
The active opener may be vanilla or the qualified SKSE VR 2.0.12
`SKSEFileLoader` wrapper. SKSE's wrapper delegates directly to the vanilla
function, so the adapter hooks the active wrapper's OpenFileEx slot, retaining
its previous function for all unrelated URLs. The wrapper's four vtable entries
and 192-byte loaded-code SHA-256 are checked before installation; unknown or
modified openers are refused rather than overwritten. This supports the
currently qualified SKSE build, not arbitrary future wrapper versions.
The constructor load hook checks that the memory adapter actually served the
movie, refusing an unverified pre-existing cache/fallback. A failure is logged
in RaceMenuNGVR2.log and returns the engine's normal failed-load path. No stale generated
file or partly patched program is exposed as a fallback.

An incompatible original movie now queues one dismissible **OK** message box
per process. It names `Interface/VR/RaceSex_menu.swf`, the required original
RaceMenu SE **0.4.20.0**, and asks the player to disable conflicting loose
VR layout/generated menus and restart Skyrim. A recipe/loader/installation
failure receives a separate generic warning pointing to `RaceMenuNGVR2.log`,
not an incorrect accusation that the original SWF is wrong. Preparation remains
fail-closed, including after dismissing the warning. Fixes require a restart
because the first preparation result is intentionally cached for the process.

The warning is scheduled on SKSE's game-thread task queue after the menu
constructor returns; it never captures a menu/movie pointer or exception.
It uses Skyrim's separate MessageBoxMenu and a null callback, so **OK** does
not run the character-creation finish/name callback. No original file is changed.
If the task interface or message box is unavailable, the detailed log remains
the fallback. Offline tests cover failure classification and message content;
actual headset visibility/dismissal requires live qualification.

Static evidence uses a retained immutable memory dump, not a live debugger
attachment. Offline reconstruction and successful compilation alone do not
establish live qualification; the corrected loading route's acceptance is
recorded below. Full feature and release qualification remain separate.

The first 0.1.54 live test rejected SKSE's normal wrapper before reconstruction,
causing character creation to be skipped. Read-only external inspection
(QUERY_LIMITED_INFORMATION | VM_READ, without debugger attachment, thread
suspension or writes) established that the native opener was unchanged and the
active type-10 opener belonged to `sksevr_1_4_15.dll`, RTTI `SKSEFileLoader`.
0.1.55 / native 0.5.0.65 corrects this specific adapter routing error. Its live
log subsequently confirmed exact-original verification, in-memory reconstruction,
verified FWS delivery and successful VR movie loading; the user reported visual
parity with the independently built menu. This qualifies the corrected loading
route, not every feature or a complete release. The patch payload is unchanged.

## Offline qualification (17 September 2026)

Eight reference tests pass, including deterministic regeneration of the
checked-in patch and exact reconstruction of the privately held accepted movie.
The independent native applicator also produces the exact accepted canonical
output. Both original and accepted files are hash-verified unchanged after the
test. Native malformed-input tests include truncated records/compressed input,
trailing data, incorrect lengths/hashes, unsupported opcodes, copy bounds,
zero lengths, output overflow and a changed release patch. The VR Release
candidate is package 0.1.54, native 0.5.0.64, built from a recorded dirty
development worktree; it is not final immutable release provenance.

Maintainers can include the private equivalence check in the managed build:
`tools/vr2-build.ps1 -SwfQualificationSource original.swf
-SwfQualificationTarget accepted.swf -PromoteNativeDirectory <new-L-path>`.
The receipt records the result, and promotion is refused on a failed check.

For provider verification without extracting assets, the optional maintainer
tool `tools/verify-vr-swf-bsa.py original/RaceMenu.bsa` reads a bounded Skyrim SE
v105 archive and hashes only the required movie in memory. It checks the archive
hash/mtime again before accepting the receipt; compressed members require the
Python `lz4` module. It is not shipped in the player package. Archive layout
was cross-checked against the [fo76utils archive reader](https://github.com/fo76utils/fo76utils/blob/main/libfo76utils/src/ba2file.cpp).

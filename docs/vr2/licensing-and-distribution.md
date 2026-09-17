# Licensing and distribution boundary

## Approved provenance and implementation boundary

The implementation base is Expired's RaceMenu NG source in
`expired6978/SKSE64Plugins`, branch `CommonLibSSE-NG`. That GitHub branch has an
explicit GPL-3.0 license. The older Nexus distribution comes from a different
historical branch whose licensing is not explicit; it is not the source basis
for this implementation.

`THIRD_PARTY_NOTICES.md` identifies `skee64` as GPL-3.0-or-later and records the
statically linked CommonLib dependency. Preserve and update those notices as
dependencies change.

CommonLibSSE-NG, the Skyrim VR Address Library, and SKSEVR may be used for their
published contracts, mappings, and permissively available implementation where
applicable. SKEEVR/RaceMenuVR is a separate work and is not a source-code or
binary dependency: do not copy its code. Equivalent VR functionality must be
implemented locally from the GPL-3 RaceMenu NG base and independently qualified
Skyrim VR interfaces and executable evidence.

## Product relationship

The user-facing VR package is an add-on, not a repackaging of the complete mod.
The GPL source is the NG branch. The tested **asset prerequisite** is the separate
RaceMenu SE 0.4.20.0 Nexus download (19080), not the Skyrim LE download (29624).
This does not assert that the original Nexus assets are GPL. Users receive
them through Expired's original channel.
The compatibility package should contain only project-built VR binaries,
project-owned configuration or documentation, and legally redistributable
notices.

The mod manager should load the compatibility package after original RaceMenu so its
VR DLL replaces the incompatible runtime DLL. Document this override clearly;
do not silently bundle the upstream asset tree.

Do not publish the complete modified menu or decompiled upstream classes.
Distribute our source-pinned transformation recipe instead: it regenerates the
user's private movie locally. The native-only download is not functional without
this step. The recipe reproduces the accepted development movie exactly.
Disable different-filename legacy DLLs; do not use dummy files to mask them.
SteamVR uses separately installed VR Menu Mouse Fix as an explicit dependency;
its code/DLL is not bundled or relicensed. OCU supplies its tested pointer path.

## GPL source delivery

A GitHub repository can satisfy source availability when it exposes the complete
corresponding source for the exact binary, including build scripts and the
material needed to regenerate it. A floating branch link alone is fragile.
For each release, publish all of the following:

- an immutable Git tag and source archive matching the DLL;
- submodule/dependency revisions and acquisition instructions;
- the complete build recipe and required tool versions;
- local patches needed to build the distributed binary;
- `LICENSE`, third-party notices, and copyright attribution; and
- hashes tying the binary to the release record.

Place a conspicuous source link on the Nexus page and in the archive README.
Retain the source for as long as the corresponding binary is distributed. If
Nexus hosts the binary while GitHub hosts source, make the exact tag/archive a
direct, version-specific link rather than relying only on a repository home
page.

This document records the project's conservative compliance policy; it is not
legal advice.

## Contribution licensing

New contributions should use `GPL-3.0-or-later` unless upstream requires a
compatible alternative. Preserve original authorship, add verified co-author
trailers for materially ported GPL work, and never credit a reviewer or tool as
an author.


# Licensing and distribution boundary

## Approved provenance

The project is based exclusively on Expired's
`expired6978/SKSE64Plugins` branch `CommonLibSSE-NG`. Its repository root
contains an explicit GPL-3.0 license. The owner introduced that license in
commit `c7237ada0ee5b4ad344790f579e35aed4103b06f`; the hand-off checkout begins at
`7ceab706e0f4fdd1816f8c61390ff33ecdeda1f8`.

`THIRD_PARTY_NOTICES.md` identifies `skee64` as GPL-3.0-or-later and records the
statically linked CommonLib dependency. Preserve and update those notices as
dependencies change.

The absence of an explicit root license on other historical branches is not a
reason to infer permission from them. Do not copy from the historical `VR`
branch or from Nightfallstorm's RaceMenuVR adaptation.

## Product relationship

The user-facing VR package is an add-on to RaceMenu NG, not a repackaging of the
complete mod. RaceMenu NG remains a prerequisite so its author retains the
download and users receive the original assets through the original channel.
The compatibility package should contain only project-built VR binaries,
project-owned configuration or documentation, and legally redistributable
notices.

The mod manager should load the compatibility package after RaceMenu NG so its
VR DLL replaces the incompatible runtime DLL. Document this override clearly;
do not silently bundle the upstream asset tree.

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


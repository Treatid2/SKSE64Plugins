# RaceMenu VR 2 — prepared native add-on

Read [INSTALLATION.md](INSTALLATION.md) before installing. This download
contains no original RaceMenu assets and **requires local menu generation**.
The recipe is in `AssetPatcher/tools/patch-vr-racesex-swf.ps1`; run its example
from the `AssetPatcher` directory so `./tools/...` resolves correctly.

Download RaceMenu SE 0.4.20.0 assets from Expired:
https://www.nexusmods.com/skyrimspecialedition/mods/19080

SteamVR additionally requires VR Menu Mouse Fix and its prerequisites:
https://www.nexusmods.com/skyrimspecialedition/mods/33414

The tested OCU wand route uses OCU's own pointer system.

Treatid2 source fork: https://github.com/Treatid2/SKSE64Plugins

This candidate's DLL is the retained 0.1.53 development binary (native 0.5.0.63),
SHA-256 `3CB289C2338C8397038272BDAC9F96A4CB783652A4B2DDE1CF18A95939F2263E`.
It was built from an uncommitted development worktree. This candidate is for
release preparation, **not a publicly tagged GPL binary release**. Replace
this paragraph with the final clean-build tag/archive/hash before publication.

Our native source/recipes inherit the upstream GPL licence. Original assets
and user PNG artwork are not relicensed. See LICENSE and THIRD_PARTY_NOTICES.md.
Primary credit belongs to Expired6978. The intended Donation Points recipient
is Expired, pending confirmation of the actual Nexus allocation.

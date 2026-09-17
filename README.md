# RaceMenu VR 2 — add-on

A Skyrim VR adaptation of Expired6978's GPL RaceMenu NG source. Maintained by
Treatid2. **Requires separately downloaded original RaceMenu assets.** This is
not an official Expired release and is not a standalone RaceMenu distribution.

Features include aligned VR pointer interaction, Filter and Name keyboards,
accepted-name persistence, steady face view, player-relative menu/picker
placement, readable consolidated controls, aspect-preserving PNG backgrounds,
INI colours, and versioned native character-creation/menu-extension interfaces.

See [installation](docs/release/INSTALLATION.md),
[configuration](docs/menu-customization.md), and
[release status](docs/release/RELEASE-CHECKLIST.md).

## Requirements

- Skyrim VR 1.4.15.0, matching SKSEVR and Skyrim VR Address Library.
- [RaceMenu SE](https://www.nexusmods.com/skyrimspecialedition/mods/19080)
  **0.4.20.0 assets**, downloaded from Expired's page. No old RaceMenu VR DLL.
- SkyUI VR and prerequisites of the installed original asset package.
- **SteamVR only:** [VR Menu Mouse Fix](https://www.nexusmods.com/skyrimspecialedition/mods/33414)
  and its requirements. Enable its laser pointer. OCU's tested native wand path
  does not require this helper. Neither pointer provider is bundled here.
- Local menu preparation: JPEXS Free Flash Decompiler **26.2.1**, plus a BSA
  extraction tool if the original movie is inside RaceMenu.bsa. These are
  preparation tools, not runtime dependencies.

The original [Skyrim LE RaceMenu page](https://www.nexusmods.com/skyrim/mods/29624)
is credited, but its incompatible files are **not** an install prerequisite.

## Source and contributions

Fork: https://github.com/Treatid2/SKSE64Plugins

Upstream: https://github.com/expired6978/SKSE64Plugins/tree/CommonLibSSE-NG

Native source and our patch recipes inherit the upstream GPL licence; preserve
`LICENSE` and `THIRD_PARTY_NOTICES.md`. Original Nexus assets and user artwork
are not relicensed or redistributed. Build instructions are in `BUILDING.md`.
Every published binary must have an exact corresponding-source tag/archive;
the development worktree binary is not yet a tagged public release.

Expired6978 deserves primary credit for RaceMenu and its original assets.
See the original mod's credits for its contributors. CommonLibSSE-NG, SKSEVR,
SkyUI VR, Address Library, OCU and VR Menu Mouse Fix enable this adaptation.
The intended Nexus Donation Points recipient is Expired; the page owner must
confirm the allocation in Nexus before publication.

# RaceMenu VR 2 — VR beta 0.1.60

Read [INSTALLATION.md](INSTALLATION.md) before installing. This download
contains no original RaceMenu assets. The exact original menu is verified and
transformed automatically in memory. No manual menu generation is needed.
The matching DLL and `RaceMenuVR2/racesex-menu.rmp` must be installed together;
disable other loose VR menu SWFs and legacy `skeevr.dll`.

Download RaceMenu SE 0.4.20.0 assets from Expired:
https://www.nexusmods.com/skyrimspecialedition/mods/19080

SteamVR additionally requires VR Menu Mouse Fix and its prerequisites:
https://www.nexusmods.com/skyrimspecialedition/mods/33414

The tested OCU wand route uses OCU's own pointer system.

Treatid2 source fork: https://github.com/Treatid2/SKSE64Plugins

Corresponding source (including build scripts, dependency pins and patches):
https://github.com/Treatid2/SKSE64Plugins/tree/racemenu-vr2-v0.1.60
Release downloads and SHA-256 identities:
https://github.com/Treatid2/SKSE64Plugins/releases/tag/racemenu-vr2-v0.1.60

This update adds a large installation notice when the original SWF is wrong,
with no notice for the correct original. Independent main/picker height offsets
are documented in the INI: default menu centre is on the captured player eye
line; adjustments are relative to it. The user tested extreme offsets and
confirmed wand alignment remained accurate. Both new features are live-tested.

The in-memory menu route has been tested live. Face View and menu reopening
work. Camera-tab options respond but have no confirmed visible camera movement;
use Face View instead. This is a VR beta, not an SE/AE replacement. Testing is
not exhaustive across headsets, bindings, presets, sculpt tools or extensions.

Our native source/recipes inherit the upstream GPL licence. Original assets
and user PNG artwork are not relicensed. See LICENSE and THIRD_PARTY_NOTICES.md.
Primary credit belongs to Expired6978. The intended Donation Points recipient
is Expired, pending confirmation of the actual Nexus allocation.

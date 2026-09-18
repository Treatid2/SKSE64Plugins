# RaceMenu VR 2 — VR beta 0.1.79

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
https://github.com/Treatid2/SKSE64Plugins/tree/racemenu-vr2-v0.1.79
Release downloads and SHA-256 identities:
https://github.com/Treatid2/SKSE64Plugins/releases/tag/racemenu-vr2-v0.1.79

This update adds a VR Sculpt workspace: controls, History and head parts in
one left column, a right-hand sculpt canvas, independent Face View and a larger
canvas toggle. Turn left/Centre view/Turn right adjust viewing direction in
5-degree steps, limited to +/-60 degrees, without moving the menu. Rotation
persists through Face/Normal view changes. Buttons and transitions are
headset-tested. See RELEASE-NOTES-0.1.79.md for changes and test boundaries.

Sculpt deformation, History undo/redo, head export/clear/import and preset
save/change/load have passed human tests during this development cycle.
Leaving Sculpt resets its brush, History and part selection after the stock
warning; cross-session sculpt persistence has not been qualified.

The INI defaults to upright menus; bForceVertical=0 opts into vertical tilt
toward the captured viewpoint. Height offsets remain relative to captured eye
line. fSculptAzimuth independently positions the wider Sculpt workspace.

The in-memory menu route has been tested live. Face View and menu reopening
work. The non-VR Camera tab is removed in VR; use Face View instead. This is
a VR beta, not an SE/AE replacement. Testing is not exhaustive across headsets,
bindings, sculpt tools or extensions; stable observed performance is not a
measured performance qualification. Preserve your custom INI on update.

Our native source/recipes inherit the upstream GPL licence. Original assets
and user PNG artwork are not relicensed. See LICENSE and THIRD_PARTY_NOTICES.md.
Primary credit belongs to Expired6978. The intended Donation Points recipient
is Expired, pending confirmation of the actual Nexus allocation.

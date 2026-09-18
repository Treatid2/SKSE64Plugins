# RaceMenu VR 2 — VR beta 0.1.80

**An add-on to Expired's RaceMenu, adapted for Skyrim VR.**

Please download the original RaceMenu SE asset package from Expired's page
first. This project builds from his GPL RaceMenu NG branch; it does not
redistribute the original Nexus assets. Treatid2 maintains this VR fork.

## What it adds

- Correct VR inventory-preview layout and bounded tint lookup, addressing an
  identified inventory armour-preview crash. Character creation and varied
  armour/clothing previews passed the development headset test. This does not
  establish that every reported random crash has been resolved.
- Working Filter and Name virtual keyboards, with accepted names surviving
  character-creation exit and save/load. No second naming prompt on exit.
- Stable face view and a steady, player-facing menu that reanchors when you
  change between normal and face views.
- Independently positioned colour picker; angular placement, height, distance and
  size in the INI. Controls and pointer coordinates stay aligned.
- Readable footer controls, cap-aligned translucent slider tracks and reduced
  projected moire.
- Optional PNG backgrounds that preserve their image aspect ratio, plus
  configurable background/text/accent colours and opacity.
- Documented versioned native interfaces for other developers to extend.
- A large on-screen installation notice if an incompatible menu overrides the
  required original; no notice with the correct original.
- A VR Sculpt workspace with brush, History and head-part controls on the
  left, a sculpt canvas on the right, Face View and a larger canvas toggle.
- Working Sculpt stroke history/undo/redo and repaired head export/import.
- Turn left, Centre view and Turn right: 5-degree viewing-direction steps,
  limited to +/-60 degrees, leaving the menu fixed. Rotation persists through
  Face/Normal view changes. These controls do not rotate the sculpt mesh.
- Upright menus by default, opt-in vertical tilt and independent Sculpt angle
  in the INI. The redundant non-VR Camera tab is removed in VR.

## Requirements and installation

Skyrim VR 1.4.15.0, SKSEVR, Skyrim VR Address Library, SkyUI VR and the original
[RaceMenu SE 0.4.20.0](https://www.nexusmods.com/skyrimspecialedition/mods/19080)
assets. **SteamVR additionally requires
[VR Menu Mouse Fix](https://www.nexusmods.com/skyrimspecialedition/mods/33414)
and its dependencies.** OCU uses its own tested wand system.

Install after RaceMenu. Our `skee64.dll` must overwrite the incompatible SE
DLL. Disable any separately installed legacy `skeevr.dll`; it is not covered
by that overwrite. Do not install the older Skyrim LE package as a dependency.

The add-on patches the exact original menu **automatically in memory**.
Players need no extraction, JPEXS or manual preparation. Original files remain
unchanged. Read `INSTALLATION.md`: the matching DLL and small runtime patch
must be installed together, without a conflicting loose VR menu SWF.
The original-BSA loading route has been tested live without a generated loose SWF.

## Credits, source and permissions

Primary credit: **Expired6978**, creator of RaceMenu and its original assets.
Please support him and download the originals from his pages:
[RaceMenu SE](https://www.nexusmods.com/skyrimspecialedition/mods/19080) and
[original Skyrim RaceMenu](https://www.nexusmods.com/skyrim/mods/29624).
The original page's contributor credits remain applicable to its assets.
Thanks to CommonLibSSE-NG, SKSEVR, Address Library, SkyUI VR, OCU and the
VR Menu Mouse Fix authors. Their projects are separate and not bundled here.

Our native code/patch recipes are GPL, inherited from the upstream repository.
That licence does not cover the separately downloaded Nexus assets or custom
user artwork. This is not an official release or implied endorsement by Expired.

Source fork: https://github.com/Treatid2/SKSE64Plugins

Corresponding source:
https://github.com/Treatid2/SKSE64Plugins/tree/racemenu-vr2-v0.1.80
Versioned downloads and SHA-256 identities:
https://github.com/Treatid2/SKSE64Plugins/releases/tag/racemenu-vr2-v0.1.80

## Qualification and limitations

The development headset test confirmed the menu layout, image proportions,
normal/face transitions, picker placement, Filter/Name entry and character-name
save/load. The user found SteamVR + VR Menu Mouse Fix workable. In 0.1.60
the user also confirmed the wrong-SWF notice,
absence of that notice with the correct SWF, and accurate wand alignment with
extreme INI height offsets. Default height aligns the menu centre to captured
player eye level; height offsets are relative to that, not absolute world heights.
During this development cycle the user confirmed visible Sculpt deformation,
History population, snapshot undo/redo, head export/clear/import and preset
save/change/load. Hover and strokes showed no obvious performance collapse;
this is an observation, not a measured FPS qualification. In 0.1.79 the user
confirmed working rotation controls and persistence through Face/Normal view.
Leaving Sculpt resets brush, History and part selection following the stock
warning. Cross-session sculpt persistence remains unqualified. Brush/Property
and Primary/Secondary are stock control hints, not additional mouse buttons.

This is not exhaustive coverage of all headsets, controller bindings, sculpt
tools, third-party extensions or SE/AE standalone compatibility. Back up saves
and preserve/merge your custom INI on update. SteamVR Sculpt is not separately
qualified to the same extent as the OCU development session.

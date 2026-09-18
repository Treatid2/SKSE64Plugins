# RaceMenu VR 2 — 0.1.79 VR beta

Native plugin: 0.5.0.89. Install the matching DLL and runtime patch together.
Requires original RaceMenu SE 0.4.20.0 assets. SteamVR additionally requires
VR Menu Mouse Fix and its prerequisites; neither dependency is bundled.

## Changes since the previous Nexus download

- Upright menus by default; bForceVertical=0 opts into vertical facing tilt.
  Height remains relative to the captured player eye line. Expanded INI
  coordinate documentation and independent fSculptAzimuth placement.
- Repaired Sculpt pointer/stroke lifecycle and CPU vertex access. Hover no
  longer reads write-combined GPU buffer mappings. Added optional bounded
  diagnostics without permanent per-pointer logging.
- Repaired Sculpt History notifications, snapshot undo/redo and head export
  skin cloning. Head export/clear/import and preset save/change/load were
  successfully exercised by the development tester.
- Removed the redundant Camera tab in VR while preserving Sculpt's legacy
  semantic mode ID and both native/ordinary tab activation paths.
- Wider Sculpt workspace: proportional brush controls, History and head parts
  in one left column; sculpt canvas on the right. Independent Face View and
  larger canvas controls remain reachable in both sizes.
- Turn left, Centre view and Turn right adjust the player viewing direction
  in 5-degree steps within +/-60 degrees. The menu stays fixed and real head
  tracking remains active. The selected angle survives Face/Normal changes;
  closing RaceMenu restores the owned rotation. These are viewpoint controls,
  not sculpt-mesh rotation controls.
- Late button capacity allocation fixes rotation controls missing from Sculpt.

## Testing and limitations

Human OCU testing confirmed visible deformation, History population and
undo/redo, head export/clear/import, preset save/change/load, canvas toggling,
Face View, accurate pointer interaction and working rotation controls.
Rotation persistence across Face/Normal changes was confirmed in 0.1.79.
Observed performance was stable, not a controlled timing/FPS measurement.
SteamVR's helper was found workable earlier; this Sculpt cycle does not qualify
all SteamVR controllers or bindings. SE/AE standalone behavior, cross-session
sculpt persistence and every third-party extension remain outside this test.

Leaving Sculpt resets brush, History and head-part selection after the stock
warning. Primary/Secondary and Brush/Property are stock input hints, not extra
clickable settings. Back up saves and preserve/merge an existing custom INI.

## Source and provenance

Corresponding source and pinned build instructions:
https://github.com/Treatid2/SKSE64Plugins/tree/racemenu-vr2-v0.1.79
Release downloads and hashes:
https://github.com/Treatid2/SKSE64Plugins/releases/tag/racemenu-vr2-v0.1.79

The public player package retains the exact headset-tested 0.1.79 DLL and RMP;
only documentation was refreshed after that candidate was built and tested.
The build receipt records its original development-worktree provenance.
Original/reconstructed SWFs, upstream Nexus assets and private Prisma bridge
are not included. Native code and project recipes inherit GPL-3.0-or-later.
Original assets and user artwork are not relicensed. Primary credit: Expired6978.

# Menu customization implementation / qualification

The 0.1.50 development package adds the following to the VR branch. This is
build- and surrogate-tested, not a qualified SE/AE/VR standalone release.

## Configuration

`[Menu Appearance]` is the shared default. `[Menu Profile VR Normal]` and
`[Menu Profile VR Face]` override individual supplied values. Empty values keep
shared defaults/original artwork. `Flat` reserves the original flat layout;
the flat movie and flat engine camera are not replaced by this VR patch.

RGB keys: `sBackgroundColor`, `sTextColor`, `sAccentColor`, `sSelectionColor`,
`sBorderColor`, each `RRGGBB`/`#RRGGBB`. Opacity keys (0..100):
`fBackgroundOpacity`, `fTextOpacity`, `fImageOpacity`, `fAccentOpacity`,
`fSelectionOpacity`, `fBorderOpacity`. All are optional; defaults retain the
existing black, neutral text, green active indicators and original alpha.

VR profile layout keys: `bConsolidatePanel` (default 1), `fPanelScale` (percent),
`fPanelX`, `fPanelY` (movie coordinates), `fColorPickerScale` (percent).
Consolidation moves the existing registered mode/action controls into the
left panel, wraps actions, hides the extended bottom bar and moves race info.
It changes `listHeight`, not the slider clips' scale or track/cap geometry.
Setting consolidation to 0 retains the original framing.

0.1.51 separates the four mode tabs into a clear strip above the panel,
reserves more lower action space and moves the race description clear of it.
World placement in `[Menu Profile VR Normal]` uses `fAzimuth` (degrees, positive left,
-85..85), `fElevation` (-60..60), `fDistance` (30..300 world units), and
`fSurfaceScale` (25..300 percent). Defaults are 30, 0, 100, 100. The corresponding
independent modal keys are `fColorPickerAzimuth`, `fColorPickerElevation`,
`fColorPickerDistance`, `fColorPickerSurfaceScale`, default 38, 0, 100, 100.
Angles are relative to the initial avatar-facing direction, not a yaw applied
to artwork. The actual surface is oriented toward the viewer and translated
so the visible panel/modal centre is at the requested radius and direction.
The registered ray parent and visible geometry receive the same world-space
transform, without double-transforming descendants. These new defaults also
apply when a preserved older INI lacks the keys. Existing movie X/Y/scale keys
remain available but the polar keys are the recommended world placement.
0.1.52 shares these placement settings between normal and face views. The
viewer anchor is captured on menu opening and each normal/face transition,
not every 500 ms or on physical headset movement. Opening/closing the picker
switches the modal/main transform around that same fixed viewer anchor.
Old face-profile placement keys may remain in preserved INIs, but are ignored;
face styling and camera-distance/eye-height overrides remain supported.
`fHeightOffset` and `fColorPickerHeightOffset` in VR Normal add independent
vertical offsets after polar positioning (-150..150 Skyrim world units,
positive up). Both default to zero, including in preserved older INIs.
Empty values inherit Menu Appearance; invalid/out-of-range values use zero.
The main height does not implicitly offset the picker. The resulting surface
still faces the captured viewer and moves its wand interaction geometry with
it. These settings are shared with Face view and require a game restart.
`bForceVertical` in `[Menu Profile VR Normal]` keeps the surface upright by
default: `1` locks pitch to zero; `0` opts into viewer-facing tilt.
It applies to both the main menu and colour picker in Normal and Face views.
Elevation and height still move the centre, and horizontal facing toward the
captured viewer is retained. The matching wand plane uses the same orientation.
Missing or invalid values use upright. Opt into `0` at extreme elevations
(e.g. +/-60 degrees), where an upright surface is harder to see. This option
does not introduce head-following movement; restart Skyrim after changing it.
Category selection and dynamically wrapped action buttons do not reanchor the
surface. In 0.1.53 the stock double-rule decoration is removed from the VR
movie (a named directly placed shape could not be moved as an AS2 MovieClip).
The consolidated race display omits the redundant `Race:` label and sits on
a padded row below the actions. Footer text is 25px, matching the visible
height of the 24px slider labels under their stock list scaling. Active actions
wrap at 40px row spacing; inactive renderer slots consume no space. The list
reserves at least 160px for this footer, growing for additional action rows.
The 0.1.51 user test reports steady face framing and correctly aligned moved
controls. The 0.1.52 user test confirms steady menu placement, correct movement
on face-view transitions, excellent steady face framing and good picker
placement. The 0.1.53 user test reports the resulting menu is perfect and a
solid base, qualifying the footer/decorative cleanup in this headset session.
This does not replace the separate standalone SE/AE or native-consumer checks.

Category presentation uses `[Menu Category <decimal category flag>]`:
`sLabel`, `bVisible`, `iOrder`. It never disables underlying body/head features.
Native extensions also use `[Menu Provider <provider ID> Section <section ID>]`
with the same keys and optional `sControlIds` (comma-separated exact control
IDs, no whitespace). Legacy providers have no stable provider IDs in their
stock category callback, so only their numeric category flags are addressable.

## Native APIs

`CharacterCreation` interface version 3 appends to the complete v1/v2 vtable
prefix. Consumers must check `GetVersion()` and `GetCapabilities()` first.
Adds caller-buffer accepted-name/filter snapshots, `SetFilter`, normal/face
view query/selection, and state/name/filter/view notifications. Strings reflect
accepted model values, never keyboard drafts; publication is every 500 ms and
on direct name/filter updates. Queued operations may be abandoned if the menu
closes or changes session. Do not issue filter updates during a keyboard edit.
Listeners run outside service locks on the publishing/game thread. Consumers
must unregister and synchronize any in-flight callback before unloading code.

The separately named `MenuExtensions` interface version 1 copies stable
provider/section/control IDs and labels. It registers numeric slider controls
with bounds/steps/current values and game-task callbacks, updates their values,
and unregisters providers. Registry bounds: 32 sections, 128 sliders; IDs 48
ASCII bytes, labels 128 bytes. Each provider reserves its own single category
bit >= 1<<20. The movie rejects collisions with legacy categories rather than
silently merging them. New controls use the existing slider renderer and
change path, without sending a nonexistent engine callback or simulating clicks.
Race/sex rebuilds restore extension presentation; saved-game reversion does
not erase registrations belonging to still-loaded plugins. Other control types
(text, buttons, texture pickers) are not part of this first interface version.

## Face view

VR's ordinary Zoom action becomes Face view / Normal view. `bEnableFaceView=0`
in `[VR]` restores the stock Zoom path. `fFaceDistance` in `[Menu Profile VR Face]`
accepts 25..150 Skyrim world units, default 45.

The game task resolves the current avatar head and an independent RoomNode/HMD
tracking hierarchy. It refuses to translate an origin that contains the avatar
head. It translates the tracking origin, never the actor or headset rotation.
The normal-view anchor is fixed, so real translational HMD tracking is not
cancelled on subsequent pulses. Head positions are re-read for race/height/
sculpt changes. Only our additive translation is removed on return/closure;
an origin replaced by another owner is not overwritten. Unsupported topology
returns an unavailable state, with visible feedback, rather than pretending
that a changed button label proves camera movement.

0.1.51 uses a level horizontal viewing direction and `fFaceEyeHeight` (-20..30,
default 5 units above the head bone). It anchors the target instead of following
idle head animation every 500 ms; head identity, race and avatar scale changes
refresh that anchor. The existing head-view pulse does not cancel real HMD
tracking. `GetFaceViewDiagnostics` exposes cumulative update, anchor-refresh
and competing-origin-replacement counters. This addresses one plausible
jitter source; it does not prove or disable physics/collision. Live testing
must distinguish animation-following jitter from an engine ownership conflict.

This path requires live stereoscopic, pointer/keyboard, tracking-origin
ownership and comfort qualification. No forced rotation or animated camera sweep
is performed. A whole-view fade is not yet implemented.

## Remaining qualification / wishlist work

### VR Sculpt workspace (0.1.73 candidate)

Sculpt puts the canvas, Brush controls, History, Head Parts and action buttons
in its own wider workspace on the menu side. The canvas is on the right,
beside one left-hand column: Brush, History, then Head Parts. Each section is
uniformly fitted to 280 movie units wide and at most 260, 160 and 180 units
high respectively. This keeps their proportions and makes room for the complete
canvas and reachable actions below both columns. `fSculptAzimuth` in VR Normal independently positions Sculpt
(default 48 degrees left, -85..85, positive left); omitted/empty keys use 48.
Height, elevation, distance and surface scale use the main VR menu settings;
the menu's world anchor changes with a view transition, not each head movement.
Brush slider rails retain the brush artwork's shorter endpoint independently
of the regular character sliders. VR exposes Sliders, Presets and Sculpt tabs;
the stock Camera tab is omitted in VR only.

Sculpt's **Face view / Normal view** button remembers its own view for this
menu session and restores the preceding view when leaving Sculpt. **Large
canvas / Standard canvas** grows the canvas from 560 to 760 movie units, retaining
a 280-unit controls column. Brush's registered slider list is uniformly doubled
within that column, including text and hit geometry, rather than stretching
only artwork. Original transforms are captured once so rebuilds/toggles do
not compound the enlargement. This matches the live size the tester reports
as much easier to click and grab. The brush category selector is doubled too;
its fixed visible mask, rather than hidden sliding labels, determines the fit.
The height limit uses nested two-operand AS2 comparisons so Large canvas
cannot discard the footer's vertical space. The reflow needs a headset check.
The complete backing frame, content and actions
are uniformly fitted inside both dimensions of the movie's drawable area,
independently of the hidden Sliders panel. Uniform fitting can reduce the whole
workspace slightly in Large canvas mode. Resize is ignored during active
painting, rotation or panning. Added text-only action buttons retain visible
low-alpha backgrounds for ordinary GFx hit testing; their explicit click counts
are available as `vrFaceActionCount` and `vrCanvasActionCount` on VertexEditor.
The new actions are registered in the complete static action panel (the same
group as Import/Export/Clear), but visually stay beside the top-row hints.
This supports OCU's semantic targeting, which scans only Done in the hint row.
Owned listeners are refreshed without replacing stock actions or duplicating
callbacks when the bottom bar is rebuilt.
These candidate layout and physical-click behaviours require a headset check
before publication.

The existing canvas pan/rotation and mesh zoom remain; this is not a separate
movable magnifier or a higher-resolution texture. The native sculpt texture
remains 1024 by 1024, and hit/paint coordinates remain canvas-local after scale.

- Live 0.1.49 physical PNG metric/aspect verification, separately from layout.
- Live 0.1.50 normal/face layout, modal placement, keyboard regressions, head
  tracking and restoration; do not infer success from the native/AS builds.
- A native consumer smoke test for callback lifetimes and section registration.
- Flat appearance/profile customization, SE implementation/audit and SE/AE
  regression runs. Current CMake still explicitly offers AE/VR, with SE disabled.
- Optional calligraphic race flourish and whole-view transition fade.

The former character-creation lifecycle investigation harness is absent from
current source/config/build packaging; normal API readiness probes remain.
The later default-off bounded input-observation API is separate and retained
for live qualification. Its presence is not evidence that the old lifecycle
harness has been reintroduced.


[Menu Profile Flat]
; Flat layout retains original framing and engine camera behavior.
bConsolidatePanel=0

[Menu Profile VR Normal]
; Existing registered controls, moved into the left panel. 0 restores framing.
bConsolidatePanel=1
; Empty overrides inherit [Menu Appearance] or retain original artwork.
; Restart Skyrim after changing INI settings. For update-safe personal overrides,
; use these same sections/keys in SKSE/Plugins/skee64_custom.ini.
;
; TWO COORDINATE SYSTEMS: use the polar controls below for normal positioning.
; They move/orient the actual VR surface AND the matching wand interaction plane.
; The older PanelX/Y/Scale controls edit artwork inside that surface instead.
;
; Optional 2D artwork overrides (logical movie coordinates, NOT PNG pixels).
; fPanelX/Y are absolute panel-origin positions, not offsets: X increases right,
; Y increases down on the movie's 1024x1024 logical canvas. Range -2048..2048.
; Empty keeps the stock origin; 0 explicitly places that origin at coordinate 0.
; fPanelScale: 25..300 percent; 100 keeps the stock 2D panel size.
; fColorPickerScale: 50..400 percent of the picker artwork; empty keeps its
; current enlarged 225-percent movie default. 100 would make it smaller.
; These local artwork controls are separate from fSurfaceScale below.
fPanelScale=
fPanelX=
fPanelY=
fColorPickerScale=
;
; POLAR WORLD PLACEMENT: describes the visible menu centre around the viewer.
; Reference direction is the initial horizontal viewer-to-avatar heading when
; RaceMenu opens, not the direction you happen to turn your head afterwards.
; The viewer position is captured on opening and each Normal/Face view change.
; Between those changes the menu stays steady in world space, not head-locked.
; The surface faces that captured viewer position automatically; azimuth moves
; the menu around the viewer, rather than twisting its artwork on a fixed plane.
;
; fAzimuth: degrees, -85..85. 0 = ahead; positive = LEFT; negative = RIGHT.
; Example: 30 places the centre 30 degrees left of the starting avatar heading.
fAzimuth=30
; fElevation: degrees, -60..60. 0 = captured viewer/eye height; positive = UP,
; negative = DOWN. This is an angular position, not a vertical world-unit offset.
fElevation=0
; fDistance: radius from the captured viewer to the menu centre, 30..300 Skyrim
; world units. Larger = farther/smaller-looking; smaller = nearer/larger-looking.
; Radius is not simply forward depth: at nonzero angles it also determines the
; sideways/vertical displacement. It is not pixels, metres or face-camera distance.
fDistance=100
; fSurfaceScale: uniform VR surface size, 25..300 percent. 100 = base size;
; 120 = 20 percent larger at the SAME centre/distance. PNG proportions stay intact.
fSurfaceScale=100
;
; Independent colour-picker placement uses the SAME heading, viewer anchor,
; units, sign convention and ranges. Opening/closing it switches between these
; settings and the main menu settings without following physical head movement.
; 38 puts it farther left than the main menu's 30-degree setting.
fColorPickerAzimuth=38
fColorPickerElevation=0
fColorPickerDistance=100
fColorPickerSurfaceScale=100

[Menu Profile VR Face]
; World placement is shared with VR Normal; no separate face placement keys.
; Main and picker polar keys belong in VR Normal. Old copies of those eight keys
; under VR Face are ignored. The menu reanchors once when switching views, using
; the SAME relative placement; it does not follow the headset continuously.
bConsolidatePanel=1
; FACE CAMERA (not menu coordinates): fFaceDistance sets viewing separation from
; the avatar head, 25..150 Skyrim world units. Smaller = closer face framing.
; View translation only, no forced rotation; physical headset tracking is retained.
fFaceDistance=45
; fFaceEyeHeight: vertical target offset above the head bone, -20..30 world units.
; Positive raises framing toward eye level; negative lowers it. Not angular elevation.
fFaceEyeHeight=5
; Optional face-view 2D artwork overrides: same meanings as VR Normal above.
; Empty retains shared/default artwork; these do not define a second VR position.
fPanelScale=
fPanelX=
fPanelY=
fColorPickerScale=
; Example stable engine category: Body=4. Presentation only, not functionality.
; [Menu Category 4]
; sLabel=Body
; bVisible=1
; iOrder=-950
;
; New native extensions also support stable provider + section IDs.
; [Menu Provider example.mod Section appearance]
; sLabel=Appearance
; bVisible=1
; iOrder=-925
; sControlIds=height,width

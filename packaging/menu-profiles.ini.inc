
[Menu Profile Flat]
; Original framing / engine camera behavior.
bConsolidatePanel=0


[Menu Profile VR Normal]
; 1 = move controls into left panel; 0 = restore original framing.
bConsolidatePanel=1
; Restart Skyrim after INI changes. Personal overrides: SKSE/Plugins/skee64_custom.ini.

; --- 2D artwork overrides (movie coords, NOT PNG pixels; separate from fSurfaceScale below) ---
; fPanelX/Y: absolute origin (not offset), range -2048..2048. Empty = stock origin.
; On the 1024x1024 logical canvas, X increases right and Y increases down.
; fPanelScale: 25-300%, 100 = stock size.
; fColorPickerScale: 50-400% of picker art; empty = current 225% default.
fPanelScale=
fPanelX=
fPanelY=
fColorPickerScale=

; --- Polar world placement: main menu ---
; Reference heading = viewer-to-avatar direction when RaceMenu opens, recaptured on each
; Normal/Face switch. Menu stays fixed in world space between switches (not head-locked).
; Surface faces the captured viewer; wand interaction moves with it.
; bForceVertical: 1 = upright (default), no tilt from elevation/height; 0 = tilt toward viewer.
; Applies to main menu and color picker in both views. Horizontal facing is always retained.
; Opt into 0 for extreme elevations (e.g. +/-60 deg), where tilt makes the menu easier to see.
bForceVertical=1
; fAzimuth: -85..85 deg. 0=ahead, +=left, -=right.
fAzimuth=30
; fElevation: -60..60 deg. 0=captured eye height, +=up, -=down (angular, not world units).
fElevation=0
; fHeightOffset: -150..150 world units. Added after elevation; +=up, -=down.
; At elevation 0, default 0 aligns menu center to the player's captured eye line.
; Adjustments are relative to that baseline, not absolute height above ground.
fHeightOffset=0
; fDistance: 30..300 world units. Radius from viewer to menu center (also affects offset at angles).
fDistance=100
; fSurfaceScale: 25..300%. Uniform VR surface size at same center/distance; PNG proportions kept.
fSurfaceScale=100

; --- Sculpt workspace ---
; Wider workspace: controls/history/head parts left, sculpt canvas right beside avatar.
; Independent horizontal angle, same reference heading/range/sign as fAzimuth.
; Empty or omitted = 48 degrees LEFT. Other placement settings stay shared with main menu.
fSculptAzimuth=48

; --- Polar world placement: color picker (same units/conventions, independent of main menu) ---
fColorPickerAzimuth=38
fColorPickerElevation=0
; At elevation 0, default 0 aligns picker center to captured player eye line; independent height.
fColorPickerHeightOffset=0
fColorPickerDistance=100
fColorPickerSurfaceScale=100


[Menu Profile VR Face]
; Shares world placement with VR Normal (placement keys live there only).
; Reanchors once on view switch, using the same relative placement; not head-locked.
bConsolidatePanel=1
; fFaceDistance: 25..150 world units. Viewing separation from avatar head; smaller = closer.
fFaceDistance=45
; fFaceEyeHeight: -20..30 world units. Vertical offset above head bone; +up/-down (not angular).
fFaceEyeHeight=5
; Optional face-view artwork overrides (same meaning as VR Normal above). Empty = shared/default.
fPanelScale=
fPanelX=
fPanelY=
fColorPickerScale=


; --- Reference examples (inactive) ---
; [Menu Category 4]
; sLabel=Body
; bVisible=1
; iOrder=-950
;
; [Menu Provider example.mod Section appearance]
; sLabel=Appearance
; bVisible=1
; iOrder=-925
; sControlIds=height,width

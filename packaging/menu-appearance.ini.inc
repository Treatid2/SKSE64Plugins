
[Menu Appearance]
; RGB hexadecimal (#RRGGBB or RRGGBB). Empty preserves the existing appearance.
; These defaults retain the black background, white/grey text and green active tabs.
sBackgroundColor=
sTextColor=
sAccentColor=
sSelectionColor=
sBorderColor=
; Opacity percentage 0..100. Empty preserves original artwork alpha.
fBackgroundOpacity=
fTextOpacity=
fImageOpacity=
fAccentOpacity=
fSelectionOpacity=
fBorderOpacity=
; Optional PNG: Interface/RaceMenu/racemenu.png. Restart after image/INI changes.
; In MO2 place it at Interface/RaceMenu/racemenu.png inside an enabled mod.
; Current consolidated VR background fill ratio, WIDTH:HEIGHT = 0.79646:1.
; A 1593x2000 PNG is a close match; 4:5 (1600x2000) is nearly the same.
; This covers the main black panel only, not the tabs above or avatar framing bars.
; Derived from the current 427.3903x953.9815 movie-coordinate panel and measured
; 16:9 VR surface X/Y factor: (427.3903 / 953.9815) * (16 / 9) = 0.79646.
; It is NOT the unscaled source-artwork ratio or the apparent ratio in an angled
; screenshot. Rotation, distance and uniform scaling do not change this ratio.
; Other image ratios are accepted: the complete image is centred/contained,
; never stretched or cropped; uncovered space uses the background colour.
; Do not pre-distort the PNG. A different layout/projection may need a new fill ratio.
; Limits: 16 MiB encoded, maximum 4096 pixels per dimension. Alpha is preserved.

# VR menu background and text

Place your optional PNG at `Data/Interface/RaceMenu/racemenu.png` (for MO2,
`Interface/RaceMenu/racemenu.png` inside a mod). Restart Skyrim after changing it.
The image is centred and fitted inside the main panel without stretching or
cropping, compensating for the menu artwork's unequal X/Y scaling and the
measured VR surface projection. Uncovered
space uses the background colour. PNG transparency is supported. The avatar
framing bars are not covered. An image of any aspect ratio is accepted. The
current consolidated panel is **0.79646:1 width:height**, nearly **4:5**, in
physical VR space. A **1593x2000 PNG** closely fills it. The stock source bounds
are 562.45x754.4; the background's X/Y artwork scales produce approximately
427.3903x1023.9815 movie coordinates. Consolidation removes a 70-unit top strip,
leaving 427.3903x953.9815. The recorded VR projection X/Y factor is 16:9:
`(427.3903 / 953.9815) * (16 / 9) = 0.79646`.
This includes the main panel's lower footer but excludes the mode tabs above
and the avatar framing bars. It is not the apparent ratio in an angled
screenshot. Rotation, distance and uniform scaling preserve this ratio; a
different layout or surface projection may change the fill target. Do not
pre-distort your PNG. Other aspect ratios remain supported without stretching.

Add this section to `Data/SKSE/Plugins/skee64_custom.ini`:

```ini
[Menu Appearance]
; Empty/missing values preserve the existing black background and neutral text.
sBackgroundColor=
sTextColor=
; Examples (RGB hexadecimal, with or without #):
; sBackgroundColor=#182030
; sTextColor=#E8D8B8
```

Colour overrides retain the existing background opacity. Text overrides replace
neutral white/grey text, preserving semantic colours such as green active tabs.
Invalid colours fall back to the existing appearance. No existing user INI is
overwritten by this feature.

Missing, unreadable, corrupt or oversized PNGs fall back to the configured
background colour, or the existing black treatment if no colour is configured.
Limits: 16 MiB encoded file, 4096 pixels per dimension. Decode happens once at
startup, not during interaction. The supplied PNG is never modified.

The plugin uploads the decoded PNG into one immutable, explicitly registered
Scaleform texture, shared across menu reopens for the game process. No DDS cache
is generated or selected. A cache left by 0.1.43 is unused and may be removed.
The menu checks loaded clip bounds against the native loader's wrapper size.
Skyrim VR represents registered textures as 64x64 Scaleform clips even when
their GPU images are larger. Fitting uses the decoded PNG dimensions, not those
square clip bounds, so the original image aspect ratio is preserved. Unexpected
wrapper dimensions are rejected and the colour background remains visible.

Projection is measured on the game thread from the four-vertex UI surface's
CPU positions/texture coordinates and the movie's visible frame. No debugger,
GPU readback, renderer locking or guessed desktop/headset aspect is needed.
The image waits for the measurement; unsupported/unavailable surface geometry
falls back to the colour background. Rotation and uniform menu scaling do not
change the image proportions. Image fitting does not resize the controls or
alter the wand interaction plane.

These controls apply to the patched VR menu; the flat-runtime menu is unchanged.

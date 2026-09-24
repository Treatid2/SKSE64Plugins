# RaceMenu VR 2 — 0.1.89 beta candidate

This candidate corrects VR preset export and loading for the complete player
tint-mask array. Skin tone, complexion, dirt, warpaint and other tint layers
now use Skyrim VR's runtime data, retain their colour and opacity, and preserve
the selected texture even when its saved opacity is zero. Textureless tint
entries are retained rather than silently omitted.

RaceMenu's existing appearance-preset families remain unchanged: head parts,
hair colour, weight, face and head textures, face morphs, custom morphs, sculpt
data, node overrides and overlays, skin overrides, transforms and body morphs.
Existing presets that contain `tintInfo: null` cannot recover tint information
that an older build did not write; resave them with this build.

This candidate also introduces the runtime-neutral `VisualEquipment` provider
interface for mods which alter rendered equipment without changing inventory.
Providers can expose effective armor and armor-addon slot masks, enumerate the
effective `(ARMO, ARMA)` pairs RaceMenu should traverse, and request a coherent
RaceMenu visual refresh. This removes the need for cooperating mods to detour
private functions in a particular `skee64.dll` build.

Native version: 0.5.0.99. Package version: 0.1.89. Runtime qualification is
pending.

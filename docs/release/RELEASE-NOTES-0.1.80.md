# RaceMenu VR 2 0.1.80 — VR beta, inventory-preview crash correction

Native plugin version: 0.5.0.90.

Character creation and previews of varied armour/clothing items passed the
development headset test. Preview rotation/zoom was not tested. This addresses
the identified inventory-preview fault, not every possible random crash.

Fixes an out-of-bounds read in the inventory armour-preview tint hook. Skyrim
VR uses 0x48-byte preview entries and an array count at manager offset 0x258;
the hook previously used the flat-screen 0x20-byte entry layout and read its
count from inside a VR entry. This could turn an ordinary inventory preview
into a long invalid-memory walk, matching the supplied boots-preview crash.

The hook now selects the VR array representation explicitly, checks the
engine's seven-preview limit, capacity and storage before indexing, and checks
for a null preview node. The flat-screen representation remains separate.
The CommonLib correction is retained as a patch applied to pinned dependency
source by the build wrapper.

Regression tests exercise the actual CommonLib inline/heap VR array layout,
the second entry's stride, a poisoned legacy count location, invalid counts,
capacity mismatch, null storage and the flat-screen lookup representation.
Compile/test results and binary identity are recorded with the retained build.
These tests do not establish that every reported random crash has this cause.

No menu, Sculpt, lighting or INI behaviour is changed. Preserve both existing
INIs when installing over a test profile. Original RaceMenu SE 0.4.20.0 assets
remain required and are not included; SteamVR still requires VR Menu Mouse Fix.

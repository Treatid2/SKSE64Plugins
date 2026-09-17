# Menu height offset candidate

0.1.60 / native 0.5.0.70 adds `fHeightOffset` and
`fColorPickerHeightOffset` under Menu Profile VR Normal. Values inherit shared
Menu Appearance when empty, otherwise validate finite numbers in -150..150;
missing/invalid values default to zero. Both settings are read once at startup.
Normal/Face positioning remains shared and the fixed viewer anchor is unchanged.

The native placement handler snapshots the existing colour-field visibility
to choose the independent picker/main height before queuing the game task.
Height is added after angular/radial positioning; the orientation is calculated
from the resulting viewer-to-centre direction. Geometry and ray parent use the
same existing transform path. At zero, the established frame is preserved
exactly. No ActionScript, original asset or RMP payload is changed.

Managed Release build passed in 50.905 seconds. Added mandatory polar placement
tests run with assertions enabled in Release; cover positive/negative/zero
height, elevation/radius combinations, unchanged horizontal placement and
viewer-facing axes. SWF policy tests and 23 production shader compilation
checks passed. This is not a live qualification of the new offsets.

Retained logs:
`L:/Codex/logs/managed-process/20260917-230251.271-racemenu-menu-height-0160/`.
DLL SHA256: `BB153205FF85ECA6C5DAD32B54E1A7B934E6C7EFFCF6C41BA1804EDECE086BFC`.
Test package: `L:/Codex/artifacts/RaceMenu-VR-2/menu-height-0160/RaceMenu-VR2-0.1.60-test.zip`.
ZIP SHA256: `8C897887165E34644CB2AD112B2192D0D0EB0C1BF3A571C51DF2081C88645EA2`.
All 78 entries independently stream-hashed against the staging manifest plus
candidate notes; direct Data-root, no original assets included. Native build
receipt discloses unrelated dirty worktree state, which is not imported into
the package. The wrapper exports pinned dependencies, not dirty dependency data.

Publication was interrupted at the user's request; Nexus and immutable 0.1.58
release/tag remain unchanged. User confirmed the previous 0.1.59 warning:
large clear notice for the wrong movie, no notice for the correct original.
That acceptance does not extend to this newly built 0.1.60 height candidate.

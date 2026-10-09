# Persistent VR view controls — first pass

The consolidated Sliders page gains two stock slider rows beneath its bottom
action/race footer: **Avatar rotation** and **View direction**. They are separate
from the scrolling/category-filtered list, so opening Race, Body or another
category does not hide them. The existing View/All entries are retained as a
fallback and to preserve the diagnostic API's live-list identity contract.

The panel uses the original movie's `SliderListEntry` linkage (export ID40 in the
hash-pinned original VR movie), not copied upstream class source or newly drawn
slider controls. Project-owned `ViewPanel.as.inc` supplies placement and binding.
The upstream movie/source/artwork licensing boundary is unchanged. This does not
enable the separately retained source-extension SWF or finish its loader/GFx
qualification.

112 source pixels are reserved inside the existing panel/background/pointer
surface; the main list and footer move up by that amount. Rows are uniformly
scaled to fit the available width and44-pixel row height,48pixels apart. No
new world-space menu extent or placement origin is introduced. Legacy
non-consolidated profiles keep their original View/All placement.

Input uses the same keyed `SetMenuExtensionValue` dispatch and native movie/token,
topology, lifecycle and restoration guards. View direction retains the400ms
quiet-window staging and inverted intuitive sign. Endpoint behaviour remains
the stock behaviour (select the corresponding limit, not a one-degree step).
Display synchronisation does not dispatch a change. Widgets are created once;
polling values/revisions does not call `setEntry` again or rebuild a held drag.

The first pass is visible on the **Sliders** main page, in every subcategory.
Presets and Sculpt remain unchanged. Rebuilding/race changes disable the rows;
modal texture/colour/text entry hides or disables them. Callback-time checks
reject inactive UI, stale revisions and removed panel owners independently of
the existing500ms appearance poll. No extra timer or input listener is added.
Unload invalidates callbacks and clears stock per-row deferred-event timers.
Missing renderer/control or changed ranges keeps the existing category fallback.

## Qualification boundary

Five executable AS2-glue surrogate tests plus the34 existing native source/math
checks pass. These are not an AS2 compilation, real GFx input test or visual
layout proof. The interpreted movie recipe changed, so the unchanged-movie
native successor lane cannot compile this candidate. Broker-owned movie
qualification, reconstruction of the matching RMP/native pin and exact paired
native compilation are required before any separately authorised installation.
No test build is installed or release published by this source change.

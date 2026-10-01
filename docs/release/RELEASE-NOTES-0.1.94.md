# RaceMenu VR 2 — 0.1.94 VR beta

This complete update adds live menu diagnostic controls and corrects the
zero-based indexing of custom head-part attachments, such as antlers.

The diagnostic API enumerates the active slider list with live values,
minimum/maximum values, intervals and availability. Validated slider, race
and sex requests use the menu's normal callbacks; they do not directly write
actor data. Each query supersedes previous snapshots; each dispatched action
consumes its snapshot before the callback. Frozen control identities detect
reordering, replacement and in-place changes. Results include request identity
and value for correlation. Race/sex changes and rebuilds invalidate snapshots.
Requests made on an inactive Sliders tab, with stale state or with an invalid
value are rejected with a specific error. See diagnostic-menu-api.md.

Custom attachment sliders now consistently use -1 for no attachment and
zero-based indices for actual parts. The correction covers initial selection,
removal and invalid-index handling. Existing race-specific restrictions remain.

Character-creation lighting cleanup and switch-state synchronization are
hardened. The prior body-morph lock-order and VR inventory-preview callback
corrections remain included, together with the complete shaders, configuration
template, runtime menu patch and modder resource interface.

Validation: the live menu API was exercised successfully in character creation.
The custom attachment/antler path has not been directly tested in-headset.
The snapshot validation correction passed 17 focused production-source checks;
the wider offline Node suite passed all 34 checks. These are not a claim
that all sliders or mod combinations have been tested. The reported intermittent
race-change/scene crashes have not been established as RaceMenu-related.

Install the matching DLL and runtime menu patch together, after the required
original RaceMenu SE 0.4.20.0 assets. Preserve both skee64.ini and your existing
skee64_custom.ini settings. No manual menu generation is needed. VR Menu
Mouse Fix is not required. This remains a VR beta, not an SE/AE replacement.

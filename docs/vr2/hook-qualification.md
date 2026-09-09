# VR hook qualification matrix

## Current hazard

`skee64/SKEEHooks.h` defines 49 custom address identifiers and resolves its
wrappers with expressions such as `REL::RelocationID(0, kID_...)`. In the VR
CommonLib configuration, the first argument supplies the VR identifier. These
calls therefore resolve from zero rather than from the AE identifier in the
second argument.

A sparse numeric mapping experiment was not sufficient: only five of the 49 AE
identifiers mapped directly to an SE identifier, and only two of those had a VR
entry. Numeric coincidence is not acceptable address provenance. Locate each
VR routine by semantics, symbols where available, call graph, cross-reference,
and instruction comparison.

Hard-coded offsets from an entry point and every Xbyak trampoline require their
own SkyrimVR.exe instruction-window validation. An Address Library entry proves
only the entry point, not the safety of `entry + offset`.

## Required per-hook record

Maintain a checked-in inventory for every hook with:

- stable feature-group name and source call site;
- AE/SE/VR relocation IDs or a documented reason no ID is used;
- SkyrimVR executable version and Address Library version;
- expected instruction bytes/signature and overwrite length;
- calling convention, register/stack assumptions, and original-call behavior;
- trampoline lifetime and thread/lifecycle constraints;
- behavior when qualification fails; and
- static, load, runtime, and regression evidence.

Installation must compare the expected instruction window before writing. A
mismatch disables only that group and produces an actionable log entry.

## Feature groups

| Order | Group | Representative work | Product consequence |
| --- | --- | --- | --- |
| 1 | Core services | SKSE lifecycle, serialization, Papyrus, interfaces | Required foundation; no raw hook should be needed merely to load |
| 2 | Body systems | BodyGen/morph, skeleton, body overlays | High user value and separable from face UI |
| 3 | Native face sliders | sex/playable queries, slider lookup/insertion, category invocation, double-morph callbacks | Minimum useful custom slider/head-part integration |
| 4 | Face morph application | `UpdateMorphs`, `UpdateMorph`, FaceGen apply/race morph | Extended TRI application and persistence after head regeneration |
| 5 | Sculpt rendering | `RaceSexMenu` render-vtable path | Separate 3D sculpt-model rendering |
| 6 | Head/preset compatibility | regenerate-head and preprocessed-head paths | Preset/head-generation compatibility |
| 7 | Tint and inventory | skin/hair/model updates and inventory model hooks | Tint synchronization and inventory presentation |
| 8 | Face overlays | allocation/free and update-head-state hooks | Most invasive; qualify last |
| Optional | FaceGen cache bypass | cache-control hook | Must remain independently gated |

## Face-hook interpretation

Face hooks are not one all-or-nothing feature. Native slider insertion and
morph application are needed for a credible RaceMenu face workflow. Sculpt
rendering is needed only for the separate sculpt model. Head preprocessing,
tint/inventory, face overlays, and cache bypass can be qualified and released
independently.

CommonLib's `RaceSexMenu::GetRuntimeData()` is VR-aware, but the local
`RaceMenuSlider` layout is documented as verified against Skyrim SE. Directly
validate every field written on VR before enabling slider insertion.

## Fail-closed design

Hook installation should return a structured result per group rather than one
global success flag. The startup log should state:

- runtime and executable version;
- dependency versions;
- each group as enabled, disabled-by-policy, unavailable-address,
  signature-mismatch, or initialization-failed; and
- the exact feature consequence without claiming unrelated services failed.

No failed optional hook may leave a partial write, stale trampoline, published
callback, or half-initialized singleton. Install a group transactionally or do
not expose it.

## Runtime evidence

Intermittent defects do not require a contrived deterministic reproduction
before they can be addressed, but release claims still require proportionate
evidence. Use long-running repetitions, saves, cell transitions, actor
unload/reload, RaceSexMenu open/close cycles, preset round trips, and head
regeneration. Record exact versions and preserve crash dumps when a fault is
rare.

The safe-load milestone should be tested before any face hook is enabled. Each
subsequent group then gets an enabled/disabled A/B run so a crash can be
attributed without treating the entire plugin as one experiment.


# VR hook qualification matrix

## Addressing rule

`skee64/SKEEHooks.h` defines 49 historical flat-runtime address identifiers.
They are not treated as implicit VR identifiers: a VR site must instead have an
explicit Address Library mapping or an independently qualified SkyrimVR 1.4.15
RVA and exact instruction/target contract.

A sparse numeric mapping experiment was not sufficient: only five of the 49 AE
identifiers mapped directly to an SE identifier, and only two of those had a VR
entry. Numeric coincidence is not acceptable address provenance. Locate each
VR routine by semantics, symbols where available, call graph, cross-reference,
and instruction comparison.

Hard-coded offsets from an entry point and every Xbyak trampoline require their
own SkyrimVR.exe instruction-window validation. An Address Library entry proves
only the entry point, not the safety of `entry + offset`.

## Required per-hook record

The machine-readable inventory is
[`hooks.json`](hooks.json). `tools/validate-vr-hook-inventory.ps1` verifies that
it covers every custom relocation declared in `skee64/SKEEHooks.h`, preserves
the AE IDs, and does not assign a VR ID to an unqualified entry.

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

The current pass has explicit VR implementations for pointer input, body
attachment, native sliders, morph updates, sculpt rendering, mapped head
presets, tint/inventory integration, face overlays, and the optional FaceGen
cache bypass. Each group validates every enabled instruction window and decoded
original call target before committing any write. Direct helper RVAs are gated
to SkyrimVR 1.4.15 and their exact retained-executable prologs are checked as
one fail-closed table before any SKEE hook group writes game memory. This table
covers the shader/material, FaceGen/morph, slider, geometry, `NiStream`,
inventory, and Scaleform helpers called by the VR implementation. CommonLib
methods replace custom entry points where a versioned public contract exists.

## Qualified RaceSexMenu pointer input

The stock RaceSexMenu constructor call at RVA `0x8DD500+0x66` requests the flat
movie. The VR baseline independently replaces that call and loads
`VR/RaceSex_menu`; RaceMenu supplies that VR asset alongside the flat movie. It sets
`kRendersOffscreenTargets` before loading, but deliberately leaves
`kUsesCursor` clear: native wand coordinates do not need engine cursor-menu
ownership, and that ownership draws the redundant quill. The movie-local mouse
endpoint remains enabled. This constructor-time work is deliberately earlier
than the old menu-open-event mutation that left later VR menus pinned to
RaceSexMenu's world pose.

Skyrim VR's `RaceSexMenu` has a secondary `MenuEventHandler` vtable at RVA
`0x173F328`. CommonLibSSE-NG records that address as
`VTABLE_RaceSexMenu[1]`. The VR baseline installs a menu-local `CanProcess`
hook at `+0x8` during `kDataLoaded`, before character creation can open.

The hook recognizes motion-controller Trigger plus the mapped
Accept/Activate/Click action, reads the engine's native laser coordinates at
RVA `0x2FEBC40`, allocates down/up events through the engine-owned Scaleform
pool at RVA `0xF37680` using event state at RVA `0x3013620`, and dispatches them
to `RaceSex Menu` through RVA `0xF209A0`. Installation requires the exact
SkyrimVR 1.4.15 version, exact 15-byte/10-byte prologs for those two helper
functions, and both globals to lie in the executable's data segment. Non-pointer
input is forwarded to the captured original handler. The movie-local cursor
endpoint is repaired at the input edge without further flag mutation.

Installation fails closed if the vtable slot is outside Skyrim's read-only data
segment or if its original target is not inside Skyrim's executable code. This
also prevents silently stacking the baseline over a still-enabled `skeevr.dll`
or another competing RaceSexMenu handler patch.

## Qualified NiStream lifecycle

The Skyrim VR Address Library records the `NiStream` constructor as SSE/VR ID
68971 (SSE `0x140c59690`, VR `0x140c9ec40`) and the destructor as ID 68972
(SSE `0x140c598f0`, VR `0x140c9eea0`). Both records have status 4, meaning the
VR routines are bit-for-bit identical to the SSE routines. The installed
Skyrim VR 1.4.15 release CSV version 0.261.0 independently maps those IDs to
RVAs `0x0c9ec40` and `0x0c9eea0`.

`SKEE::NiStreamCtor` and `SKEE::NiStreamDtor` therefore use an explicit
three-runtime relocation: SE 68971/68972, AE 70324/70325, and VR 68971/68972.
This restores construction for NIF loading during node-transform lookup. The
separate `NiStreamAddObject` helper at VR RVA `0xC9F090` is now independently
qualified by its exact `40 57 48 83 EC 30` entry window before it is available
to save/export work. The surrounding `NifStreamWrapper` also checks
construction, input streams, and object pointers before any virtual call.

## Qualified VR armor attachment callback

The flat build detours an `AttachBipedObject` function entry. The independently
qualified VR contract instead replaces the armor-node call inside the biped
attachment routine. Skyrim VR Address Library ID 15501 resolves the enclosing
routine to RVA `0x1D7450`; the call is at `+0xC41` (RVA `0x1D8091`). The exact
SkyrimVR 1.4.15 dump contains this 14-byte window:

`45 8B C7 48 8B D6 49 8B CD E8 E1 35 00 00`

This is `mov r8d,r15d; mov rdx,rsi; mov rcx,r13; call rel32`, with the call
resolving to RVA `0x1DB680`. Disassembly establishes `r13` as `BipedAnim`,
`r12` as `slotIndex * sizeof(BIPOBJECT)`, and the armor/addon fields at
`r13+r12+0x10/+0x18`. The VR bridge preserves the two original stack arguments,
packs the original `r8d`/`r9d` pair into wrapper `r8`, supplies the slot record
in wrapper `r9`, calls the captured six-argument armor-node function, and
resumes at call-site `+0xE`. The wrapper calls the original first, then forwards
the created node and armor/addon pair to `ActorUpdateManager`.

Installation requires the exact 14 bytes, exact decoded call target, and text
segment membership before allocating either trampoline. A mismatch leaves the
entire group disabled and logs all 14 observed bytes. The wrapper uses
CommonLib's `TESObjectREFR::LookupByHandle`; shader texture invalidation now
uses CommonLib's explicit Skyrim VR relocation instead of the former empty VR
fallback. Rendering overlays remain separately gated, so this tranche restores
attachment-driven body morph, skeleton, and override observers without enabling
the later face/overlay hook groups.

## Feature groups

| Order | Group | Representative work | Product consequence |
| --- | --- | --- | --- |
| 1 | Core services | SKSE lifecycle, serialization, Papyrus, interfaces | Required foundation; no raw hook should be needed merely to load |
| 2 | VR pointer input | Constructor-time VR movie selection, balanced cursor ownership, menu-local event claim, and native GFx mouse dispatch | Trigger/A interaction without global input interception or late menu-flag mutation |
| 3 | Body systems | Qualified armor attachment callback for BodyGen/morph, skeleton, and override observers; rendering overlays remain gated | High user value and separable from face UI |
| 4 | Native face sliders | sex/playable queries, slider lookup/insertion, category invocation, double-morph callbacks | Minimum useful custom slider/head-part integration |
| 5 | Face morph application | `UpdateMorphs`, `UpdateMorph`, FaceGen apply/race morph | Extended TRI application and persistence after head regeneration |
| 6 | Sculpt rendering | `RaceSexMenu` render-vtable path | Separate 3D sculpt-model rendering |
| 7 | Head/preset compatibility | regenerate-head and preprocessed-head paths | Preset/head-generation compatibility |
| 8 | Tint and inventory | skin/hair/model updates and inventory model hooks | Tint synchronization and inventory presentation |
| 9 | Face overlays | allocation/free and update-head-state hooks | Most invasive; qualify last |
| Optional | FaceGen cache bypass | cache-control hook | Must remain independently gated |

## Face-hook interpretation

Face hooks are not one all-or-nothing feature. Native slider insertion and
morph application are needed for a credible RaceMenu face workflow. Sculpt
rendering is needed only for the separate sculpt model. Head preprocessing,
tint/inventory, face overlays, and cache bypass can be qualified and released
independently.

`RaceSexMenu::GetRuntimeData()` describes the flat layout. Skyrim VR omits the
flat `RaceSexCamera` member between `headParts` and `sliderData`; the VR build
therefore uses a separate `VR_RUNTIME_DATA` accessor. Dump-backed disassembly of
SkyrimVR 1.4.15 establishes `sliderData[0]` at absolute menu offset `+0xF8`, the
active race/component index at `+0x140`, sex at `+0x150`, a `RaceComponent`
stride of `0x28`, and a `RaceMenuSlider` stride of `0x138`. All RaceMenu slider
and head-part access is routed through runtime-aware helpers so a custom slider
cannot be misclassified and forwarded to Skyrim's vanilla morph callback.

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


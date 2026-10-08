# VR character coordinate probe (candidate)

The original coordinate probe implements the explicitly offered **instrumentation-first** alternative to
avatar-rotation controls. It adds no sliders and changes no scene transforms.
Player view yaw remains the existing bounded tracking-origin adjustment. Avatar
rotation needs a separately qualified owner/path; VR has no embedded flat-game
RaceSexCamera, and sculpt mesh rotation is not whole-character rotation.

## Capture interface

While the exact RaceSex movie is open, call the registered Scaleform function
`_root.CaptureVRCharacterInspection()` (no arguments). `true` means queued, not
completed. `false` means unsupported, task interface unavailable, or one capture
already pending. After the game task executes, read
`_root.GetVRCharacterInspection()` (no arguments).

Equivalent CharGen bridge names are `CaptureCharacterInspection` and
`GetCharacterInspection`. These are diagnostic functions, not normal appearance
sliders, Papyrus/API-provider controls, or a new polling/console input mechanism.
No automatic timer, collector, file writer or runtime transport is added.

### Readout through the existing SKSE Papyrus UI bridge

SKSE's `UI.Invoke*` functions discard the Scaleform return value. For transports
that expose Papyrus but not a direct GFx invoke-return operation, use:

1. `UI.InvokeIntA("RaceSex Menu", "_root.CaptureVRCharacterInspection")`,
   with the optional typed integer array omitted (`None`), hence zero GFx arguments.
2. After the queued main-thread task has run,
   `UI.InvokeIntA("RaceSex Menu", "_root.PublishVRCharacterInspection")`,
   again with no array/arguments supplied.
3. `UI.GetString("RaceSex Menu", "_root.VRCharacterInspectionJSON")`.

Publish serializes the same movie/session-validated native copy into a bounded
(64 KiB maximum) movie-local JSON string. It does not capture again or read live
nodes. Its equivalent CharGen name is `PublishCharacterInspection`. A new movie
initializes the string to `not-published`; serialization failure produces
`readout-failed`, not an old successful payload. Node names are JSON-escaped,
non-ASCII is escaped, and invalid UTF-8 is replaced.

This zero-argument `InvokeIntA` route was verified through the selected DevBench
Papyrus bridge. Do not substitute `InvokeInt(..., 0)`: that passes an extra GFx
argument and violates the capture/publish contract. A transport's `called` status
or discarded return alone does not prove the native function accepted the call.

The string is a **transport copy**, not a continuously maintained live property.
Invoke Publish immediately before every read, verify RaceSex Menu is still open,
and check state/session/serial. Never read an old variable alone to infer that a
closed menu or replaced actor is still live. An explicit fresh Publish reports
expiry/wrong-movie through the same guard as the object getter. `UI.Invoke`
returning none is not confirmation that capture or publication succeeded:
the returned JSON state is the evidence, and `queued` may require another
bounded publish/read after a frame, not another capture request.

The getter reads a copied snapshot only. Capture runs through SKSE's main-thread
task interface, bound to the exact originating movie and menu generation. Menu
close expires retained data and cancels stale work. Old movie instances cannot
read a different movie's capture. No queued callback retains scene-node pointers.

## Schema 1

- `state`: `not-captured`, `queued`, `captured`, `unavailable`, `cancelled`,
  `expired`, `wrong-movie`, `dispatch-failed`, or `capture-failed`.
- `serial`, `session`: decimal strings (no 64-bit GFx number rounding).
- `queued`: whether the one allowed task is pending.
- Only `captured` includes the following evidence:
  - race and cell FormIDs, numeric `menuView`, `viewYawDegrees`;
  - actor position and engine Euler angles in **radians**, with separate validity
    flags; invalid values are omitted;
  - ten named node roles: avatar, avatar parent, head, pelvis, tail, tracking
    origin, tracking parent, headset, menu, menu quad.
- Each role explicitly indicates `present`. Present nodes report hexadecimal
  identity/parent strings, a node name capped at 128 characters, and independent
  `localValid`/`worldValid` flags. Valid transforms include position `[x,y,z]`,
  uniform scale, and nine `rotationRowMajor` matrix entries. Do not infer an
  engine Euler convention or visual forward axis from the field name.
- Present roles also report `fixedBound` and `worldBoundValid`. Finite,
  nonnegative world spheres include copied center/radius; invalid values are
  omitted. Radius zero is an engine empty bound, not a containing sphere.
- `ancestorsSelfFirst` is bounded to 16 entries, including the node itself.
  `ancestryComplete` distinguishes complete chains from truncated/cyclic ones.
  `sharesTrackingOrigin: true` proves observed membership. `false` only proves
  exclusion when ancestry is complete **and** tracking origin is present.
- Missing tail/head/pelvis nodes are meaningful observations, not capture failure.
  Different skeletons may use different names; no claim is made that these named
  probes exhaust the skeleton.

This is one main-thread CPU observation, not an atomic renderer/GPU frame or
proof of transform ownership. Comparing captures reveals relationships and
changes; it does not by itself grant permission to overwrite a transform.

## Explicit graph-bound preflight — native successor, 8 October

The JSON transport additionally copies `controls.avatarBoundProbe`. It runs only
on explicit main-thread capture, not on every menu update. Traversal retains the
4096-node/64-depth limits and validated topology, frame, native dispatch and
protected tracking/UI-node checks. Its diagnostic mode looks past fixed-bound
flags to find the first *other* failure. Actual preview application/restoration
still use strict fixed-bound rejection. The probe cannot change transforms,
flags, scene bounds, slider values or the latched application refusal.

`enumeratedNodeCount` includes queued but not necessarily validated nodes;
`validatedNodeCount` defines sphere coverage. Partial graph observations are
explicit and containment is null when coverage is incomplete. Fixed descendant
details are capped at eight, with a truncation flag; parent-first ancestors are
bounded by the depth limit. `geometryBoundsRoutineCount` identifies CB78C0,
not necessarily skinned meshes. `composerSkipFlagBit9Count` records the raw bit
that the inspected composer tests without borrowing an unqualified semantic name.

The sampled full-turn envelope is a sphere about the avatar world pivot. Each
positive sphere contributes distance-to-pivot plus radius. Empty spheres are
ignored; invalid samples prevent complete coverage. Complete sampled envelopes
are compared to each unchanged ancestor sphere with an explicit 0.01-world-unit
margin. This conservative spherical estimate can overestimate the required yaw
volume. It cannot account for stale skin spheres, geometry/AABB propagation or
rendered culling. `rotationQualified` remains false even if containment succeeds.

Engine evidence: the qualified C9DC10 parent-bound routine returns without bound
writes when VR flag bit13 is set. That explains why a fixed ancestor need not
automatically forbid child rotation, but does not qualify the remaining path.

## Next live evidence, after a separately authorised installation

Retain captures at normal and face view, before/after existing view yaw, and
after a race/sex change. Compare actor angles with avatar-root/parent matrices,
head/pelvis/tail positions, and headset/menu ancestry. Include New Game versus
loaded-save `showracemenu` if examining the sideways-facing report. Do not use
SetAngle, arbitrary node writes, or a guessed engine callback for this assay.

Use those results to choose an independent avatar-rotation path that preserves
pointer alignment, lights and camera framing, handles node replacement, and
restores only transforms it demonstrably owns. The proposed final UI remains a
separate inspection panel with view yaw (±60°, centre reset) and character angle
(full revolution, front reset), not more crowded footer buttons. Placement and
sliders are intentionally not introduced by this diagnostic candidate.

# VR character coordinate probe (candidate)

This implements the explicitly offered **instrumentation-first** alternative to
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

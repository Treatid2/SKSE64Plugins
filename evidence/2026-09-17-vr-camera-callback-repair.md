# VR Camera-tab callback repair

The diagnosed 0.1.55 crash is recorded in `2026-09-17-camera-query-crash.md`.
All three native callbacks now select a VR tracking implementation before any
flat RaceSexMenu camera access. An unavailable VR path never falls back to the
nonexistent embedded camera. The non-VR path retains the menu and camera node,
uses the runtime accessor, and checks node availability.

## Coordinate and ownership contract

- VR position is the tracked HMD world position transformed into the avatar's
  parent space, matching the existing `GetPlayerPosition` coordinate frame used
  by CameraEditor. Rotation uses that same frame.
- Movement captures the requested delta in world space and queues it on the
  SKSE game task interface. It translates RoomNode, not HmdNode or the avatar.
  Real headset motion between enqueue and execution is preserved.
- Tracking ancestry, independent avatar hierarchy, finite orthonormal frames,
  positive scale, and a bounded movement are required. Missing/wrong-type,
  nonfinite and float-overflowing supplied coordinates are rejected atomically;
  omitted coordinates are preserved.
- The queued operation checks menu-session generation and re-resolves the same
  origin, HMD, avatar and parent nodes. Replaced origins or origins changed by
  another owner are not overwritten.
- Manual Camera-tab translation shares Face View's owned offset and cleanup.
  Face-view refresh retains manual movement. Returning to Normal View resets
  the accumulated translation, as does menu close/revert. Cleanup removes only
  an offset still owned by this implementation.
- `GetFaceViewDiagnostics` includes camera read, applied movement, rejected
  queued movement and cancelled-session counters. These counters do not count
  invalid Scaleform arguments as queued movement.

## Offline qualification

The required `vr2_camera_policy_tests` build target exercises the production
policy header: lazy VR/non-VR dispatch (including poison flat fallback), rotated
and scaled avatar/tracking coordinate frames, coordinate round trips, preserved
head motion, offset composition/reversal, and invalid transforms/movements.
It does not simulate the engine's scene graph, GFx interface or task queue.

Managed VR Release build 0.1.57-vr-camera / native 0.5.0.67 completed successfully.
Camera policy tests passed, all 23 shader sources compiled through the corrected
production runtime compiler, and native SWF parser policy tests passed.
The SWF patch payload is unchanged. Exact private SWF reconstruction was not
rerun for this build; it remains separately qualified in earlier evidence.

Retained binary:
`L:/Codex/artifacts/RaceMenu-VR-2/runtime-swf-patch/native-0157/Data/SKSE/Plugins/skee64.dll`

SHA-256: `EC0CC30C1E4B6ADE1D754704E6485A8E4928FAD4796D11E08F194D5E0BB38FDD`

Build receipt and log are retained alongside that artifact (`build-receipt.json`
and `../native-build-0157.log`). The candidate was built from a recorded dirty
development worktree, not a final immutable release tag.

Live Camera-tab movement, close/reopen, Face View and Sculpt qualification
remain pending. Offline tests and successful linkage are not live acceptance.

The asset-free TEST archive contains 77 files at the direct Skyrim Data root;
each ZIP member was independently checked for path, length and streaming
SHA-256 against the staging manifest, with the archive hash unchanged across
verification. No original SWF/BSA/ESP or background PNG is included.

Archive: `RaceMenu-VR2-0.1.57-vr-camera-TEST.zip`

SHA-256: `9DAB1B64849DE08F9225235B8609C4A762868499C283E873C56A17F743431E87`

Retained receipts: `addon-stage-0157-receipt.json` and
`independent-package-verification-0157-20260917.json` in the same authoritative
runtime-swf-patch artifact directory. Managed build/staging allocations were
released as reclaimable only after promotion and verification.

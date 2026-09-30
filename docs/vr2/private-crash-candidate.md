# Private crash-reproduction candidate

This candidate restores the completed body-morph, inventory-preview and
avatar-lighting corrections alongside the live menu diagnostic API and the
zero-based custom head-part slider correction. It is for the isolated crash
reproduction environment; it is not a public release.

The integration starts from `baf200aa62ed67c13a7676b082b8e36a9f4e3d7f`.
The earlier broker result at that commit excludes the subsequent fixes that
were still in the development worktree.

## Source behavior

- Prepare skin-partition updates while the morph cache is locked, then release
  that lock before submitting or executing the updates. This removes the lock
  cycle with the SKSE task queue observed in the Whiterun transition dump.
- Preserve the Skyrim VR inventory-preview callback's `NiNode**` contract.
  Resolve its node once for dye work and forward the holder unchanged to the
  original engine target. Flat-runtime callbacks keep their `NiNode*` contract.
- Retain the three lights' exact renderer registrations and retire them after
  renderer removal queues reopen. Keep requested light state distinct from
  applied light state, and synchronize the menu label with the applied state.
- Send the legacy Papyrus light toggle only when the menu reports that light
  as enabled. A default-off menu must not toggle that old light on.
- Keep the live diagnostic API's generation checks and actual menu callbacks
  for race, sex and slider changes. Keep `-1` distinct from part index zero.

## Runtime movie pairing

The native loader reads the exact original RaceMenu SE 0.4.20.0
`Interface/VR/RaceSex_menu.swf` and a hash-pinned
`SKSE/Plugins/RaceMenuVR2/racesex-menu.rmp`. It rejects a generated loose movie
as the original input. The broker's full generated movie is a private compiler
output, not the deployable menu payload.

After compiling the integrated movie, generate and audit an RMP against the
hash-pinned original. Commit that payload, its manifest and the matching native
`SwfBytePatch::ApplyRelease` hash, then compile the final exact commit. Verify
that applying the RMP produces the canonical compiled movie byte for byte.
The crash-testing owner deploys the final DLL, compiled shaders and matching
RMP over the original RaceMenu assets, preserving the testing profile's INIs.

## Validation boundaries

The broker compiles native code, shaders and the private movie. Its current
compile-only graph excludes the VR test executables. A successful compilation
does not report those tests as executed. Keep offline checks and live race
changes, inventory previews and cell-transition results separate in the final
candidate receipt.

Debug COC crash owns MO2 setup, deployment and runtime testing. This task owns
RaceMenu source integration and compilation and does not launch that environment.

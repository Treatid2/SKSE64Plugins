# SteamVR pointer qualification — publication gate

Observed 2026-09-17. The successful 0.1.53 layout/name-save qualification used
OCU. Subsequently the user tested SteamVR with VR Menu Mouse Fix and reported
it workable, explicitly accepting the helper as a SteamVR dependency. This
removes the no-pointer blocker, but is not exhaustive SteamVR modal/text-entry
qualification. The initial disabled-helper snapshot below is not the final state.

## Initial evidence

- Direct DevBench lane `1756fd7239ef`, bound port 8921, identified SkyrimVR.exe
  PID 43684, DevBench 1.16.0, with a loaded player and RaceSex Menu open.
- Native read-only `CharGen.ReadVRInputTrace` identified package 0.1.53,
  producer 0.5.0.63, contract 2. The trace was never armed and no synthetic
  input was active. Movie mouse endpoint count was one.
- Two read-time movie mouse samples were (985.8631, 0) and (966.2736, 0).
  These are not controlled controller-motion or click samples. They do not
  establish a general Y-axis fault or a physical hit location.
- The preceding OCU log explicitly records `Creating VRMenuLaser system`,
  beam/dot atlas creation, and use of the game uiNode plane. Thus the lasers
  seen in the previous successful test were supplied by OCU, not this project.
- The current exact-profile modlist has VR Menu Mouse Fix disabled. Its
  installed configuration enables ShowLaserPointer, and the author's page
  documents lasers in cursor-enabled menus. Its log predates this session;
  it is not evidence of that helper running now.
- RaceSexMenu's constructor hook sets offscreen rendering but deliberately
  leaves UsesCursor clear to hide the quill. Existing pointer-button injection
  consumes the engine mouse-coordinate global; it does not implement a
  controller ray intersection or render a laser itself.

## Next experiment / design decision

Test a separately installed SteamVR pointer provider with correctly balanced
constructor-time cursor ownership, or implement a project-owned pointer
backend. The helper is now an accepted SteamVR-only dependency. Do not copy
the helper's code into GPL source without resolving its distinct permissions.

For any provider, qualify hover and physical Trigger down/up against the actual
transformed UV surface; require the visible beam, hit coordinates, and movie
target to agree. Test normal/face transitions, colour picker, Filter and Name
keyboard completion, menu close/reopen, and OCU regression. No menu-layout
change or debugger attachment is justified by the initial samples alone.

Source: author's VR Menu Mouse Fix description and changelog:
https://www.nexusmods.com/skyrimspecialedition/mods/33414

## CommonLib submission inventory

Two local dependency changes exist, neither established as accepted upstream:

1. `include/RE/R/RaceSexMenu.h`: VR_RUNTIME_DATA and accessor for the different
   VR object layout (26 added lines in the current submodule worktree).
2. `include/RE/N/NativeLatentFunction.h`: invoke the GetRawType function object
   as `GetRawType<latentR>{}()`; the build wrapper applies the retained
   `evidence/commonlibsse-ng-latent-vr.patch` to its staged dependency.

Submit these against CommonLib separately from the RaceMenu repository changes.

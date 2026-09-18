# Sculpt pointer and stroke qualification

Candidate 0.1.65 / native 0.5.0.75 is functionally headset-qualified for visible
deformation, History population/undo/redo and head export/clear/import by the
user. Performance remained stable by observation, not a measured FPS assay.
The user's preset save/change/load cycle also restored the saved character.
See `evidence/2026-09-18-0165-live-qualification.md` for scope and limitations.
The subsequent 0.1.79 / native 0.5.0.89 workspace is headset-accepted: single
left controls column, right-hand canvas, usable expansion, working turn/centre
buttons and rotation retained through Face/Normal view changes. The earlier
candidate and preflight descriptions below are historical diagnostic steps.
In the preceding 0.1.62 headset run, the user observed stable hover and
visible deformation, but no visible History entries. Undo/persistence remain
unqualified. The 0.1.63 live trace recorded four commits, four undo pushes,
four queued/executed History tasks and four failed direct AddAction invocations.
The registered AS2 GameDelegate callback was independently read as onAddAction;
its History list contained zero entries. Candidate 0.1.64 routes VR stroke and
standard-command notifications through the registered external call dispatcher,
with AddAction as the first argument. The non-VR direct route is unchanged.
History population and selection/undo/redo were subsequently confirmed in
0.1.65. Cross-session sculpt persistence remains unqualified.

## Source changes

Picking previously mapped the D3D11 dynamic vertex buffer with
WRITE_NO_OVERWRITE and read every triangle from that write mapping. The
initial CPU vertex allocation was released without freeing it. Nested normal
calculation and per-vertex brush edits also reused write mappings.

The authoritative vertices now stay in owned CPU memory. Picking, brush
radius selection and normal calculation read that copy. Edits mark it dirty;
the next mesh draw performs one write-only WRITE_DISCARD upload. A failed
upload retains dirty state for retry. Solid and wireframe passes share the
upload. Primitive brush meshes own separate, correctly destructed storage.

This follows Microsoft's warning about slow reads from write-combined mapped
memory: [ID3D11DeviceContext::Map](https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-map).
It is a concrete source defect and plausible cause of OCU hover slowdown,
not a measured live attribution yet. No pointer-rate throttle is introduced.

Each painting session retains its starting brush. A duplicate begin ends
the previous stroke; updates without a begin are ignored; repeated ends are
safe. Brush changes, camera manipulation, history actions, mesh-state changes
and editor closure finalize an active stroke. Move strokes still receive
off-mesh drag rays. Closure finalizes before the actor/meshes are cleared.
Queued history notifications capture metadata, not dangling command pointers,
and are ignored after the editor generation changes. Mirror-off hover avoids
the mirrored raycast and hides the old mirrored cursor.

## Bounded native trace

The existing CharGen Scaleform plugin object exposes:

- `BeginSculptTrace()` resets aggregates and arms a 60-second window.
- `ReadSculptTrace()` reads aggregates; `ReadSculptTrace(true)` also stops it.

The registered path is `_global.skse.plugins.CharGen`. These are direct GFx
callbacks, not console commands. Contract 2 additionally publishes a bounded
JSON string at `_global.skse.plugins.CharGen.sculptTraceJson`. Use the existing
DevBench Papyrus tool after describing the live `UI` script signatures:

1. `UI.GetInt("RaceSex Menu", "_global.skse.plugins.CharGen.sculptTraceContractVersion")`
   must return 2.
2. `UI.InvokeBool("RaceSex Menu", "_global.skse.plugins.CharGen.BeginSculptTrace", false)`.
   Its `called/none` acknowledgement does not prove activation: read the JSON
   with `UI.GetString` and independently verify `active=true` and generation.
3. Perform the bounded physical test. Do not mix injected input into it.
4. `UI.InvokeBool("RaceSex Menu", "_global.skse.plugins.CharGen.ReadSculptTrace", true)`
   publishes and stops. Read `sculptTraceJson` with `UI.GetString`; require
   `active=false` and the same generation. Passing false reads without stopping.

History's registration and actual list state can additionally be read without
injecting a synthetic action:

- `UI.GetString("RaceSex Menu", "_global.gfx.io.GameDelegate.callBackHash.AddAction.1")`
  should return `onAddAction` while Sculpt is initialized.
- `UI.GetInt("RaceSex Menu", "_global.gfx.io.GameDelegate.callBackHash.AddAction.0.historyList.entryList.length")`
  reports the actual History entry count. Compare before/after a physical stroke.

Root AddAction lookups in the diagnostic report are expected to be absent in
this VR movie; they describe the old failed direct route, not the delegate
registration. One stroke can commit entries for multiple meshes and its mirror;
do not assume one entry per physical Trigger gesture.

Arming and reading serialize only at explicit calls; no pointer event logs.
The JSON also reports current active stroke, selected native brush, mesh/editor
generation, native undo count/index, task-interface availability and lookup
results/types for the three known AddAction callback locations. Missing lookup
is explicit. Lookup presence and successful Invoke are not proof that the
movie History list actually changed.
Tracing is off at startup; no debugger attachment is needed.

Each operation returns `calls`, `totalUs`, `maxUs`, and `units`:

- `hover`, `begin`, `paint`, `end`: callback/session costs.
- `scenePick`: complete scene picking and picker/brush application.
- `meshPick`: triangle raycasts; units are candidate triangles, not hits.
- `upload`: GPU upload attempts; units are attempted vertex bytes.
- `rejectedPaint`: updates rejected because no active stroke exists.
- Count-only lifecycle boundaries (timing fields remain zero): `pointerDown`,
  `pointerUp` (native VR dispatch edges, all pointer controls while armed),
  `endCallback`, `brushChange`, `strokeCommit` (units: changed vertices),
  `undoPush`, `historyQueued`, `historyRun`, `historyStale`, `historyNoMovie`,
  `historyInvoked`, `historyInvokeFailed`, `historyNoTask`. History counters
  currently describe stroke notifications, not reset/import commands. Counter
  recording ends at the same 60-second deadline, including deferred delivery.

Times are elapsed CPU-side durations, including waits, not GPU execution time.
Scopes nest: do not add their totals together. Upload counts include failures
after attempting a Map. The timer covers completed operations started in the
window; a pending scope can finish after the deadline. There is no per-pointer
logging, coordinate/vertex export or unbounded event buffer. Active tracing
adds timer/locking overhead; use equivalent trace settings for comparisons.

## Headset test sequence

Use a disposable character/test save. Verify the deployed DLL identity first.
Each trace phase must fit within its 60-second window; retain its response.

1. Open Sculpt, select a brush, leave the pointer outside the face for 10s.
2. Re-arm; traverse the face without pressing Trigger for 10s. Repeat with
   Mirror off and on. Hover must not produce painting begins or dirty uploads
   once initial scene setup is complete. Record callback rate and pick costs.
3. Re-arm; perform one short Trigger stroke, release, then hover. Confirm
   one begin/end lifecycle, visible deformation and no subsequent paint edits.
4. Repeat with release outside the face; change brush/view/tab while pressed.
   Confirm strokes end cleanly and no stale paint persists on re-entry.
5. Check Inflate/Deflate/Smooth/Move and mask brushes, Mirror, Undo/Redo,
   editor close/reopen and save/load persistence. A moving cursor alone is
   not evidence that sculpting or persistence works.

For FPS/GPU comparisons, use the performance-neutral DevBench/profiler contract
and stable ownership epoch. This trace alone cannot establish GPU frame costs.

## Viewing-direction rotation preflight (0.1.71)

`CharGen.GetFaceViewDiagnostics()` additionally returns read-only booleans
`rotationTopologyAvailable`, `menuSharesTrackingOrigin` and
`quadSharesTrackingOrigin`. Check availability before interpreting either
ancestry result: false sharing with unavailable topology proves nothing.
These report whether the registered menu node/visible quad descend from the
independent RoomNode that contains the HMD, not whether any rotation control
is implemented or qualified. A bounded view rotator must preserve both the
visible surface and pointer geometry, real HMD tracking, and restore its own
offset on menu closure without overwriting another owner's transform.

0.1.73 adds the read-only `vertexEditor.ReadVRSculptRotationPreflight()` bridge.
Invoke it through UI.InvokeIntA with `[0]` (the bridge ignores this argument;
the current DevBench call adapter cannot type empty arrays), then read
`vertexEditor.vrRotationPreflightAvailable` and
`vertexEditor.vrRotationPreflight.rotationTopologyAvailable`,
`.menuSharesTrackingOrigin`, `.quadSharesTrackingOrigin` using UI.GetBool.
The full prefix is
`_root.RaceSexMenuBaseInstance.RaceSexPanelsInstance.vertexEditor` in RaceSex Menu.
Require both availability flags before interpreting sharing. No rotation is
performed by this call.

## Isolated viewing-direction yaw test (0.1.74)

### Visible controls (0.1.75)

Sculpt adds Turn left, Centre view and Turn right to its registered action
panel. Left/right turn the viewpoint in 5-degree steps, limited to 60 degrees
either side of the original view. Centre restores the original viewing yaw.
The menu stays in place; these controls do not rotate the sculpt mesh. Real
headset tracking remains active. Menu closure restores the owned turn.
Requests during a sculpt gesture or an already queued turn are ignored.
Face view and canvas size remain independent controls. The native
`CharGen.SetViewYaw(degrees)` callback takes an absolute angle; the UI reads
the applied native angle before stepping. `_root.TestVRViewYaw` remains a
diagnostic alias. The larger range is for direct headset testing, not a
claim of completed headset qualification.

The native-only `_root.TestVRViewYaw(degrees)` function is callable in
`RaceSex Menu` through UI.InvokeInt. It accepts an absolute test yaw in
[-10,10] degrees, not a repeated incremental turn. Zero removes the owned yaw
and its accumulated pivot compensation. No visible control or default turn is
added, and the Sculpt layout/SWF payload is unchanged.

Refresh the read-only bridge after dispatch and inspect
`vrRotationPreflight.yawQueued` (Bool), `.yawDegrees` (Float), and `.yawState`
(Int): 0 neutral/restored, 1 applied, 2 rejected/ownership lost, 3 cancelled.
Dispatch completion alone is not evidence of rotation. Require queued false,
the expected state/angle, visible direction change, stationary menu/quad and
correct wand picking. Positive yaw is world-Z counterclockwise; perceived
left/right is a headset qualification question, not assumed from a label.

The operation requires independent HMD/avatar/menu/quad topology and valid
parent transforms, runs on the game thread, and is bound to the exact movie
and character-creation session generation. An existing yaw requires unchanged
origin, HMD, avatar, menu and quad identities plus owned local pose and parent
world transform. It rotates the tracking origin around the live eye and
retains real tracked head/hand motion. Face-view translation composes with the
separate yaw compensation. Exit restores only an owned pose; a competing
transform is never replaced with a stale snapshot.

Start with a disposable character, pointer outside the canvas, no active
stroke. Test +5 degrees then zero; verify menu and pointer stability and real
tracking. Only after this passes, test -5, normal/face transitions, zero and
menu close/reopen. Stop on unexpected movement, bad wand targeting or failure
to restore. Do not publish this diagnostic candidate as a qualified rotator.

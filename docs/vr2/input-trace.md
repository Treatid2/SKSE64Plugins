# VR text-target input trace (diagnostic contract 2)

The native observer in RaceSexMenuVRInput.cpp and AS2 adapter in
tools/vr-racesex-patches/InputTrace.as.inc are default-off. No debugger or new
engine hook is required. Compile and deploy the matching native DLL and loose
VR movie together before use. Contract-2 diagnostic build: package 0.1.35,
native 0.5.0.45. This is not a Filter/Name fix or keyboard qualification.

Use the existing DevBench transport and UI global functions. Verify runtime
identity and CharGen.vrInputTraceContractVersion == 2. Movie owner:
_root.RaceSexMenuBaseInstance.RaceSexPanelsInstance, menu RaceSex Menu.

Call the owner's ArmVRInputTrace and independently read vrInputTraceArmed and
vrInputTraceFault. Arming rejects active colour/text/makeup modals and missing
button listeners. It temporarily disables SearchWidget and routes Filter/Name
click listeners to the existing Zoom handler. This isolates targeting from the
known problematic keyboard backends. Use physical Trigger on Zoom, Filter and
Name in that order, once each, with a pause between controls. Do not invoke
keyboard backends or inject synthetic input during this comparison.

For direct Papyrus calls, use native UI.InvokeBool(menu, functionPath, false)
for these GFx methods; the unused extra Boolean is ignored by the no-argument
AS2 methods. Do not infer successful arming from the Papyrus called/none
acknowledgement: independently read the Boolean and fault properties. Refresh
the UI function signature inventory after a new runtime if needed.

Call DisarmVRInputTrace immediately afterwards. Native ReadVRInputTrace places
retained JSON in _global.skse.plugins.CharGen.vrInputTraceJson. Export that
readout as evidence; verify armed=false, original click listeners and saved
SearchWidget disabled state restored. The AS2 adapter also disarms at 90s and
native recording independently expires at 90s. Menu cleanup attempts disarm.

The first 256 accepted rows are retained; overflow is explicitly counted. Rows
cover raw device-manager button down/up scalars (all devices), RaceSexMenu
CanProcess button entry, native pre-submission packet copies, wrapped existing
movie handlers and movie mouse-listener observations. Raw receipt does not
prove menu consumption. No event pointers are retained by the raw observer;
it always returns continue, never injects or claims events. Sink registration
occurs on explicit arm and removal occurs on end/rollback/menu cleanup outside
the trace mutex. Its process-owned lifetime protects deferred engine removal.
Raw recording independently stops at the native deadline; the movie timer
performs listener removal. Read/status reports registration separately.
Raw/menu/native counters increment independently of buffer drops.
Owner handleInput/onZoomClicked and button underlying press/release methods
are included, so mapped Zoom can be distinguished from a mouse-click chain.
Repeated dispatchEvent:stateChange observations are omitted, not their original
dispatch; ignoredStateChanges is separately reported. Receiver paths are actual handler receivers;
shape membership is not a topmost-target claim. latestNativeEdge is temporal
context, not causal attribution. A read-time mouse snapshot is not an edge-time
sample. Original receiver, arguments, return value and exceptions are preserved.
Observer failures cannot suppress original handlers. Readout is non-destructive.

Native compile/link and final movie compile/re-export passed. Run
node tests/vr-input-trace.test.cjs for the AS2 surrogate preservation,
restoration, deadline and rollback checks. Surrogate results do not prove live
GFx targeting or native-ring overflow. Qualify those against the deployed build
before claiming the trace answers the physical-click question.

Diagnostic package 0.1.36 (native 0.5.0.46) adds a read-time `targetSnapshot` to native
`ReadVRInputTrace`. Its matching movie provides `ReadVRInputTargetSnapshot`;
`targetSnapshotReady=false` explicitly reports a missing/incompatible inspector.
Inspection works without arming and compares a fixed maximum of fourteen clips:
the owner, containers, SearchWidget, added Filter/Name and six stock buttons.
It reports AS2 handler types, visibility, focus flags, depth, root-space bounds,
background bounds, hit-area metadata and geometric centre shape membership.
It never invokes an input handler, changes routing/focus, or opens a keyboard.
Missing fields are null, not proof of false. These are read-time observations,
not evidence of native target registration, topmost selection or a physical click.
Movie inspection and publishing run outside the trace mutex; the mutex is held
only while copying trace state. Optional inspection errors report separately in
`targetSnapshotError` and preserve the independent captured trace. The snapshot
read time is `targetSnapshotReadTickMs` (Windows monotonic system-uptime ticks),
with `targetSnapshotReadDurationUs` from the steady clock. Neither is an input
event timestamp or a performance assay.

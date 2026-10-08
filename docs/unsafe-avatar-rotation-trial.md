# Explicit unsafe avatar rotation trial

This is human-authorised diagnostic functionality, not a qualified rotation fix.
In the View category set **Unsafe avatar trial** from 0 to 1, then adjust
**Avatar rotation**. Default is 0 on every menu opening; it is not saved in an INI,
preset or game save. Ordinary avatar rotation keeps its previous strict guards.

The trial validates the complete avatar subtree's bounded topology, transforms,
spheres and exclusion of tracked headset/menu nodes, then temporarily writes
root-local yaw and rigidly rotated world poses/sphere centers. Non-root local
animation poses, translation/scale, saved actor facing, tracking/UI and ancestor
bounds are not changed. It calls no unqualified composer, skin update or bounds
refit routine. CamSpin and fixed-bound qualification are intentionally bypassed
for this copied-world-pose experiment only. Skinning, shader caches, special
camera nodes and ancestor culling may still be wrong; a crash is possible.

Before each native menu update or close, restore only fields matching the copied
preview on the same branch/parents. Do not overwrite foreign engine/mod writes or
apply old values to a replacement race/sex root. Restoration conflicts latch a
refusal and are logged; this is not a guaranteed recovery path after arbitrary
external writes or a crash. All planned copies are allocated/validated before
the first trial write. No broad animation/collision update is forced.

For a live test start with a small angle (e.g. 10–15 degrees), inspect body, hair
and shadows, then return yaw to 0 and disable the trial before closing. Test
face/normal view, +/-90 and 180 degrees only if the small trial is coherent.
Check close/reopen and race/sex replacement separately. Do not save while a
known bad presentation is active. These are suggested future test actions, not
automated runtime dispatch. Keep crash/hang dumps and restoration diagnostics.

`CaptureDiagnostics().unsafeAvatarTrial` exposes opt-in state, applications,
owned node count, restoration conflicts, bypasses and `rotationQualified:false`.
The existing bound probe continues reporting the STRICT contracts, even while
the experimental trial is active. It is not permission to use the strict path.
Source-contract/math-reference tests do not execute the DLL or establish safety.

## Restoration-conflict observations

The 1418c8a live trial accepted one 14-degree request, then latched a restoration
conflict and reset yaw to zero. That establishes neither visible rotation nor
the identity of another writer. The next instrumentation retains
`unsafeAvatarTrial.lastRestore` without weakening those ownership guards.

The report separates root-local rotation from descendant world rotation,
position, scale, sphere center/radius and parent changes. For the same live root
and parent, totals cover every copied node; detailed samples retain only the
first eight conflicting nodes in traversal order, with names capped at 128 bytes.
Each sample copies baseline, preview and observed values before that node's
guarded restoration. It retains numeric identities, not extra node references.
Serialization reads these copies, not the live scene; non-finite values are null.

`worldMatchesNative`, `boundMatchesNative` and `rootLocalMatchesNative` can suggest
recomposition to the original pose, but do not identify the engine/mod writer.
`writerIdentified` remains false. `intervalMilliseconds` is elapsed time between
application and observation, not a frame count or proof of render timing.
`samplesTruncated` means totals exceed the retained detail; it is not evidence of
an incomplete subtree traversal. Root-parent conflicts and replacement roots
have distinct outcomes and do not inspect/replay the detached branch.

This is the most recent restoration report, not a current ownership assertion.
It remains available after a refusal and is cleared when menu state is restored
on close/reinitialization. The fixed storage and source-contract/reference checks
bound diagnostic growth, but do not prove the native capture's actual size or
runtime correctness. No new update/refit target, guard bypass or animation-state
overwrite is introduced by this instrumentation.

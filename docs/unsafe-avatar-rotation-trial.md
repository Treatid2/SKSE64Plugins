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

Before each native menu update or close, remove the yaw on the same branch.
The entire removal plan is validated before inverse writes: root-local and
root-world preview, unchanged parent world frame, exact original child topology,
finite valid transforms/spheres and unchanged world scale. Matching preview
snapshots restore exactly; recognisable original snapshots are left alone.
Other current descendant poses and positive sphere centers are inverse-turned,
preserving current animation instead of replaying stale snapshots. Non-root
local animation poses and current sphere radii remain untouched.

This **assumes changed descendant world values still use the preview basis**.
The root witnesses do not prove an external writer's convention. An independently
rewritten native-basis pose that does not match its old snapshot can therefore
be inverse-turned incorrectly. `externalWriterBasisQualified:false` explicitly
records that unresolved risk; this remains a default-off, unsafe experiment.
No wider epsilon or unqualified engine update has been used to conceal it.

If the preflight fails, only still-matching copied fields on unchanged parents
are restored; no inverse plan is applied. The refusal latches. Replacement roots
are discarded without writes. This is not guaranteed recovery after arbitrary
external writes or a crash. No broad animation/collision update is forced.

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
overwrite was introduced by that instrumentation alone.

## Animation-preserving removal trial

The 3f075ff observation had one 21-degree application followed by refusal:
root-local still matched the preview, but 31 of 351 descendant world rotations
changed (16 positions also changed), with no parent, scale or bound changes.
The first eight samples were MOV weapon attachment nodes. This is consistent
with animation/attachment updates, not proof of their writer or coordinate basis.

The next trial replaces descendant snapshot equality as the success condition
with the guarded inverse-current-pose policy above. Existing mismatch counts and
samples remain observations, even on successful removal. The new
`animatedWorldsUnturned` and `animatedBoundsUnturned` counts describe the accepted
inverse paths, not a finding that those values were authored by animation.
On refusal these counts are zero because the staged inverse plan was not applied.
The outcome `removed-yaw-current-pose-assumption` is deliberately not a safety claim.

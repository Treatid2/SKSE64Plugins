# Character inspection controls — first native implementation

The existing menu-extension movie support presents a separate **View** category:

- **Avatar rotation**: −180 to +180 degrees, one-degree steps; zero removes the
  preview offset from the native starting pose. This does not promise to correct
  a starting pose supplied sideways by another system.
- **View direction**: −60 to +60 degrees, one-degree steps, using the existing
  tracked-origin yaw implementation. Sculpt's existing buttons remain available.

The category also participates in All, following the existing extension contract.
No SWF, footer layout, build control, saved character angle, INI setting or public
interface layout changes. The two slider values are temporary menu-session state.

## Native lifecycle and scope

The exact Skyrim VR 1.4.15 RaceSexMenu ProcessMessage slot 4 is qualified before
installation and chained exactly once. Before a native update, remove our owned
local preview rotation; after the native update, apply an absolute world-Z turn
to the fresh whole-avatar root. Preserve local position and scale. A replaced
race/sex root acquires a fresh native baseline, never a copied old transform.
Hide, force-hide, close and revert remove owned preview state. Unexpected competing
root transforms refuse further preview changes rather than overwriting another
owner. Face-view framing refreshes on actual rotation/root changes, not idle bones.

Pure VR local-to-world composition and world-bound routines were inspected in the
retained engine snapshot. A bounded, unique, parent-consistent graph is collected
before mutation, and function targets are allowlisted. Propagate transforms without
controllers/collision, then refresh child-to-parent bounds and containing scene
bounds. Foreign dispatch, fixed bounds, malformed transforms or excessive graph
size/depth reject preview rotation and log a warning; these are intentionally not
guessed compatibility paths. Scene updates use the native menu-update stage, not
the rendering hook. The existing avatar-local detail lights turn with the model.
The graph explicitly excludes the tracked room, HMD, menu and pointer quad, so a
mod-altered hierarchy cannot accidentally turn those with the whole avatar.

Extension requests revalidate their originating movie and registration token.
Close/reopen cancels old requests even with address reuse. View yaw no longer drops
the final absolute slider input merely because another input is pending.
Inputs already dispatched on a game task do not enqueue another view-yaw task.
The registry keeps their current values without invalidating the actively dragged
renderer; corrections and external changes still publish a new revision.

## Verification boundaries

Source-contract and mathematical-reference tests are not native execution or
proof of rendered behavior. Broker compilation is separate from installation and
live testing. Live checks still needed: whole-body/back/hair/tail rotation, normal/
face toggles, sculpt and preset transitions, race/sex root replacement, zero reset,
close/reopen, pointer alignment and detail lighting. Unknown-node/fixed-bound
compatibility refusals should be inspected if a live avatar rejects rotation.

Static inspection receipts are retained under
`L:/Codex/analysis/completion-driven-process/runs/`:
`racemenu-avatar-implementation-qualification-20261008`,
`racemenu-avatar-bounds-slots-20261008`, and
`racemenu-avatar-bounds-functions-20261008`.

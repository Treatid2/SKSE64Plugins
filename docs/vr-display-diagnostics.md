# VR display recovery and logging

This native-only change is based on the 0.1.96 source. It retains the exact
runtime movie patch and its source identity; it does not compile or distribute
another SWF. No display outcome is inferred from successful movie loading.

## Operational states

`Character-creation automation state: ready` means the menu is open and has a
movie. It does not mean character creation finished, that polar placement was
applied, or that either headset eye displayed the surface. The message now says
this explicitly. The Address Library version in the startup identity is likewise
labelled as a qualification baseline, not an installed CSV-version measurement.

Each movie load is logged, with a session generation and constructor-time cursor
and offscreen flags. Previously only the first load in a process was logged.
Placement states retain the existing movie contract:

| State | Meaning |
| --- | --- |
| 0 | Pending; the same placement may retry while scene resources become ready. |
| 1 | A validated native placement was applied. Headset visibility is unmeasured. |
| 2 | Unavailable for this request; the log explains the reason. |

Temporary missing nodes, renderer/CPU geometry, valid viewport centre, heading or
parent transforms remain pending. Each logical request has a six-second window,
at most twelve attempts, and at least 250 ms between attempts. The existing movie
normally requests retries every 500 ms. Repeated calls cannot extend the deadline;
the changing artwork centre is not a new logical request. A real view/profile
change starts another request. Closing/reopening starts a new session.

The existing movie stops retrying an unchanged request for any nonzero state.
The native code therefore reserves state 2 for expiry or an ownership/safety
refusal, rather than prematurely caching a missing renderer as unavailable.

Scale, pose, basis and candidate transforms must be finite and nondegenerate.
Both candidate scene transforms are computed and checked before either is
written. A replaced/reparented captured scene or a later external local-pose
change is reported and preserved. The plugin does not repeatedly force another
owner's pose, unhide shared nodes, change cull flags, or assert a compositor fix.
Scene replacement refusals request reopening the menu to reacquire its scene.

## Reading the log

`RaceMenu VR display:` records changes and placement outcomes, with at most
sixteen observation lines per opening. Closing reports the suppressed count.
Unchanged observations do not generate another line. There is no additional
worker, file, engine hook or per-input trace. The existing movie status query
queues at most one generation-checked scene observation per 500 ms, and initial
menu readiness queues one observation independently of the placement request.

Useful fields include session, placement state/reason, last waiting reason,
attempts/elapsed time, requested angles/distance/scale/height/view, UI node and
quad/parent identities, finite world poses, hidden flags, renderer/CPU-vertex and
shader presence, and later identity/parent/local-pose changes. `HiddenAncestry`
values are 0 for no hidden flag found, 1 for a hidden ancestor, and 2 for the
bounded ancestry-inspection limit. A hidden flag, missing shader or changed pose
is an observation, not proof of the cause of a user's invisible headset menu.
`headsetVisibility` is always `not_measured`.

`_global.skse.plugins.CharGen.GetMenuDisplayState()` returns JSON for the latest
native observation in the current menu context. It uses the same rate-limited
observation path. The JSON cache remains current after the log-output cap;
`suppressedObservations` explains why further transitions may not appear in the
file. It is not a new DevBench endpoint or a synchronous rendering fence.

## General debugging, not permanent incident traces

Keep low-volume version, configuration, hook qualification, resource lifecycle,
failure/recovery and cleanup messages. They explain what ran and where a failure
occurred across different mod lists. Keep detailed observations bounded and
change-driven. Never label movie construction as a successful headset render.

The existing default-off input observer remains a separate bounded 90-second,
256-row assay for routing/drag problems. Fixed-issue per-click/per-frame tracing
does not become ordinary logging. Head-asset morph/vertex mismatch errors remain
asset diagnostics; their presence does not establish a display failure.

The standard `RaceMenuNGVR2.log` is replaced when the plugin starts. Preserve a
working and failing log before another launch, with matching winning INIs and a
clear description of whether the desktop image is Skyrim's window or SteamVR
VR View. Public logs can include paths, actor names and process-local addresses;
review those before sharing. A useful log narrows the failure stage; it cannot
replace a headset observation or prove a named mod conflict.

## Validation boundaries

The engine-free regression source exercises the production readiness, ticket,
numeric validation and log-budget policy. It does not simulate GFx/Skyrim scene
ownership or prove a rendered frame. Native compilation, source review, runtime
movie pairing and actual headset testing are separate evidence. The original
PahnDuh report remains an unconfirmed runtime reproduction; this change fixes
the identified retry gap and adds broader display safeguards/observability.

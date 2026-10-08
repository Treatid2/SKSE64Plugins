# Character view diagnostic trial — 8 October 2026

Native-only successor of ad333ba6. No movie, controller settings or saved actor
facing changes. Not a qualified behavioural fix or a release candidate yet.

## View direction

The native extension callback now stages the latest requested slider value.
The existing exact-live-menu update commits after 400 ms without another callback.
Every callback, including an identical value, restarts that quiet window. This is
an experimental debounce, **not a physical trigger-release detector**: a held
pointer that stops sending values can still lead to a commit. No worker, sleeping
game task, second input sink, or uncancellable delayed queue is added.

While pending, the native updater does not overwrite the displayed slider with
the previous applied angle. Menu closure cancels the pending value. An independent
change of applied yaw cancels rather than overwriting that change. UI sign is
inverted consistently on input, initial registration and feedback; native/sculpt
angles keep their existing convention.

Next live checks: arm existing BeginVRInputTrace, click one arrow, then capture
and publish character inspection. Repeat opposite arrow, short drag, longer hold,
zero, Face/Normal, and close/reopen. The click should produce one degree and one
commit after quiet, not a bound. If requests themselves reach a bound before
movement, diagnose the renderer/input path rather than blaming view feedback.
Do not claim release-only acceptance from this experiment.

## Diagnostic coverage

The existing default-off 90-second, 256-row input trace records
`extension_gfx_request`, `extension_game_callback`, and
`view_trial_applied`/`view_trial_rejected`, with requested value and applied native
yaw. Its original drop count/deadline remain authoritative. No trace is armed
automatically. Read/end through the existing API as before.

CaptureVRCharacterInspection then PublishVRCharacterInspection includes a copied
`controls` member in VRCharacterInspectionJSON. It includes requested/applied avatar
yaw, rejection, first failing graph guard from the most recent failed validation,
node identity/name/depth/fixed-bound/vtable and qualified readable virtual targets,
and view trial pending/request/commit/cancel counts. All identities/counters are
strings where precision matters. No live-node dereference is added to JSON readout.
The rejection is also logged once per movie. Avatar safety guards stay unchanged.

HUD suppression is excluded. The previous HUDMovieBaseInstance opacity experiment
did not hide the human-observed marker; its presentation owner remains unknown.

Compilation via Build Broker is distinct from installation and live validation.
Private dirty checkout, source-extension movie enablement, PR75 draft, other
review obligations and migration holds remain unaffected.

# Live RaceMenu diagnostic controls

This is a menu-owned diagnostic API for controlled reproduction, not a direct
write to player or RaceMenu data. The complete add-on pairs the native
DLL with an audited runtime movie patch. Install both together over the
required original RaceMenu assets; do not install the broker's full compiled
`RaceSex_menu.swf` as a loose original movie. See `addon-receipt.json`
and the runtime patch manifest for exact source and payload identities.

The menu is `RaceSex Menu`; the GFx owner is
`_root.RaceSexMenuBaseInstance.RaceSexPanelsInstance`. Invoke methods on that
owner while character creation or `showracemenu` is open. An invocation made
when the menu is closed fails at the UI transport; report that as
`menu-not-open`, not as a successful no-op. With the menu open but on Presets
or Sculpt, the movie returns `sliders-tab-inactive`. During race changes or
slider rebuilds, it returns `race-change-pending` or `sliders-rebuilding`.

1. Call `RefreshVRDiagnosticControls()` (the no-argument call can use the
   existing DevBench `UI.InvokeIntA` convention with an ignored `[0]`). Read
   `vrDiagnosticSnapshotJson` and parse it. Require `status == "ready"` before
   using the listed controls. Read the `generation` number from this snapshot.
2. Call `SetVRDiagnosticSlider(generation, slot, value)` only for a listed,
   enabled slider with `action == "set-slider"`. To change sex, call
   `SelectVRDiagnosticSex(generation, slot, value)` only for a listed slider
   with `action == "select-sex"`. Values must meet the live minimum, maximum
   and interval. The API uses the same menu GameDelegate callback and
   arguments as the slider widget (or its registered extension callback),
   rather than editing actor fields directly.
3. Call `SelectVRDiagnosticRace(generation, raceId)` only for a listed,
   enabled race. It enters the menu's actual `onItemPress` race-selection path,
   including loading state and `ChangeRace`. It will not bypass a hidden or
   disabled Race category.
4. Read `vrDiagnosticResultJson` after a setter. Its `ok` means the menu
   callback was dispatched, **not** that the native change or subsequent
   rebuild completed. Wait for the menu to settle, then refresh and verify
   the new live state. Every query supersedes earlier snapshots, and every
   dispatched action consumes its snapshot before entering the callback.
   Query again after each action. The movie compares the complete live list
   and categories to frozen object/routing/range/value identities; reordered,
   replaced or modified controls return `slider-identity-changed` without
   dispatching. Category, slider-list and
   extension rebuilds, or switching tabs, invalidate it too.

The snapshot has `schema`, `status`, `generation`, `mode`, `sliders`, `races`
and `allCoversEverySlider`. Each slider includes its live `slot`, `id`,
`name`, `callback`, `minimum`, `maximum`, `step`, `value`, `enabled`,
`categoryMask`, `inAll` and `action`. Race entries include `slot`, `id`,
`name`, `active` and `enabled`. The slot is an ephemeral index in the movie's
current item list; never cache it across generations. The API enumerates the
item list, independent of the selected category tab and any currently rendered
rows. It intentionally excludes paint/overlay texture pickers, color-dialogue
actions and sculpt/preset controls, which require different menu workflows.

`All` is **not** a guarantee that every active control is visible. The stock
movie creates `All` with mask `2044`, covering the ordinary body-through-hair
categories but not Race (`2`) or standalone Color/Paint categories. VR
extensions add their category flags to `All` when installed. The snapshot's
`inAll` and `allCoversEverySlider` describe the current actual category masks;
diagnostic callers should not switch to All to discover controls.

Results also include `requestGeneration`, `slot`, `value`, `sliderId`,
`callback` and `raceId` for request correlation (inapplicable numbers are null).
`generation` is the current token, not the consumed request token.

Expected rejections include `snapshot-required`, `slider-identity-changed`,
`stale-generation`, `slider-not-live`,
`slider-not-offered`,
`slider-disabled`, `invalid-value`, `use-select-sex`, `not-sex-slider`,
`race-disabled`, `race-not-live`, `already-selected`, `modal-open`,
`slider-input-disabled`, `sliders-tab-inactive`, `sliders-rebuilding` and
`race-change-pending`. A rejection leaves the menu and actor unchanged.

This API is suitable for a controlled debugger to repeat a race/sex/slider
choice, but it cannot make an intermittent crash deterministic by itself.
Capture the live snapshot, requested action, result, resulting state, and
process/crash evidence together. Do not report `dispatched` as a completed
race change.

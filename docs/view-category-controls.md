# VR rotation controls — category-list replacement

The standalone footer experiment failed live acceptance: its controls did not
respond, and reserving 112 pixels reduced the list and crowded the race footer.
That panel and its geometry reservation are removed. Previous list heights,
race-description positioning and footer layout are restored; no new pointer
surface, world-space extent, widget, timer or input listener is introduced.

**Avatar rotation** and **View direction** use normal native-extension slider
entries and the working stock list renderer/callback route. One pair of backing
entries is shared by the flags of actual enabled categories, appearing before
other sliders in each ordinary category and once in All. Race choices are
excluded; Presets and Sculpt are unchanged. The View category remains available.
Category visibility and the View section's control-ID selection remain respected.
Hidden View categories retain the pair in All and ordinary categories.

The stock numeric priority/item-index sorter places the pair first. Missing
slider priorities become neutral zero; explicit third-party priorities remain
unchanged and the pair is ordered ahead of their lowest finite priority. Backing
entry order, slider IDs, callbacks, ranges and values are not changed. Masks are
recomputed from current categories, not accumulated: race/sex rebuilds and category
removal cannot leave obsolete flags. Unchanged polls do not invalidate snapshots,
recreate renderers or interrupt a held drag. Diagnostic snapshots now freeze
priority and category text-filter identity as well as existing flags and callbacks.

Native avatar lifecycle/restoration checks, the 400ms view-direction quiet window,
intuitive direction and stock endpoint-to-limit behaviour are unchanged. The
Unsafe slider remains removed. Native rotation is not reimplemented in AS2.

## Qualification boundary

Executable glue/diagnostic/layout surrogate checks exercise the actual original
adapter source. They are not AS2 compilation, real GFx input or visual layout
proof. This interpreted recipe needs Broker-owned SWF compilation, owner RMP/pin
reconstruction and exact paired native compilation before live installation.
The previous f6/3aa builds and failed live panel remain separate retained evidence.

The pinned original private movie supplies stock assets. This change does not
activate the separately retained upstream source-extension movie, distribute
upstream source or change its licensing. No installation or publication is part
of this source change.

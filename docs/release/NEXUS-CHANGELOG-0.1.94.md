## 0.1.94

- Added live menu diagnostic controls: query available sliders and their valid
  ranges, then request validated slider, race and sex changes through the
  normal menu callbacks. Stale or unavailable controls return useful errors.
- Diagnostic actions now use single-use snapshots, validate control identities
  before dispatch and reject reordered or changed lists. Results identify the
  requested slider/value so diagnostic tools can correlate their actions.
- Corrected custom head-part slider indexing: -1 means no attachment; 0 is
  the first attachment. Initialization, clearing and range validation now
  use the same convention.
- Included character-creation lighting lifecycle and switch-state hardening.
- Complete update package; retains the previous inventory-preview and
  body-morph fixes. The reported intermittent crashes have not been
  established as RaceMenu-related.

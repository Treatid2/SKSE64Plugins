# Source-built side-loader foundation

Original AS2 extension following the published RaceMenu source's owner and
method contract. It imports no RaceMenu/SkyUI class implementation. The pinned
reference and author suggestion are recorded in `reference.json`.

The compiled child movie is intended to be loaded under
`_root.RaceSexMenuBaseInstance.RaceSexPanelsInstance`. It waits for a compatible
owner for at most 120 frames, wraps list/category/race lifecycle calls, preserves
arguments, return values and receiver, and cooperatively restores its own hooks
on unload. Attach is idempotent. Another module's later wrapper is not removed.

This is a foundation for gradual feature migration, not a replacement for the
current runtime patch. No automatic loader is enabled and no SWF is distributed
yet. It does not expose diagnostics or change controls/layout. Diagnostic review
is a separate draft.

Proposed compiler contract for broker qualification (not a local build fallback):

```
mtasc -version 8 -header 1:1:30 -main -cp ui/VR/SourceExtension -swf <managed-build-output>/RaceMenuVR2Extension.swf ui/VR/SourceExtension/RaceMenuVR2Extension.as
```

The broker must pin the compiler and standard library, compile this exact
source, verify the SWF and retain its receipt. Then live qualification must
check load, owner attachment, callback registration semantics, unload/reopen
and coexistence before migrating any feature or enabling a production loader.
In particular, GameDelegate may have retained pre-hook function references;
these wrappers must not be assumed to observe all registered callbacks until
tested in GFx. Failure must leave the current menu operational.

Source tests: `node --test tests/VR/source-extension.test.cjs`. These exercise
the original include in a mock environment; they do not prove MTASC acceptance
or live Scaleform behaviour. The released 0.1.94 package remains unchanged.

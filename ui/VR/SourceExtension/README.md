# Source-built side-loader foundation

Original AS2 extension following the published RaceMenu source's owner and
method contract. It imports no RaceMenu/SkyUI class implementation. The pinned
reference and author suggestion are recorded in `reference.json`.

## Licensing and provenance

This directory contains our original extension, not Expired's original
RaceMenu menu source. Our extension is covered by the fork's GPL-3.0-or-later
declaration. **We have not relicensed Expired's ActionScript/FLA source, menu
SWFs or assets under GPL-3.0.** Their applicable upstream terms and permissions
remain unchanged.

[Expired's offer](https://github.com/expired6978/SKSE64Plugins/pull/66#issuecomment-5861061752)
suggests a side-loaded extension and provides the original source for reference.
It is not represented here as a GPL grant or blanket permission to redistribute
that source. The original remains in
[Expired's repository](https://github.com/expired6978/skyui/tree/master/src/RaceMenu);
we pin the revision in `reference.json` and do not copy or compile it here.
See [the repository's third-party notices](../../../THIRD_PARTY_NOTICES.md)
for the explicit licensing boundary. Distribution of an original extension
does not grant rights to the upstream menu materials.

## Extension foundation

The compiled child movie is intended to be loaded under
`_root.RaceSexMenuBaseInstance.RaceSexPanelsInstance`. It waits for a compatible
owner for at most 120 frames, wraps list/category/race lifecycle calls, preserves
arguments, return values and receiver, and cooperatively restores its own hooks
on unload. Attach is idempotent. Another module's later wrapper is not removed.

One child can own only one parent lease; it must detach before changing owners.
Optional diagnostic invalidation is exception-contained and guarded against
synchronous re-entry, independently of the original callback's return or error.
Detach releases ownership before notification, restores inherited methods by
removing its instance override, and reduces retained wrappers to inactive
original-function delegation without child/owner/lease references. These are
source/mock-tested guarantees, not a measured live-GFx retention claim.

Attachment sets a child-owned guard before owner getters, prepares all state
before writes, verifies each publication and rechecks the completed transaction.
Failed setup rolls back only still-owned publications. Detach contains each
restoration failure, retires every retained wrapper state and clears child
ownership/guards. Child status reports `attachment-rollback-incomplete` or
`detach-incomplete` if hostile property flags/accessors prevent restoration;
it does not pretend the owner was restored. Such wrappers become inactive
original delegates. An undeletable stale owner lease fails closed until repaired;
the extension never steals another module's lease. Notification eligibility
getter failures also cannot suppress original callbacks.

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
the lifecycle functions from the class source in a mock environment; they do not prove MTASC acceptance
or live Scaleform behaviour. The released 0.1.94 package remains unchanged.

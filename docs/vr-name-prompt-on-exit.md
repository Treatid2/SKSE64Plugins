# Optional name prompt on exit

Add this to `Data/SKSE/Plugins/skee64_custom.ini`, then restart Skyrim:

```ini
[VR]
bPromptForNameOnExit=1
```

The original RaceMenu Done/finish control, including its existing keyboard or
controller binding, still opens the normal confirmation. After confirming, the
option opens the existing VR naming keyboard with the current character name.
Accepting a valid name finishes character creation. Cancelling, an invalid name,
or an unavailable/timed-out keyboard leaves RaceMenu open and preserves the
accepted name. The option applies to the UI exit path, including `showracemenu`.

The default is `0`: finish with the name already shown in the menu. Missing,
blank or invalid values also select this default. The separate Name button still
edits without finishing, and Filter remains independent. A cancelled exit prompt
does not make the next Name or Filter interaction close the menu.

This option uses the runtime keyboard already used by the Name button. It does
not install a keyboard, create a new hotkey, remap controller bindings, or alter
explicit programmatic finish APIs. It requires the matching native plugin and
runtime movie patch; adding the key to release 0.1.97 alone has no effect.

The selected setting is logged once during configuration. Adapter regression
tests execute the production ActionScript bodies with controlled keyboard and
completion callbacks. They cover default/opt-in routing, pending/double requests,
accept/cancel/error/invalid results, unload and subsequent Name/Filter actions.
These tests do not establish live GFx hit testing, headset keyboard behavior or
in-game completion. Compilation and live validation have their own evidence.

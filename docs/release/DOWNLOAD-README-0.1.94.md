# RaceMenu VR 2 — VR beta 0.1.94

Read INSTALLATION.md before installing. This is a complete native add-on
update, not an incremental patch. Original RaceMenu SE 0.4.20.0 assets remain
required: https://www.nexusmods.com/skyrimspecialedition/mods/19080

The exact original menu is verified and transformed automatically in memory.
Install the matching skee64.dll and RaceMenuVR2/racesex-menu.rmp together.
Do not install a generated loose VR RaceSex_menu.swf or leave a legacy
skeevr.dll enabled alongside this add-on.

0.1.94 adds live diagnostic queries and validated slider/race/sex requests
through the menu's own callbacks. It corrects custom attachment sliders to
use -1 for no attachment and 0 for the first real part, and includes lighting
lifecycle and switch-state hardening. All previous add-on features remain.
See RELEASE-NOTES-0.1.94.md and diagnostic-menu-api.md.

The preceding live diagnostic API passed human testing. This release's
snapshot identity correction passed focused offline regression checks; it has
not had an additional headset test. The antler/custom attachment
correction has not been directly tested in-headset. Reported intermittent
crashes have not been established as RaceMenu-related. This is a VR beta,
not an SE/AE replacement or an exhaustive modlist stability qualification.

Both configuration files are used: the original skee64.ini, followed by
skee64_custom.ini overrides. Preserve and merge your configuration on update.
VR Menu Mouse Fix is not required. SteamVR's native quill is enabled by
default; OpenComposite/OCU uses its native pointer path by default. Separate
custom INI settings can change those defaults.

The Sculpt workspace retains one left controls column and a right canvas,
Face View, a larger canvas toggle, and bounded viewpoint rotation without
moving the menu. Previous headset checks covered Sculpt deformation,
History undo/redo, head export/clear/import and preset save/change/load.
Leaving Sculpt resets brush, History and part selection after the stock
warning; cross-session Sculpt persistence is not qualified.

The runtime-neutral VisualEquipment provider API and modder resource header
remain included. The live menu diagnostics are a separate menu-owned surface.

Source fork: https://github.com/Treatid2/SKSE64Plugins
Native code and recipes inherit the upstream GPL licence. Original RaceMenu
assets and user artwork are not relicensed. See LICENSE and
THIRD_PARTY_NOTICES.md. Primary credit belongs to Expired6978.

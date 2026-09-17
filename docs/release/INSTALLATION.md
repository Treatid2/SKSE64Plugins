# Installing the add-on

## Original files and conflicts

1. Download and install RaceMenu SE **0.4.20.0** from Expired's Nexus SE page.
   Keep its BSA, ESPs, scripts and other assets. Enable the original ESPs as
   appropriate for that package. Do not substitute the Skyrim LE download.
2. Install the RaceMenu VR 2 native add-on **after** it in MO2's left pane.
   Our `SKSE/Plugins/skee64.dll` must win the file conflict against its SE DLL.
   Plugin order alone does not control DLL/loose-file precedence.
3. Disable any separately installed legacy RaceMenu VR / SKEEVR binary,
   especially `SKSE/Plugins/skeevr.dll`. A different DLL filename is not
   overwritten by `skee64.dll`. Hide the exact file in MO2 if that mod has
   other required content; never use an invalid/empty dummy DLL.
4. Remove conflicting external VR RaceMenu layout/position fixes from this
   test configuration. Verify which mod wins
   `Interface/VR/RaceSex_menu.swf` in MO2's virtual Data view.

The native add-on ships no original BSA, ESP, scripts, full menu movie,
decompiled upstream class, sample copyrighted artwork, or third-party pointer
DLL. Its custom INI holds only our overrides; the original `skee64.ini` remains
installed. Preserve/merge an existing `skee64_custom.ini` rather than losing
unrelated user settings.

## Automatic in-memory menu patch

The runtime-patch add-on resolves `Interface/VR/RaceSex_menu.swf` through
Skyrim's resource system, so the original can remain inside RaceMenu.bsa.
It verifies this exact input SHA-256 and refuses other versions:

`3A012DA4FED80637CE3257B9B2B89243BEFAB29A4BEC5316A935CEA87C889963`

It reconstructs and verifies the complete patched movie before giving it to
Scaleform. This happens once on the first menu load in each game process;
later opens reuse the immutable verified bytes. No original or generated SWF
is written to disk, so nothing needs undoing when the game closes.

Do **not** install a locally generated menu or external layout-fix SWF with
this add-on: a winning modified loose movie fails the input check. Install
`SKSE/Plugins/RaceMenuVR2/racesex-menu.rmp` together with its matching DLL.
No JPEXS, BSA extraction, launcher step or manual patching is required.

On an unknown original, corrupted/missing patch or unqualified loader adapter,
the menu load is refused and the native plugin log records the reason. A large
on-screen notice identifies an incompatible original menu and explains which
asset package is required and which overrides to disable. Missing/corrupted
patch or initialization failures receive a separate installation notice.
The warning appears at most once per game process. Correct the installation
and restart Skyrim; opening the menu again in the same process is not a retry.
The original SE
DLL is not a compatible VR fallback. See `runtime-swf-patch.md` for the exact
contract. The original-BSA route has passed live menu loading and reopen checks.

## SteamVR and OCU

For SteamVR, install VR Menu Mouse Fix and its documented prerequisites.
Its own `MenuMouseFix.ini` controls its lasers and activation button:
the tested configuration used `ShowLaserPointer=1`, `InvisibleCursor=0`,
`ClickButton=33`. These are helper settings, not RaceMenu VR 2 settings.
Follow the helper's documentation for your controller bindings.

OCU supplies the tested wand lasers itself. Do not describe this project as
providing a universal controller/laser implementation. Other controllers and
runtime combinations still need testing.

## Configuration, updates and removal

Edit `SKSE/Plugins/skee64_custom.ini` while Skyrim is closed. Leave colours
blank to preserve the tested appearance. The INI documents angular world
placement separately from 2D movie offsets, and documents the background fill
ratio **0.79646:1 width:height** (1593×2000; 4:5 is a close practical match).
Any PNG ratio is supported without stretching; unmatched ratios leave margins.

For vertical adjustment, use `fHeightOffset` and `fColorPickerHeightOffset`
in `[Menu Profile VR Normal]`. Positive values raise the respective menu,
negative values lower it (-150..150 Skyrim world units; default 0). These
are relative to the viewpoint captured when the menu opens or when switching
Normal/Face view, not absolute world heights or continuously tracked head
offsets. They are added after angular elevation; both views share the settings.
The picker height is independent of the main menu height. The surface still
faces the captured viewer and its pointer interaction plane moves with it.

On update install the matching DLL and runtime patch together and merge INI
changes. Do not mix patch/native releases. To remove, disable the add-on
and restore a complete, compatible RaceMenu setup for the runtime you intend
to use. The original SE DLL by itself is not a Skyrim VR fallback.

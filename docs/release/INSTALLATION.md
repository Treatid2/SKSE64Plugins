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

## Generate the menu locally

Extract **only** `Interface/VR/RaceSex_menu.swf` from your original RaceMenu.bsa
with a BSA extractor. Leave the original BSA unchanged. The recipe verifies
this input SHA-256 and refuses other versions:

`3A012DA4FED80637CE3257B9B2B89243BEFAB29A4BEC5316A935CEA87C889963`

Download JPEXS 26.2.1 from its official project. Using PowerShell, run the
included recipe (substitute your own absolute paths):

```powershell
pwsh -NoProfile -File .\tools\patch-vr-racesex-swf.ps1 `
  -InputSwf 'C:\original\Interface\VR\RaceSex_menu.swf' `
  -OutputSwf 'C:\My RaceMenu VR Menu\Interface\VR\RaceSex_menu.swf' `
  -FfdecCli 'C:\ffdec\ffdec-cli.exe' `
  -WorkingDirectory 'C:\RaceMenu patch working files'
```

The output must be a **new** file; the original is never overwritten. Keep the
generated movie private. Install the contents of `My RaceMenu VR Menu` as a
local MO2 mod after RaceMenu and the native add-on. The installed relative path
must be `Interface/VR/RaceSex_menu.swf`, not `Data/Interface/...` or another
nested wrapper. This local generated movie is essential; the native-only ZIP
is not sufficient by itself. No online upload of your original/generated
movie is needed. Temporary exports contain original code/artwork and must not
be included in public packages.

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

On update regenerate the local menu with the matching recipe and merge INI
changes. Do not mix movie/native releases. To remove, disable both add-on mods
and restore a complete, compatible RaceMenu setup for the runtime you intend
to use. The original SE DLL by itself is not a Skyrim VR fallback.

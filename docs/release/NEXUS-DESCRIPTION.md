# Nexus page draft — RaceMenu VR 2

**An add-on to Expired's RaceMenu, adapted for Skyrim VR.**

Please download the original RaceMenu SE asset package from Expired's page
first. This project builds from his GPL RaceMenu NG branch; it does not
redistribute the original Nexus assets. Treatid2 maintains this VR fork.

## What it adds

- Working Filter and Name virtual keyboards, with accepted names surviving
  character-creation exit and save/load. No second naming prompt on exit.
- Stable face view and a steady, player-facing menu that reanchors when you
  change between normal and face views.
- Independently positioned colour picker; angular placement, distance and
  size in the INI. Controls and pointer coordinates stay aligned.
- Readable footer controls, cap-aligned translucent slider tracks and reduced
  projected moire.
- Optional PNG backgrounds that preserve their image aspect ratio, plus
  configurable background/text/accent colours and opacity.
- Documented versioned native interfaces for other developers to extend.

## Requirements and installation

Skyrim VR 1.4.15.0, SKSEVR, Skyrim VR Address Library, SkyUI VR and the original
[RaceMenu SE 0.4.20.0](https://www.nexusmods.com/skyrimspecialedition/mods/19080)
assets. **SteamVR additionally requires
[VR Menu Mouse Fix](https://www.nexusmods.com/skyrimspecialedition/mods/33414)
and its dependencies.** OCU uses its own tested wand system.

Install after RaceMenu. Our `skee64.dll` must overwrite the incompatible SE
DLL. Disable any separately installed legacy `skeevr.dll`; it is not covered
by that overwrite. Do not install the older Skyrim LE package as a dependency.

The original menu is patched **locally on your computer** using the supplied
recipe and JPEXS 26.2.1. A BSA extractor is needed to obtain the original movie.
Read `INSTALLATION.md`: the native download alone does not provide the required
patched menu. Original and generated assets must not be reuploaded.

## Credits, source and permissions

Primary credit: **Expired6978**, creator of RaceMenu and its original assets.
Please support him and download the originals from his pages:
[RaceMenu SE](https://www.nexusmods.com/skyrimspecialedition/mods/19080) and
[original Skyrim RaceMenu](https://www.nexusmods.com/skyrim/mods/29624).
The original page's contributor credits remain applicable to its assets.
Thanks to CommonLibSSE-NG, SKSEVR, Address Library, SkyUI VR, OCU and the
VR Menu Mouse Fix authors. Their projects are separate and not bundled here.

Our native code/patch recipes are GPL, inherited from the upstream repository.
That licence does not cover the separately downloaded Nexus assets or custom
user artwork. This is not an official release or implied endorsement by Expired.

Source fork: https://github.com/Treatid2/SKSE64Plugins

**Before publishing, replace this paragraph with the exact release tag/source
archive link and its binary hash. Do not publish a floating-branch source link
as the only corresponding-source reference.**

Nexus Donation Points are intended for Expired. **Confirm the Nexus allocation
with the recipient/account settings before claiming this is configured.**

## Qualification and limitations

The development headset test confirmed the menu layout, image proportions,
normal/face transitions, picker placement, Filter/Name entry and character-name
save/load. The user found SteamVR + VR Menu Mouse Fix workable. This is not
exhaustive coverage of all headsets, controller bindings, presets/sculpt tools,
third-party extensions or SE/AE standalone compatibility. Back up saves.

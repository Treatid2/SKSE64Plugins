# 0.1.89

- Fixed VR preset export and loading for skin tone, complexion, dirt, warpaint
  and every other player tint-mask layer.
- Tint colour, opacity and selected texture are now retained, including
  transparent and textureless entries.
- Added the general, runtime-neutral `VisualEquipment` provider API for mods
  that replace, remap or hide rendered armor without changing inventory.
- Integrated effective equipment mappings into RaceMenu's worn-slot lookup,
  armor-addon traversal and skin-override slot gate.
- Existing presets with `tintInfo: null` must be resaved because their missing
  tint data cannot be reconstructed.

Native version: 0.5.0.99. Runtime qualification pending.

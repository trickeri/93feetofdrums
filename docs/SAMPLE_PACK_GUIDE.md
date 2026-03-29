# VOID Drum Engine - Sample Pack Guide

## Sample Folder Location

After installation, your samples go in:

```
%APPDATA%\VOID Drum Engine\VOID_Samples\
```

On most Windows machines this is:
```
C:\Users\<YourName>\AppData\Roaming\VOID Drum Engine\VOID_Samples\
```

## Folder Structure

The plugin auto-detects samples based on folder names. The installer creates these default categories:

```
VOID_Samples/
  kicks/       - Kick drums, 808s, bass hits
  snares/      - Snares, rimshots, claps
  hats/        - Closed hats, open hats, pedal hats
  percs/       - Shakers, woodblocks, misc percussion
  toms/        - Floor toms, rack toms
  fx/          - Risers, impacts, reverse FX
  user/        - Anything that doesn't fit above
```

## Rules

1. **Any subfolder name becomes a category.** The plugin doesn't care about specific names -- `kicks`, `808s`, `my_kicks`, `basses`, `cymbals` all work. Create whatever folders make sense for your workflow.

2. **Nested subfolders are flattened** into their top-level category. For example, `kicks/heavy/808_01.wav` shows under the "kicks" category with the display name `heavy/808_01`.

3. **Supported formats:** `.wav`, `.flac`, `.ogg`, `.aiff` (16/24/32-bit, any sample rate). Samples are automatically resampled to the host sample rate when loaded.

4. **No filename restrictions.** Spaces, unicode, emojis -- all fine. The display name in the plugin is the filename without the extension.

5. **Hot reload.** Drop files in while the plugin is running. New samples appear in the browser within ~2 seconds. No restart or rescan needed (though a manual Refresh button exists as a fallback).

## Creating a Sample Pack for Distribution

```
1. Create your folder structure:
   my_pack/
     kicks/
     snares/
     hats/

2. Drop your samples into the matching folders.

3. (Optional) Create a kit preset:
   my_pack/my_kit.voidkit

4. Zip it up and distribute:
   my_pack.zip
```

Users extract the zip contents into their `VOID_Samples/` folder and the samples appear automatically.

### Kit Preset Format (.voidkit)

Kit presets are JSON files that map samples to pads:

```json
{
  "name": "MY KIT",
  "author": "YourName",
  "pads": [
    { "pad": 0, "sample": "kicks/808_01.wav", "volume": 0.9, "pan": 0.0, "pitch": 0 },
    { "pad": 1, "sample": "snares/crack_01.wav", "volume": 0.85, "pan": 0.0, "pitch": 0 },
    { "pad": 2, "sample": "hats/closed_tight.wav", "volume": 0.8 }
  ]
}
```

- `pad`: Pad number (0-15)
- `sample`: Relative path from the VOID_Samples root
- `volume`, `pan`, `pitch`: Optional, default to 0.9, 0.0, 0 respectively
- `fx`: Optional object with `drive`, `filter_cutoff`, `filter_resonance`, `bitcrush_depth`, `bitcrush_rate`, `reverb_send`

Place `.voidkit` files in `%APPDATA%\VOID Drum Engine\VOID_Presets\User\` for them to appear in the preset browser.

## Presets Folder Location

```
%APPDATA%\VOID Drum Engine\VOID_Presets\
  Factory/    - Read-only presets shipped with the plugin
  User/       - Your saved presets (.voidkit files)
```

# VOID Drum Engine -- Development Guide

## Build + Package

```powershell
.\package.ps1           # build VST3 + recompile Inno Setup installer
.\package.ps1 -Install  # build + repackage + auto-run installer
```

Or manually:
```powershell
.\build.ps1                                           # 1. Build VST3
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\installer.iss  # 2. Repackage installer
.\dist\VOID_Drum_Engine_Setup_v1.0.0.exe              # 3. Run installer
```

Primary target is Windows VST3. JUCE 7.0.9 is fetched automatically via CMake FetchContent.
VS2022 Community with MSVC compiler is required (cmake is found via VS2022's bundled copy in `build.ps1`).

### CRITICAL: Install workflow

**Always close the DAW before installing.** Ableton (and most DAWs) lock the VST3 DLL while running. Any file copy will silently fail, leaving the old binary in place.

**Always repackage the Inno Setup installer after building.** The `.exe` in `dist/` has the built DLL baked into it at compile time. If you rebuild the VST3 but don't recompile the installer, the `.exe` will install the stale old build. Use `.\package.ps1` to do both steps together.

**Do NOT use `install.ps1` for end-user installs** -- it exists for dev convenience but the Inno Setup `.exe` is the canonical install method and what end users will run.

**Do NOT use PowerShell `Copy-Item -Recurse`** to overwrite VST3 bundles -- it silently fails to replace files inside existing directory structures. Use `robocopy` or bash `cp -f` for direct file replacement if manually installing during development.

## Project layout

- `engine/` -- AudioProcessor, DSP subsystems, voice allocator
- `engine/interfaces/` -- Shared abstract interfaces (ISampleRegistry, IPadState, IKitPreset, IMIDIMapping)
- `ui/` -- AudioProcessorEditor and all UI components
- `samples/` -- Sample drop-folder; subfolders become categories (kicks, snares, hats, etc.)
- `presets/` -- Kit preset files (.voidkit JSON)
- `.github/workflows/` -- CI pipeline

## Conventions

- C++17 throughout. Use `juce::String` for strings, `juce::File` for paths.
- All interface headers live in `engine/interfaces/` and use the `void_drum` namespace.
- Parameter IDs follow the pattern `pad_XX_param` (e.g. `pad_01_volume`, `pad_16_pitch`). Pad indices are 1-based in IDs, 0-based in code.
- The audio thread must never allocate or lock. Use lock-free FIFOs for cross-thread communication.
- Constants: `NUM_PADS = 16`, `MAX_VOICES = 64`.
- Supported sample formats: WAV, FLAC, OGG, AIFF.
- Internal processing: 32-bit float, resampled to host sample rate on load.

## Key design decisions

- 16 pads per bank (expandable to 32 via bank switching in future)
- Sample folder default: `<plugin_data>/VOID_Samples/`
- Preset folder default: `<plugin_data>/VOID_Presets/`
- Maximum polyphony: 64 voices with oldest-note-first stealing
- Presets reference samples by relative path from samples root for portability

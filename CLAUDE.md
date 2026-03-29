# VOID Drum Engine -- Development Guide

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Primary target is Windows VST3. JUCE 7.0.9 is fetched automatically via CMake FetchContent.

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

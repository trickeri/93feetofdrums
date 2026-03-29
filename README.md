# VOID Drum Engine

A hot-swappable, sample-folder-driven drum VST plugin with a dark, glitch-quantum UI.

Built by **93feetofdrums** using JUCE 7+ / C++17.

## Features (planned)

- **Drop-folder architecture** -- place samples in the `/samples` directory and they appear in the plugin instantly, no restart needed
- **16 velocity-sensitive pads** with per-pad FX (filter, drive, bitcrusher, reverb send)
- **Interactive top-down drum kit view** with hit animations
- **Sample browser** with category sorting, search, and drag-and-drop assignment
- **Full MIDI mapping** with learn mode and velocity curves
- **DAW integration** -- every parameter is automatable, full state save/restore
- **Kit presets** stored as portable JSON files

## Building

### Prerequisites

- CMake 3.22+
- C++17 compiler (MSVC 2019+, Clang 12+, GCC 10+)
- Git (JUCE is fetched automatically via CMake FetchContent)

### Build steps

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The VST3 plugin will be output under `build/`.

## Project structure

```
engine/              C++ audio engine (processor, DSP, interfaces)
engine/interfaces/   Shared contract headers between subsystems
ui/                  Plugin editor and UI components
samples/             Sample drop-folder (subfolders = categories)
presets/             Kit preset JSON files (.voidkit)
docs/                Documentation and contributor guides
```

## License

Proprietary -- all rights reserved.

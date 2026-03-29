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

## Quick Start (Windows)

### Option 1: PowerShell (easiest)

```powershell
# Build the plugin
.\build.ps1

# Install to system VST3 folder (requires admin)
.\install.ps1

# Or install to user folder (no admin needed)
.\install.ps1 -UserOnly
```

### Option 2: Manual build

```bash
# Requires Visual Studio 2022 with C++ workload
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Then copy `build\VOIDDrumEngine_artefacts\Release\VST3\VOID Drum Engine.vst3` to `C:\Program Files\Common Files\VST3\`.

### Option 3: Installer (for distribution)

1. Build the plugin first (Option 1 or 2)
2. Install [Inno Setup 6](https://jrsoftware.org/isdl.php)
3. Run: `iscc installer\installer.iss`
4. Distributable installer appears in `dist\`

## After Installation

1. Drop your `.wav` / `.flac` / `.ogg` / `.aiff` samples into the subfolders at:
   `%APPDATA%\VOID Drum Engine\VOID_Samples\` (kicks, snares, hats, percs, toms, fx, user)
2. Open your DAW and scan for new plugins
3. Load **VOID Drum Engine** on a track — samples appear automatically

See [docs/SAMPLE_PACK_GUIDE.md](docs/SAMPLE_PACK_GUIDE.md) for full details on folder structure, hot-reload, creating sample packs, and the kit preset format.

## Prerequisites

- Visual Studio 2022 with "Desktop development with C++" workload
- Git (JUCE 7 is fetched automatically via CMake FetchContent)

## Project Structure

```
engine/              C++ audio engine (processor, DSP, interfaces)
engine/interfaces/   Shared contract headers between subsystems
ui/                  Plugin editor and UI components
samples/             Sample drop-folder (subfolders = categories)
presets/             Kit preset JSON files (.voidkit)
installer/           Inno Setup installer script
docs/                Documentation and contributor guides
```

## License

Proprietary -- all rights reserved.

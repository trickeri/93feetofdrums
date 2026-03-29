# 🥁 VØID DRUM ENGINE — Agent Team Build Plan

### A hot-swappable, sample-folder-driven drum VST with a dark, glitch-quantum UI

---

## 0 — DESIGN DIRECTION (from 93feetofsmoke reference)

The 93feetofsmoke2.com aesthetic is defined by:

| Element | Translation to VST UI |
|---|---|
| **Deep void-black backgrounds** | Main plugin canvas is pure `#000000` or near-black `#0A0A0A` |
| **Stark white typography** | All labels, pad names, and readouts in high-contrast white or off-white |
| **Glitch / quantum motifs** | Subtle scan-line overlays, pixel-shift animations on hit, CRT-style glow on active pads |
| **Anime / manga illustration accents** | Custom illustrated drum kit centerpiece (top-down overhead kit view with illustrated cymbals, snares, toms) |
| **Greek character accents** | Section headers use Greek glyphs as decorative markers (Θ, Σ, Ω) |
| **SD-card / hardware aesthetic** | Sample browser styled like a hardware file selector, monospace readouts |
| **Raw underground energy** | No rounded-corner softness — sharp edges, hard bevels, industrial feel |
| **Occasional neon accent pops** | Electric cyan `#00F0FF` or hot magenta `#FF00AA` for active states and meters |

### Color Palette

```
--void-black:      #000000
--surface:         #0D0D0D
--surface-raised:  #1A1A1A
--surface-hot:     #252525
--text-primary:    #F0F0F0
--text-dim:        #666666
--accent-cyan:     #00F0FF
--accent-magenta:  #FF00AA
--accent-red:      #FF2244
--hit-flash:       #FFFFFF (momentary flash on pad trigger)
--meter-green:     #00FF66
--ghost:           #333333 (grid lines, subtle borders)
```

### Typography
- **Headers / Pad Labels:** Monospace or condensed gothic (e.g., `JetBrains Mono`, `IBM Plex Mono`, or custom pixel font)
- **Readouts / Values:** Tabular monospace for alignment
- **Decorative:** Greek character accents on section dividers

---

## 1 — ARCHITECTURE OVERVIEW

```
VØID DRUM ENGINE
├── /engine              (C++ audio engine — JUCE framework)
│   ├── SampleEngine     (loads, pitches, and triggers samples)
│   ├── FolderScanner    (watches sample directory, hot-reloads)
│   ├── MixBus           (per-pad volume, pan, sends)
│   ├── FXChain          (per-pad: filter, drive, bitcrush, reverb send)
│   └── MIDIMapper       (maps MIDI notes to pads, learns mode)
│
├── /ui                  (JUCE + custom OpenGL/WebView UI)
│   ├── DrumKitView      (interactive illustrated top-down kit)
│   ├── PadGrid          (4x4 MPC-style pads with hit animation)
│   ├── SampleBrowser    (folder-aware, drag-and-drop assignment)
│   ├── MixerStrip       (per-pad faders, pan, mute/solo)
│   ├── FXPanel          (per-pad effects with knobs)
│   └── WaveformDisplay  (current sample waveform + start/end markers)
│
├── /samples             ← THE DROP FOLDER (hot-scanned)
│   ├── /kicks
│   ├── /snares
│   ├── /hats
│   ├── /percs
│   ├── /fx
│   └── /user            (catch-all for uncategorized)
│
├── /presets             (kit mapping snapshots as JSON)
└── /docs                (build instructions, contributor guide)
```

### The "Drop Samples In A Folder" Architecture

This is the core design philosophy. The `/samples` folder is the single source of truth:

1. **FolderScanner** uses a filesystem watcher (`juce::FileSystemWatcher` or OS-native) that polls/watches the `/samples` directory tree
2. On detecting new/removed `.wav`, `.flac`, `.ogg`, or `.aiff` files, it rebuilds an internal **SampleRegistry** — a flat list of `{id, name, category, path, waveformCache}`
3. Subfolder names become **category tags** automatically — drop a file in `/kicks` and it's tagged as a kick
4. The **SampleBrowser** UI reads from SampleRegistry and updates live — no restart, no reimport
5. Preset files (JSON) reference samples by **relative path** from the samples root, so presets are portable as long as the folder structure ships together

```json
// SampleRegistry entry (internal)
{
  "id": "kicks/808_quantum_01.wav",
  "display_name": "808_quantum_01",
  "category": "kicks",
  "absolute_path": "/path/to/samples/kicks/808_quantum_01.wav",
  "sample_rate": 44100,
  "channels": 2,
  "duration_ms": 1420,
  "waveform_cache": [0.0, 0.12, 0.87, "...128 points"]
}
```

```json
// Preset / Kit file (JSON)
{
  "name": "QUANTUMØØN KIT",
  "author": "93feetofsmoke",
  "pads": [
    { "pad": 0, "sample": "kicks/808_quantum_01.wav", "volume": 0.85, "pan": 0.0, "pitch": 0, "fx": { "drive": 0.2, "filter_cutoff": 1.0 }},
    { "pad": 1, "sample": "snares/vinyl_crack_03.wav", "volume": 0.9, "pan": 0.1, "pitch": -2, "fx": {} }
  ]
}
```

---

## 2 — AGENT TEAM BREAKDOWN

Each agent is a specialist with a clear domain boundary. They communicate through well-defined interfaces (header files, JSON schemas, UI component props).

---

### AGENT 1 — 🏗️ ARCHITECT / PROJECT LEAD

**Role:** Sets up the JUCE project scaffold, CMake build system, CI/CD, and defines all inter-agent interfaces.

**Deliverables:**
- JUCE project with `AudioProcessor` and `AudioProcessorEditor` stubs
- CMake configuration for Windows VST3/CLAP + macOS AU/VST3 + Linux VST3
- GitHub Actions CI pipeline (build matrix: Win/Mac/Linux)
- **Interface contracts** as header files:
  - `ISampleRegistry.h` — the shared sample database interface
  - `IPadState.h` — per-pad parameter state (volume, pan, pitch, fx params)
  - `IKitPreset.h` — serialization/deserialization contract for kit files
  - `IMIDIMapping.h` — MIDI note-to-pad routing contract
- Plugin parameter tree layout (JUCE `AudioProcessorValueTreeState`)
- Folder structure creation and README

**Key decisions this agent locks in:**
- 16 pads (expandable to 32 via bank switching)
- Sample folder default location: `<plugin_data>/VOID_Samples/`
- Preset folder: `<plugin_data>/VOID_Presets/`
- Maximum polyphony: 64 voices (configurable)
- Supported formats: WAV, FLAC, OGG, AIFF
- Internal processing: 32-bit float, any sample rate (resampled on load)

---

### AGENT 2 — 🔊 AUDIO ENGINE

**Role:** The DSP core. Loads samples, plays them back with pitch/volume/pan, manages voice allocation.

**Deliverables:**

**SampleLoader**
- Async sample loading with a background thread pool
- Resampling to host sample rate on load (using `juce::ResamplingAudioSource` or libsamplerate)
- Waveform minimap generation (128-point peak array for UI)
- Memory-mapped loading for large sample libraries (optional, for performance)

**VoiceAllocator**
- 64-voice polyphony pool with voice stealing (oldest-note-first)
- Each voice: one-shot or loop mode, ADSR envelope, pitch shift (semitones + fine cents)
- Choke groups (e.g., open hat chokes closed hat)
- Round-robin support (multiple samples per pad, cycled on each hit)

**PadDSP (per-pad chain)**
```
[Sample Playback] → [Pitch Shift] → [Filter (LP/HP/BP)] → [Drive/Saturation]
    → [Bitcrusher] → [Volume/Pan] → [Send to Reverb Bus] → [Main Out]
```

**MixBus**
- 16 pad channels → stereo master bus
- Per-pad: volume, pan, mute, solo
- Global reverb send bus (convolution or algorithmic)
- Master volume + limiter

**Key technical notes:**
- Process audio in `processBlock()` — zero allocation, lock-free
- Use a lock-free FIFO for communicating sample-load events from scanner thread to audio thread
- MIDI velocity → volume + optional velocity-to-pitch mapping

---

### AGENT 3 — 📂 FOLDER SCANNER & SAMPLE REGISTRY

**Role:** The hot-reload brain. Watches the filesystem, maintains the registry, notifies UI and engine of changes.

**Deliverables:**

**FolderScanner**
- Filesystem watcher on the root samples directory (recursive)
- Debounced scan (waits 500ms after last change before rebuilding — prevents thrash during bulk copies)
- Supported file detection by extension (case-insensitive)
- Generates `SampleRegistry` on startup and on any filesystem event

**SampleRegistry**
- Thread-safe read access (UI reads, scanner writes)
- Indexed by relative path (the canonical ID)
- Cached metadata: duration, channel count, sample rate, waveform minimap
- Category derived from parent folder name
- Flat search/filter API for the browser UI

**Preset Manager**
- Load/save JSON kit files
- Validate that referenced samples still exist on load (graceful fallback: mark missing pads)
- "Save As Default Kit" option
- Import/export kits as single ZIP (samples + preset JSON bundled)

**The developer experience:**
```
1. Developer drops "new_kick_01.wav" into /samples/kicks/
2. FolderScanner detects the new file within ~1 second
3. SampleRegistry adds the entry, generates waveform cache
4. SampleBrowser UI updates — new sample appears under "kicks" category
5. User drags it onto a pad — done
```

---

### AGENT 4 — 🎨 UI: INTERACTIVE DRUM KIT VIEW

**Role:** The crown jewel — an illustrated, interactive top-down drum kit that responds to hits.

**Deliverables:**

**DrumKitView (main visual centerpiece)**
- Top-down overhead view of a drum kit rendered as vector/SVG-style graphics
- Components: kick drum (center), snare (front-center), hi-hat (left), 3 toms (across top), ride cymbal (right), crash cymbal (left-top), 2 auxiliary pads (floor)
- Each kit piece is clickable → triggers that pad's sample
- **Hit animation:** on trigger, the piece flashes white (`--hit-flash`) with a radial glow in `--accent-cyan`, then fades back over ~150ms
- **Cymbal shimmer:** cymbals get a wobble animation on hit (CSS transform oscillation)
- **Kick pulse:** kick drum emits a bass ripple ring animation outward
- Pieces visually connected to their assigned pad number via thin ghost-colored lines

**Visual style (93feetofsmoke aligned):**
- Kit pieces rendered as sharp geometric shapes — not photorealistic, more like technical blueprint / wireframe style
- Thin `--accent-cyan` stroke outlines, filled with `--surface-raised`
- Scan-line overlay across the entire kit view (horizontal lines at 2px intervals, 5% opacity)
- Greek letter designators on each piece (Kick = Ω, Snare = Σ, Hat = Θ, etc.)
- Subtle CRT curvature vignette around edges
- Background: grid of faint dots (`--ghost` color) on `--void-black`

**Pad Grid (secondary input — below the kit)**
- 4×4 grid of square pads
- Each pad shows: sample name (truncated), assigned MIDI note, category icon
- Velocity-sensitive: click position maps to velocity zones
- Color-coded by category: kicks = `--accent-red`, snares = `--accent-cyan`, hats = `--accent-magenta`, percs = `--text-dim`
- Active pad has a pulsing border glow
- Drag-and-drop target for samples from the browser

---

### AGENT 5 — 🎨 UI: SAMPLE BROWSER & MIXER

**Role:** The utilitarian panels — browsing samples, mixing levels, tweaking FX.

**Deliverables:**

**SampleBrowser (left panel)**
- Tree view matching the folder structure under `/samples`
- Category headers with sample counts
- Single-click to preview (plays through a dedicated preview bus, not a pad)
- Double-click or drag to assign to selected pad
- Search bar with real-time filter (by filename)
- "Refresh" button (forces rescan) + auto-refresh indicator
- Visual style: monospace font, dark terminal-like aesthetic, scroll with custom thin scrollbar in `--accent-cyan`
- File entries show: name, duration (in ms), tiny inline waveform

**MixerStrip (bottom panel)**
- Horizontal strip of 16 mini channel strips
- Each strip: vertical fader, pan knob, mute/solo buttons, level meter (stereo, peak hold)
- Level meters: `--meter-green` bars on `--surface` background, peak hold line in `--accent-red`
- Selected pad's strip is highlighted with `--accent-cyan` border
- Master fader on the far right

**FX Panel (right panel, context-sensitive)**
- Shows FX for currently selected pad
- Knobs (custom drawn, not default JUCE): Filter Cutoff, Filter Resonance, Drive, Bitcrush Depth, Bitcrush Rate, Reverb Send
- Knob style: thin arc indicator on `--void-black`, value readout in monospace below
- "Bypass" toggle per effect
- Tiny waveform display of the loaded sample with draggable start/end markers

---

### AGENT 6 — 🎹 MIDI & HOST INTEGRATION

**Role:** MIDI mapping, DAW parameter automation, preset management UI.

**Deliverables:**

**MIDI Mapper**
- Default mapping: General MIDI drum map (kick = 36, snare = 38, hat = 42, etc.)
- "MIDI Learn" mode: click a pad, hit a key/pad on controller, mapping saved
- Custom mapping saved per preset
- MIDI velocity → volume (with configurable curve: linear, exponential, S-curve)
- MIDI CC support for knobs (filter cutoff, etc.)

**DAW Integration**
- All per-pad parameters exposed to host automation via `AudioProcessorValueTreeState`
- Parameter naming: `pad_01_volume`, `pad_01_pan`, `pad_01_pitch`, etc.
- DAW state save/restore (full kit state serialized in plugin state)

**Preset Browser (overlay panel)**
- Grid of kit presets with names
- Factory presets ship in a read-only directory
- User presets saved to user-writable directory
- "Init Kit" option to reset all pads
- A/B comparison (two kit slots, toggle between them)

---

## 3 — INTER-AGENT COMMUNICATION MAP

```
                    ┌─────────────┐
                    │  ARCHITECT   │
                    │  (Agent 1)   │
                    └──────┬──────┘
                           │ defines interfaces
          ┌────────────────┼─────────────────┐
          ▼                ▼                  ▼
   ┌──────────┐    ┌──────────────┐   ┌────────────┐
   │  AUDIO   │◄──►│   FOLDER     │   │    MIDI    │
   │  ENGINE  │    │  SCANNER &   │   │  & HOST    │
   │ (Agent 2)│    │  REGISTRY    │   │ (Agent 6)  │
   └────┬─────┘    │  (Agent 3)   │   └─────┬──────┘
        │          └──────┬───────┘         │
        │                 │                 │
        │    ┌────────────┼────────┐        │
        │    ▼            ▼        ▼        │
        │ ┌──────────┐ ┌──────────────┐     │
        └►│  KIT UI  │ │  BROWSER &   │◄────┘
          │ (Agent 4) │ │  MIXER UI    │
          └──────────┘ │  (Agent 5)    │
                       └──────────────┘
```

### Shared data contracts:
- **SampleRegistry** — Agent 3 writes, Agents 2/4/5 read
- **PadState[16]** — Agent 2 owns audio state, Agents 4/5/6 read/write parameters
- **MIDIMapping** — Agent 6 owns, Agent 2 reads for note routing
- **Kit Preset JSON** — Agent 3 serializes/deserializes, all agents consume

---

## 4 — BUILD PHASES

### Phase 1 — Foundation (Week 1-2)
| Agent | Task |
|---|---|
| 1 | JUCE project scaffold, CMake, CI, all interface headers |
| 2 | Basic sample loading + single-voice playback in processBlock |
| 3 | FolderScanner v1 (startup scan, no live watching yet) |
| 6 | Default GM MIDI mapping, basic processBlock MIDI routing |

**Milestone:** Plugin loads in DAW, plays a hardcoded sample on MIDI note 36.

### Phase 2 — Core Playback (Week 3-4)
| Agent | Task |
|---|---|
| 2 | Full voice allocator, pitch shifting, choke groups, per-pad chain |
| 3 | Live filesystem watching, SampleRegistry with metadata caching |
| 4 | Placeholder UI: 4×4 clickable pad grid (no illustrated kit yet) |
| 5 | Sample browser tree view reading from SampleRegistry |

**Milestone:** Drop a sample in the folder → it appears in the browser → drag to pad → play via MIDI or click. Full 16-pad playback working.

### Phase 3 — FX & Mixing (Week 5-6)
| Agent | Task |
|---|---|
| 2 | Per-pad FX chain (filter, drive, bitcrusher), reverb send bus, master limiter |
| 5 | Mixer strip UI, FX knob panel, waveform display with start/end markers |
| 6 | MIDI Learn mode, velocity curves, DAW automation parameters |

**Milestone:** Full signal chain working. Mix a kit with per-pad FX. Automate parameters in DAW.

### Phase 4 — UI Polish & Kit View (Week 7-9)
| Agent | Task |
|---|---|
| 4 | Full illustrated drum kit view with hit animations, glow effects, scan-line overlay |
| 5 | Search, preview playback, drag-and-drop refinement |
| 1 | Preset system integration, factory preset creation, ZIP export |
| ALL | Visual polish pass: 93feetofsmoke color scheme, typography, CRT effects |

**Milestone:** Plugin looks and feels like a finished product. Illustrated kit is the wow factor.

### Phase 5 — Ship It (Week 10)
| Agent | Task |
|---|---|
| 1 | Installer creation (Windows NSIS + macOS pkg), code signing |
| ALL | Cross-platform QA, DAW compatibility testing (Ableton, FL, Logic, Reaper, Bitwig) |
| ALL | Performance profiling (CPU usage target: <5% idle, <15% with all pads active) |
| 3 | Write "Developer Sample Pack Guide" (how to structure folders for distribution) |

---

## 5 — SAMPLE FOLDER SPEC (DEVELOPER-FACING)

This is what makes the plugin easy to extend. Developers / sound designers just follow this folder convention:

```
VOID_Samples/
├── kicks/
│   ├── 808_deep.wav
│   ├── 808_punch.wav
│   └── acoustic_kick.wav
├── snares/
│   ├── trap_snare_01.wav
│   ├── vinyl_crack.wav
│   └── rimshot_dry.wav
├── hats/
│   ├── closed_hat_tight.wav
│   ├── open_hat_shimmer.wav
│   └── pedal_hat.wav
├── percs/
│   ├── shaker_loop.wav
│   └── woodblock.wav
├── fx/
│   ├── riser_01.wav
│   └── impact_reverse.wav
├── toms/
│   ├── floor_tom.wav
│   └── rack_tom_high.wav
└── user/
    └── (anything goes here — shows as "user" category)
```

### Rules:
1. **Any subfolder name = a category.** The scanner doesn't care about specific names — `kicks`, `808s`, `my_kicks`, and `basses` all work as categories.
2. **Nested subfolders are flattened** into their top-level category. `kicks/heavy/808_01.wav` shows under "kicks" with the display name `heavy/808_01`.
3. **Supported formats:** `.wav`, `.flac`, `.ogg`, `.aiff` (16/24/32-bit, any sample rate)
4. **No filename restrictions.** Spaces, unicode, emojis — all fine. The display name is the filename minus extension.
5. **Hot reload.** Drop files in while the plugin is running. No restart, no rescan button needed (though one exists as a fallback).
6. **Kit preset bundles** can ship as a ZIP containing a `/samples` subtree + a `.voidkit` JSON preset file. The plugin's import function extracts samples to the right folders and loads the preset.

---

## 6 — TECH STACK SUMMARY

| Component | Technology |
|---|---|
| **Framework** | JUCE 7+ (C++17) |
| **Build** | CMake + JUCE CMake API |
| **Audio formats** | JUCE built-in (WAV, AIFF, FLAC, OGG) |
| **DSP** | JUCE DSP module + custom (filter, drive, bitcrusher) |
| **UI rendering** | JUCE Component system + custom `paint()` + OpenGL for kit view |
| **Filesystem watch** | `juce::FileSystemWatcher` or platform-native (inotify / FSEvents / ReadDirectoryChangesW) |
| **Preset format** | JSON (via `juce::JSON` or nlohmann/json) |
| **Plugin formats** | VST3, AU (macOS), CLAP (stretch goal) |
| **CI/CD** | GitHub Actions (build + artifact upload) |
| **Installer** | NSIS (Windows), pkgbuild (macOS) |

---

## 7 — UI WIREFRAME LAYOUT

```
┌─────────────────────────────────────────────────────────────────┐
│  Θ  V Ø I D   D R U M   E N G I N E              [A/B] [☰]   │
├────────────┬─────────────────────────────────┬──────────────────┤
│            │                                 │                  │
│  SAMPLE    │     INTERACTIVE DRUM KIT        │    FX PANEL      │
│  BROWSER   │     (illustrated top-down       │                  │
│            │      overhead view)              │  [FILTER ◎────]  │
│  🔍 search │                                 │  [DRIVE  ◎────]  │
│            │        ◯ crash    ◯ ride        │  [CRUSH  ◎────]  │
│  ▸ kicks   │      ◯ hat   ◯tom ◯tom ◯tom   │  [REVERB ◎────]  │
│    808_01  │           ◉ snare               │                  │
│    808_02  │         ◉◉◉ kick ◉◉◉           │  ┌────────────┐  │
│  ▸ snares  │                                 │  │ ∿∿∿∿∿∿∿∿∿∿ │  │
│  ▸ hats    │                                 │  │  waveform   │  │
│  ▸ percs   │                                 │  └────────────┘  │
│  ▸ fx      │                                 │                  │
│            ├─────────────────────────────────┤                  │
│            │  PAD GRID (4×4)                 │                  │
│            │  ┌──┬──┬──┬──┐                  │                  │
│            │  │Ω1│Σ2│Θ3│◊4│                  │                  │
│            │  ├──┼──┼──┼──┤                  │                  │
│            │  │◊5│◊6│◊7│◊8│                  │                  │
│            │  ├──┼──┼──┼──┤                  │                  │
│            │  │◊9│10│11│12│                  │                  │
│            │  ├──┼──┼──┼──┤                  │                  │
│            │  │13│14│15│16│                  │                  │
│            │  └──┴──┴──┴──┘                  │                  │
├────────────┴─────────────────────────────────┴──────────────────┤
│  MIXER  [1▕▎][2▕▎][3▕▎][4▕▎][5▕▎] ... [16▕▎]  ║  [M▕████▎]  │
│         -12  -6   0   -3   -∞                   ║  MASTER      │
└─────────────────────────────────────────────────────────────────┘
```

---

## 8 — RISK REGISTER

| Risk | Mitigation |
|---|---|
| Filesystem watcher misses events on some OS | Fallback: periodic rescan every 5s as supplement |
| Large sample folders cause slow startup | Async loading + cached registry (persist to disk, skip unchanged files via modified-time check) |
| OpenGL kit view breaks on certain GPUs | Software rendering fallback for kit view |
| Thread safety between scanner and audio | Lock-free FIFO for registry updates; audio thread never blocks |
| Preset references missing samples | Graceful UI: pad shows ⚠️ icon, still loads rest of kit |
| Cross-platform path issues | Always use `juce::File` abstractions, never raw string paths |

---

## 9 — DEVELOPER ONBOARDING (FOR SAMPLE PACK CREATORS)

To create a new pack for VØID DRUM ENGINE:

```bash
# 1. Create your folder structure
mkdir -p my_pack/{kicks,snares,hats,percs,fx}

# 2. Drop your samples in
cp my_808_samples/*.wav my_pack/kicks/
cp my_snare_samples/*.wav my_pack/snares/

# 3. Create an optional preset
cat > my_pack/my_kit.voidkit << 'EOF'
{
  "name": "MY FIRE KIT",
  "author": "YourName",
  "pads": [
    { "pad": 0, "sample": "kicks/808_01.wav", "volume": 0.9 },
    { "pad": 1, "sample": "snares/crack_01.wav", "volume": 0.85 }
  ]
}
EOF

# 4. Zip it up
zip -r my_pack.zip my_pack/

# 5. Users drag the zip onto the plugin or extract to VOID_Samples/
```

That's it. No build tools, no compilation, no config files beyond the optional preset JSON. Drop files in folders, and the engine picks them up.

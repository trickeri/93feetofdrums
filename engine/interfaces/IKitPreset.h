#pragma once

#include "IPadState.h"
#include <juce_core/juce_core.h>
#include <string>
#include <vector>

namespace void_drum {

// -------------------------------------------------------------------------
// KitPreset: a serialisable snapshot of a full drum kit configuration.
//
// Preset files are stored as JSON with the ".voidkit" extension.
// Sample references use the canonical relative-path ID so that presets
// are portable alongside their sample folders.
// -------------------------------------------------------------------------
struct KitPreset
{
    /** Display name of the kit (e.g. "QUANTUMOON KIT"). */
    juce::String name;

    /** Author / creator name. */
    juce::String author;

    /** Per-pad state for all NUM_PADS pads. */
    PadBank pads {};
};

// -------------------------------------------------------------------------
// IKitPresetManager: abstract interface for loading, saving, and
// enumerating kit presets.
//
// Agent 3 (Folder Scanner & Preset Manager) provides the concrete
// implementation.
// -------------------------------------------------------------------------
class IKitPresetManager
{
public:
    virtual ~IKitPresetManager() = default;

    /** Load a preset from the given file. Returns true on success. */
    virtual bool loadPreset(const juce::File& presetFile, KitPreset& outPreset) = 0;

    /** Save a preset to the given file. Returns true on success. */
    virtual bool savePreset(const juce::File& presetFile, const KitPreset& preset) = 0;

    /** Return a list of all discovered preset files (factory + user). */
    virtual std::vector<juce::File> getPresetList() const = 0;
};

} // namespace void_drum

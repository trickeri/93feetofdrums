#pragma once

#include "interfaces/IKitPreset.h"
#include "interfaces/ISampleRegistry.h"
#include <juce_core/juce_core.h>
#include <vector>

namespace void_drum {

// =========================================================================
// PresetManager
//
// Concrete implementation of IKitPresetManager. Handles load/save of
// .voidkit JSON files, enumerates factory and user preset directories,
// and validates sample references against the SampleRegistry.
// =========================================================================
class PresetManager : public IKitPresetManager
{
public:
    /** Construct with a reference to the registry for sample validation. */
    explicit PresetManager(const ISampleRegistry& registry);
    ~PresetManager() override;

    // -- IKitPresetManager interface ---------------------------------------
    bool loadPreset(const juce::File& presetFile, KitPreset& outPreset) override;
    bool savePreset(const juce::File& presetFile, const KitPreset& preset) override;
    std::vector<juce::File> getPresetList() const override;

    // -- Directory configuration -------------------------------------------
    /** Set the factory presets directory (read-only). */
    void setFactoryPresetsDirectory(const juce::File& dir);

    /** Set the user presets directory (read-write). */
    void setUserPresetsDirectory(const juce::File& dir);

    juce::File getFactoryPresetsDirectory() const { return factoryPresetsDir; }
    juce::File getUserPresetsDirectory() const { return userPresetsDir; }

    // -- Default kit -------------------------------------------------------
    /** Save a kit as the default (loaded automatically on startup). */
    bool saveAsDefault(const KitPreset& preset);

    /** Load the default kit if it exists. Returns true on success. */
    bool loadDefault(KitPreset& outPreset);

    /** Check if a default kit file exists. */
    bool hasDefaultKit() const;

    // -- Validation --------------------------------------------------------
    struct ValidationResult
    {
        bool valid = true;
        /** Pad indices (0-based) where the referenced sample was not found. */
        std::vector<int> missingSamplePads;
    };

    /** Validate that all sample references in a preset exist in the registry. */
    ValidationResult validatePreset(const KitPreset& preset) const;

    // -- Import / Export (stubs for stretch goal) --------------------------
    /** Import a .zip bundle containing samples + preset. Returns true on success. */
    bool importBundle(const juce::File& zipFile);

    /** Export a preset + its referenced samples as a .zip bundle. */
    bool exportBundle(const juce::File& outputZip, const KitPreset& preset);

private:
    /** Collect all .voidkit files from a directory (non-recursive). */
    static std::vector<juce::File> collectPresetFiles(const juce::File& dir);

    /** Serialize a KitPreset to a juce::var (JSON-ready). */
    static juce::var presetToVar(const KitPreset& preset);

    /** Deserialize a juce::var into a KitPreset. Returns true on success. */
    static bool varToPreset(const juce::var& data, KitPreset& outPreset);

    const ISampleRegistry& sampleRegistry;
    juce::File factoryPresetsDir;
    juce::File userPresetsDir;

    static constexpr const char* defaultKitFilename = "default.voidkit";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetManager)
};

} // namespace void_drum

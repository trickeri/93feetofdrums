#include "PresetManager.h"

namespace void_drum {

// =========================================================================
// Construction
// =========================================================================

PresetManager::PresetManager(const ISampleRegistry& registry)
    : sampleRegistry(registry)
{
}

PresetManager::~PresetManager() = default;

// =========================================================================
// IKitPresetManager interface
// =========================================================================

bool PresetManager::loadPreset(const juce::File& presetFile, KitPreset& outPreset)
{
    if (!presetFile.existsAsFile())
        return false;

    auto text = presetFile.loadFileAsString();
    auto parsed = juce::JSON::parse(text);

    if (!varToPreset(parsed, outPreset))
        return false;

    // Validate sample references and mark missing pads
    auto validation = validatePreset(outPreset);
    for (int padIdx : validation.missingSamplePads)
    {
        // Leave the sample ID in place but the caller can check validation
        // to show warning indicators in the UI
        DBG("PresetManager: missing sample for pad " + juce::String(padIdx)
            + ": " + outPreset.pads[static_cast<size_t>(padIdx)].assignedSampleId);
    }

    return true;
}

bool PresetManager::savePreset(const juce::File& presetFile, const KitPreset& preset)
{
    auto jsonVar = presetToVar(preset);
    auto jsonText = juce::JSON::toString(jsonVar);

    // Ensure the parent directory exists
    presetFile.getParentDirectory().createDirectory();

    return presetFile.replaceWithText(jsonText);
}

std::vector<juce::File> PresetManager::getPresetList() const
{
    std::vector<juce::File> result;

    // Factory presets first
    auto factory = collectPresetFiles(factoryPresetsDir);
    result.insert(result.end(), factory.begin(), factory.end());

    // Then user presets
    auto user = collectPresetFiles(userPresetsDir);
    result.insert(result.end(), user.begin(), user.end());

    return result;
}

// =========================================================================
// Directory configuration
// =========================================================================

void PresetManager::setFactoryPresetsDirectory(const juce::File& dir)
{
    factoryPresetsDir = dir;
}

void PresetManager::setUserPresetsDirectory(const juce::File& dir)
{
    userPresetsDir = dir;
    dir.createDirectory(); // ensure user dir exists
}

// =========================================================================
// Default kit
// =========================================================================

bool PresetManager::saveAsDefault(const KitPreset& preset)
{
    if (userPresetsDir == juce::File())
        return false;

    auto defaultFile = userPresetsDir.getChildFile(defaultKitFilename);
    return savePreset(defaultFile, preset);
}

bool PresetManager::loadDefault(KitPreset& outPreset)
{
    if (userPresetsDir == juce::File())
        return false;

    auto defaultFile = userPresetsDir.getChildFile(defaultKitFilename);
    return loadPreset(defaultFile, outPreset);
}

bool PresetManager::hasDefaultKit() const
{
    if (userPresetsDir == juce::File())
        return false;
    return userPresetsDir.getChildFile(defaultKitFilename).existsAsFile();
}

// =========================================================================
// Validation
// =========================================================================

PresetManager::ValidationResult PresetManager::validatePreset(const KitPreset& preset) const
{
    ValidationResult result;
    for (int i = 0; i < NUM_PADS; ++i)
    {
        const auto& sampleId = preset.pads[static_cast<size_t>(i)].assignedSampleId;
        if (sampleId.isNotEmpty())
        {
            if (sampleRegistry.findSample(sampleId) == nullptr)
            {
                result.valid = false;
                result.missingSamplePads.push_back(i);
            }
        }
    }
    return result;
}

// =========================================================================
// Import / Export stubs (stretch goal)
// =========================================================================

bool PresetManager::importBundle(const juce::File& /*zipFile*/)
{
    // TODO: Implement ZIP import
    // 1. Extract ZIP to temp directory
    // 2. Find .voidkit file inside
    // 3. Copy samples to appropriate category folders in samples root
    // 4. Load the preset
    jassertfalse; // not yet implemented
    return false;
}

bool PresetManager::exportBundle(const juce::File& /*outputZip*/,
                                 const KitPreset& /*preset*/)
{
    // TODO: Implement ZIP export
    // 1. Collect all referenced sample files
    // 2. Create ZIP with samples/ subfolder + .voidkit file
    jassertfalse; // not yet implemented
    return false;
}

// =========================================================================
// File collection
// =========================================================================

std::vector<juce::File> PresetManager::collectPresetFiles(const juce::File& dir)
{
    std::vector<juce::File> results;
    if (!dir.isDirectory())
        return results;

    for (const auto& entry : juce::RangedDirectoryIterator(
             dir, false, "*.voidkit", juce::File::findFiles))
    {
        results.push_back(entry.getFile());
    }

    // Sort alphabetically by filename
    std::sort(results.begin(), results.end(),
              [](const juce::File& a, const juce::File& b)
              {
                  return a.getFileName().compareIgnoreCase(b.getFileName()) < 0;
              });

    return results;
}

// =========================================================================
// JSON serialization
// =========================================================================

juce::var PresetManager::presetToVar(const KitPreset& preset)
{
    auto* root = new juce::DynamicObject();
    root->setProperty("name", preset.name);
    root->setProperty("author", preset.author);

    juce::Array<juce::var> padsArr;

    for (int i = 0; i < NUM_PADS; ++i)
    {
        const auto& pad = preset.pads[static_cast<size_t>(i)];

        // Only serialize pads that have a sample assigned
        if (pad.assignedSampleId.isEmpty())
            continue;

        auto* padObj = new juce::DynamicObject();
        padObj->setProperty("pad", i);
        padObj->setProperty("sample", pad.assignedSampleId);
        padObj->setProperty("volume", static_cast<double>(pad.volume));
        padObj->setProperty("pan", static_cast<double>(pad.pan));
        padObj->setProperty("pitch", static_cast<double>(pad.pitch));

        // FX sub-object
        auto* fxObj = new juce::DynamicObject();
        fxObj->setProperty("drive", static_cast<double>(pad.drive));
        fxObj->setProperty("filter_cutoff", static_cast<double>(pad.filterCutoff));
        fxObj->setProperty("filter_resonance", static_cast<double>(pad.filterResonance));
        fxObj->setProperty("bitcrush_depth", static_cast<double>(pad.bitcrushDepth));
        fxObj->setProperty("bitcrush_rate", static_cast<double>(pad.bitcrushRate));
        fxObj->setProperty("reverb_send", static_cast<double>(pad.reverbSend));
        padObj->setProperty("fx", juce::var(fxObj));

        // Routing
        padObj->setProperty("mute", pad.mute);
        padObj->setProperty("solo", pad.solo);
        padObj->setProperty("choke_group", pad.chokeGroup);

        padsArr.add(juce::var(padObj));
    }

    root->setProperty("pads", padsArr);
    return juce::var(root);
}

bool PresetManager::varToPreset(const juce::var& data, KitPreset& outPreset)
{
    auto* root = data.getDynamicObject();
    if (root == nullptr)
        return false;

    outPreset = KitPreset {}; // reset to defaults

    outPreset.name = root->getProperty("name").toString();
    outPreset.author = root->getProperty("author").toString();

    auto* padsArr = root->getProperty("pads").getArray();
    if (padsArr == nullptr)
        return false;

    for (const auto& padVar : *padsArr)
    {
        auto* padObj = padVar.getDynamicObject();
        if (padObj == nullptr)
            continue;

        int padIndex = static_cast<int>(padObj->getProperty("pad"));
        if (padIndex < 0 || padIndex >= NUM_PADS)
            continue;

        auto& pad = outPreset.pads[static_cast<size_t>(padIndex)];
        pad.assignedSampleId = padObj->getProperty("sample").toString();
        pad.volume = static_cast<float>(padObj->getProperty("volume"));
        pad.pan = static_cast<float>(padObj->getProperty("pan"));
        pad.pitch = static_cast<float>(padObj->getProperty("pitch"));

        // Parse FX sub-object if present
        auto fxVar = padObj->getProperty("fx");
        if (auto* fxObj = fxVar.getDynamicObject())
        {
            if (fxObj->hasProperty("drive"))
                pad.drive = static_cast<float>(fxObj->getProperty("drive"));
            if (fxObj->hasProperty("filter_cutoff"))
                pad.filterCutoff = static_cast<float>(fxObj->getProperty("filter_cutoff"));
            if (fxObj->hasProperty("filter_resonance"))
                pad.filterResonance = static_cast<float>(fxObj->getProperty("filter_resonance"));
            if (fxObj->hasProperty("bitcrush_depth"))
                pad.bitcrushDepth = static_cast<float>(fxObj->getProperty("bitcrush_depth"));
            if (fxObj->hasProperty("bitcrush_rate"))
                pad.bitcrushRate = static_cast<float>(fxObj->getProperty("bitcrush_rate"));
            if (fxObj->hasProperty("reverb_send"))
                pad.reverbSend = static_cast<float>(fxObj->getProperty("reverb_send"));
        }

        // Routing properties
        if (padObj->hasProperty("mute"))
            pad.mute = static_cast<bool>(padObj->getProperty("mute"));
        if (padObj->hasProperty("solo"))
            pad.solo = static_cast<bool>(padObj->getProperty("solo"));
        if (padObj->hasProperty("choke_group"))
            pad.chokeGroup = static_cast<int>(padObj->getProperty("choke_group"));
    }

    return true;
}

} // namespace void_drum

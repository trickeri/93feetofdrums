#include "HostIntegration.h"

namespace void_drum {

// =========================================================================
// Helpers
// =========================================================================

juce::String HostIntegration::padParamId(int padIndex, const juce::String& suffix)
{
    return "pad_" + juce::String(padIndex + 1).paddedLeft('0', 2) + "_" + suffix;
}

// =========================================================================
// Construction / destruction
// =========================================================================

HostIntegration::HostIntegration(juce::AudioProcessor& proc,
                                 juce::AudioProcessorValueTreeState& tree,
                                 MIDIMapper& mapper)
    : processor(proc), apvts(tree), midiMapper(mapper)
{
    // Cache raw parameter pointers for lock-free audio-thread access.
    for (int i = 0; i < NUM_PADS; ++i)
    {
        padCache[static_cast<size_t>(i)].volume       = apvts.getRawParameterValue(padParamId(i, "volume"));
        padCache[static_cast<size_t>(i)].pan          = apvts.getRawParameterValue(padParamId(i, "pan"));
        padCache[static_cast<size_t>(i)].pitch        = apvts.getRawParameterValue(padParamId(i, "pitch"));
        padCache[static_cast<size_t>(i)].filterCutoff = apvts.getRawParameterValue(padParamId(i, "filter_cutoff"));
        padCache[static_cast<size_t>(i)].filterRes    = apvts.getRawParameterValue(padParamId(i, "filter_resonance"));
        padCache[static_cast<size_t>(i)].drive        = apvts.getRawParameterValue(padParamId(i, "drive"));
        padCache[static_cast<size_t>(i)].crushDepth   = apvts.getRawParameterValue(padParamId(i, "bitcrush_depth"));
        padCache[static_cast<size_t>(i)].crushRate    = apvts.getRawParameterValue(padParamId(i, "bitcrush_rate"));
        padCache[static_cast<size_t>(i)].reverbSend   = apvts.getRawParameterValue(padParamId(i, "reverb_send"));
    }
    masterVolumePtr = apvts.getRawParameterValue("master_volume");

    // Register as listener for all parameters (for future change tracking)
    for (int i = 0; i < NUM_PADS; ++i)
    {
        apvts.addParameterListener(padParamId(i, "volume"), this);
        apvts.addParameterListener(padParamId(i, "pan"), this);
        apvts.addParameterListener(padParamId(i, "pitch"), this);
        apvts.addParameterListener(padParamId(i, "filter_cutoff"), this);
        apvts.addParameterListener(padParamId(i, "filter_resonance"), this);
        apvts.addParameterListener(padParamId(i, "drive"), this);
        apvts.addParameterListener(padParamId(i, "bitcrush_depth"), this);
        apvts.addParameterListener(padParamId(i, "bitcrush_rate"), this);
        apvts.addParameterListener(padParamId(i, "reverb_send"), this);
    }
    apvts.addParameterListener("master_volume", this);

    // Initialise slot A with current state
    slotA = captureCurrentState();
}

HostIntegration::~HostIntegration()
{
    // Remove all listeners
    for (int i = 0; i < NUM_PADS; ++i)
    {
        apvts.removeParameterListener(padParamId(i, "volume"), this);
        apvts.removeParameterListener(padParamId(i, "pan"), this);
        apvts.removeParameterListener(padParamId(i, "pitch"), this);
        apvts.removeParameterListener(padParamId(i, "filter_cutoff"), this);
        apvts.removeParameterListener(padParamId(i, "filter_resonance"), this);
        apvts.removeParameterListener(padParamId(i, "drive"), this);
        apvts.removeParameterListener(padParamId(i, "bitcrush_depth"), this);
        apvts.removeParameterListener(padParamId(i, "bitcrush_rate"), this);
        apvts.removeParameterListener(padParamId(i, "reverb_send"), this);
    }
    apvts.removeParameterListener("master_volume", this);
}

// =========================================================================
// Audio-thread parameter reads
// =========================================================================

void HostIntegration::updateParameterCache()
{
    // No-op: we read directly from APVTS atomic pointers.
    // This method exists for future batch-update optimisations.
}

float HostIntegration::getPadVolume(int i) const
{
    return (i >= 0 && i < NUM_PADS && padCache[static_cast<size_t>(i)].volume)
               ? padCache[static_cast<size_t>(i)].volume->load(std::memory_order_relaxed)
               : 0.8f;
}

float HostIntegration::getPadPan(int i) const
{
    return (i >= 0 && i < NUM_PADS && padCache[static_cast<size_t>(i)].pan)
               ? padCache[static_cast<size_t>(i)].pan->load(std::memory_order_relaxed)
               : 0.0f;
}

float HostIntegration::getPadPitch(int i) const
{
    return (i >= 0 && i < NUM_PADS && padCache[static_cast<size_t>(i)].pitch)
               ? padCache[static_cast<size_t>(i)].pitch->load(std::memory_order_relaxed)
               : 0.0f;
}

float HostIntegration::getPadFilterCutoff(int i) const
{
    return (i >= 0 && i < NUM_PADS && padCache[static_cast<size_t>(i)].filterCutoff)
               ? padCache[static_cast<size_t>(i)].filterCutoff->load(std::memory_order_relaxed)
               : 1.0f;
}

float HostIntegration::getPadFilterResonance(int i) const
{
    return (i >= 0 && i < NUM_PADS && padCache[static_cast<size_t>(i)].filterRes)
               ? padCache[static_cast<size_t>(i)].filterRes->load(std::memory_order_relaxed)
               : 0.0f;
}

float HostIntegration::getPadDrive(int i) const
{
    return (i >= 0 && i < NUM_PADS && padCache[static_cast<size_t>(i)].drive)
               ? padCache[static_cast<size_t>(i)].drive->load(std::memory_order_relaxed)
               : 0.0f;
}

float HostIntegration::getPadBitcrushDepth(int i) const
{
    return (i >= 0 && i < NUM_PADS && padCache[static_cast<size_t>(i)].crushDepth)
               ? padCache[static_cast<size_t>(i)].crushDepth->load(std::memory_order_relaxed)
               : 1.0f;
}

float HostIntegration::getPadBitcrushRate(int i) const
{
    return (i >= 0 && i < NUM_PADS && padCache[static_cast<size_t>(i)].crushRate)
               ? padCache[static_cast<size_t>(i)].crushRate->load(std::memory_order_relaxed)
               : 1.0f;
}

float HostIntegration::getPadReverbSend(int i) const
{
    return (i >= 0 && i < NUM_PADS && padCache[static_cast<size_t>(i)].reverbSend)
               ? padCache[static_cast<size_t>(i)].reverbSend->load(std::memory_order_relaxed)
               : 0.0f;
}

float HostIntegration::getMasterVolume() const
{
    return masterVolumePtr ? masterVolumePtr->load(std::memory_order_relaxed) : 0.8f;
}

// =========================================================================
// State save / restore
// =========================================================================

juce::ValueTree HostIntegration::captureCurrentState() const
{
    juce::ValueTree state("VOIDState");

    // APVTS parameters
    state.addChild(apvts.copyState(), -1, nullptr);

    // MIDI mapper state
    state.addChild(midiMapper.toValueTree(), -1, nullptr);

    return state;
}

void HostIntegration::applyState(const juce::ValueTree& state)
{
    if (!state.isValid() || state.getType().toString() != "VOIDState")
        return;

    // Restore APVTS
    auto apvtsState = state.getChild(0);
    if (apvtsState.isValid())
        apvts.replaceState(apvtsState.createCopy());

    // Restore MIDI mapper
    auto midiState = state.getChildWithName("MIDIMapper");
    if (midiState.isValid())
        midiMapper.fromValueTree(midiState);
}

void HostIntegration::saveState(juce::MemoryBlock& destData) const
{
    auto state = captureCurrentState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml != nullptr)
        juce::AudioProcessor::copyXmlToBinary(*xml, destData);
}

void HostIntegration::restoreState(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        juce::AudioProcessor::getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr)
    {
        auto state = juce::ValueTree::fromXml(*xml);
        applyState(state);
    }
}

// =========================================================================
// A/B comparison
// =========================================================================

HostIntegration::Slot HostIntegration::getActiveSlot() const
{
    return activeSlot.load(std::memory_order_acquire);
}

void HostIntegration::toggleAB()
{
    std::lock_guard<std::mutex> lock(abMutex);

    auto current = activeSlot.load(std::memory_order_relaxed);
    auto snapshot = captureCurrentState();

    if (current == Slot::A)
    {
        slotA = snapshot;
        if (slotBInitialised)
            applyState(slotB);
        activeSlot.store(Slot::B, std::memory_order_release);
    }
    else
    {
        slotB = snapshot;
        slotBInitialised = true;
        applyState(slotA);
        activeSlot.store(Slot::A, std::memory_order_release);
    }
}

void HostIntegration::copyToSlotA()
{
    std::lock_guard<std::mutex> lock(abMutex);
    slotA = captureCurrentState();
}

void HostIntegration::copyToSlotB()
{
    std::lock_guard<std::mutex> lock(abMutex);
    slotB = captureCurrentState();
    slotBInitialised = true;
}

void HostIntegration::copyAToB()
{
    std::lock_guard<std::mutex> lock(abMutex);
    slotB = slotA.createCopy();
    slotBInitialised = true;
}

void HostIntegration::copyBToA()
{
    std::lock_guard<std::mutex> lock(abMutex);
    if (slotBInitialised)
        slotA = slotB.createCopy();
}

bool HostIntegration::hasSlotBState() const
{
    return slotBInitialised;
}

// =========================================================================
// Parameter listener
// =========================================================================

void HostIntegration::parameterChanged(const juce::String& /*parameterID*/,
                                       float /*newValue*/)
{
    // Placeholder for future parameter change tracking (e.g. dirty flag,
    // automation recording indicators). The actual parameter values are
    // read directly via the atomic pointers in padCache.
}

} // namespace void_drum

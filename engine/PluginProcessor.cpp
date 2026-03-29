#include "PluginProcessor.h"
#include "../ui/PluginEditor.h"
#include "interfaces/IPadState.h"
#include "interfaces/IMIDIMapping.h"
#include <cmath>

// =========================================================================
// Parameter layout helpers
// =========================================================================

static juce::String padParamId(int padIndex, const juce::String& suffix)
{
    // Produces e.g. "pad_01_volume", "pad_16_pan"
    return "pad_" + juce::String(padIndex + 1).paddedLeft('0', 2) + "_" + suffix;
}

static juce::String padParamName(int padIndex, const juce::String& label)
{
    return "Pad " + juce::String(padIndex + 1) + " " + label;
}

juce::AudioProcessorValueTreeState::ParameterLayout
VOIDDrumEngineProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 0; i < void_drum::NUM_PADS; ++i)
    {
        // Volume: 0.0 .. 1.0, default 0.8
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { padParamId(i, "volume"), 1 },
            padParamName(i, "Volume"),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
            0.8f));

        // Pan: -1.0 (L) .. +1.0 (R), default centre
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { padParamId(i, "pan"), 1 },
            padParamName(i, "Pan"),
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
            0.0f));

        // Pitch: -24 .. +24 semitones
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { padParamId(i, "pitch"), 1 },
            padParamName(i, "Pitch"),
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
            0.0f));

        // Filter cutoff: normalised 0..1 (maps to 20 Hz .. 20 kHz in DSP)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { padParamId(i, "filter_cutoff"), 1 },
            padParamName(i, "Filter Cutoff"),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
            1.0f));

        // Filter resonance: 0..1
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { padParamId(i, "filter_resonance"), 1 },
            padParamName(i, "Filter Resonance"),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
            0.0f));

        // Drive: 0..1
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { padParamId(i, "drive"), 1 },
            padParamName(i, "Drive"),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
            0.0f));

        // Bitcrush depth: 0..1 (1.0 = full resolution / off)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { padParamId(i, "bitcrush_depth"), 1 },
            padParamName(i, "Bitcrush Depth"),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
            1.0f));

        // Bitcrush rate: 0..1 (1.0 = no rate reduction / off)
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { padParamId(i, "bitcrush_rate"), 1 },
            padParamName(i, "Bitcrush Rate"),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
            1.0f));

        // Reverb send: 0..1
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { padParamId(i, "reverb_send"), 1 },
            padParamName(i, "Reverb Send"),
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
            0.0f));
    }

    // Master volume
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "master_volume", 1 },
        "Master Volume",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.8f));

    return { params.begin(), params.end() };
}

// =========================================================================
// Construction / destruction
// =========================================================================

VOIDDrumEngineProcessor::VOIDDrumEngineProcessor()
    : AudioProcessor(BusesProperties()
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "VOID_PARAMETERS", createParameterLayout()),
      midiMapping(void_drum::createDefaultMIDIMapping())
{
    // Cache raw parameter pointers for real-time access (no string lookups)
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
    {
        padParamCache[static_cast<size_t>(i)].volume        = apvts.getRawParameterValue(padParamId(i, "volume"));
        padParamCache[static_cast<size_t>(i)].pan           = apvts.getRawParameterValue(padParamId(i, "pan"));
        padParamCache[static_cast<size_t>(i)].pitch         = apvts.getRawParameterValue(padParamId(i, "pitch"));
        padParamCache[static_cast<size_t>(i)].filterCutoff  = apvts.getRawParameterValue(padParamId(i, "filter_cutoff"));
        padParamCache[static_cast<size_t>(i)].filterRes     = apvts.getRawParameterValue(padParamId(i, "filter_resonance"));
        padParamCache[static_cast<size_t>(i)].drive         = apvts.getRawParameterValue(padParamId(i, "drive"));
        padParamCache[static_cast<size_t>(i)].bitcrushDepth = apvts.getRawParameterValue(padParamId(i, "bitcrush_depth"));
        padParamCache[static_cast<size_t>(i)].bitcrushRate  = apvts.getRawParameterValue(padParamId(i, "bitcrush_rate"));
        padParamCache[static_cast<size_t>(i)].reverbSend    = apvts.getRawParameterValue(padParamId(i, "reverb_send"));
    }
    masterVolumeParam = apvts.getRawParameterValue("master_volume");

    // Initialise choke groups, mute, solo atomics
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
    {
        chokeGroups[static_cast<size_t>(i)].store(void_drum::CHOKE_GROUP_NONE, std::memory_order_relaxed);
        mutePad[static_cast<size_t>(i)].store(false, std::memory_order_relaxed);
        soloPad[static_cast<size_t>(i)].store(false, std::memory_order_relaxed);
    }

    // Construct HostIntegration (Agent 6) after APVTS and MIDIMapper are ready
    hostIntegration = std::make_unique<void_drum::HostIntegration>(*this, apvts, midiMapper);
}

VOIDDrumEngineProcessor::~VOIDDrumEngineProcessor() = default;

// =========================================================================
// Prepare / release
// =========================================================================

void VOIDDrumEngineProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Set host sample rate on the loader so samples are resampled correctly
    sampleLoader.setHostSampleRate(sampleRate);

    // Prepare voice allocator
    voiceAllocator.prepare(sampleRate);

    // Prepare per-pad DSP chains
    for (auto& dsp : padDSP)
        dsp.prepare(sampleRate, samplesPerBlock);

    // Prepare mix bus
    mixBus.prepare(sampleRate, samplesPerBlock);

    // Pre-allocate reverb send buffer
    reverbSendBuffer.setSize(2, samplesPerBlock);
    reverbSendBuffer.clear();
}

void VOIDDrumEngineProcessor::releaseResources()
{
    voiceAllocator.killAll();
    for (auto& dsp : padDSP)
        dsp.reset();
    mixBus.reset();
}

bool VOIDDrumEngineProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Only support stereo output
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

// =========================================================================
// Read pad state from APVTS (used for non-audio-thread queries)
// =========================================================================

void_drum::PadState VOIDDrumEngineProcessor::readPadState(int padIndex) const
{
    void_drum::PadState state;
    const auto& pp = padParamCache[static_cast<size_t>(padIndex)];

    state.volume          = pp.volume->load(std::memory_order_relaxed);
    state.pan             = pp.pan->load(std::memory_order_relaxed);
    state.pitch           = pp.pitch->load(std::memory_order_relaxed);
    state.filterCutoff    = pp.filterCutoff->load(std::memory_order_relaxed);
    state.filterResonance = pp.filterRes->load(std::memory_order_relaxed);
    state.drive           = pp.drive->load(std::memory_order_relaxed);
    state.bitcrushDepth   = pp.bitcrushDepth->load(std::memory_order_relaxed);
    state.bitcrushRate    = pp.bitcrushRate->load(std::memory_order_relaxed);
    state.reverbSend      = pp.reverbSend->load(std::memory_order_relaxed);
    state.mute            = mutePad[static_cast<size_t>(padIndex)].load(std::memory_order_relaxed);
    state.solo            = soloPad[static_cast<size_t>(padIndex)].load(std::memory_order_relaxed);
    state.chokeGroup      = chokeGroups[static_cast<size_t>(padIndex)].load(std::memory_order_relaxed);

    return state;
}

void_drum::PadBank VOIDDrumEngineProcessor::getPadStates() const
{
    void_drum::PadBank bank;
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
        bank[static_cast<size_t>(i)] = readPadState(i);
    return bank;
}

// =========================================================================
// Sample loading (delegates to SampleLoader)
// =========================================================================

void VOIDDrumEngineProcessor::loadSampleForPad(int padIndex, const juce::String& absolutePath,
                                                 const juce::String& sampleId)
{
    sampleLoader.loadSampleForPad(padIndex, absolutePath, sampleId);
}

// =========================================================================
// MIDI mapping
// =========================================================================

void VOIDDrumEngineProcessor::setMIDIMapping(const void_drum::MIDIMapping& mapping)
{
    std::lock_guard<std::mutex> lock(midiMappingMutex);
    midiMapping = mapping;
}

void_drum::MIDIMapping VOIDDrumEngineProcessor::getMIDIMapping() const
{
    std::lock_guard<std::mutex> lock(midiMappingMutex);
    return midiMapping;
}

// =========================================================================
// MIDI event handling
// =========================================================================

void VOIDDrumEngineProcessor::handleNoteOn(int padIndex, float velocity)
{
    if (padIndex < 0 || padIndex >= void_drum::NUM_PADS)
        return;

    auto sample = sampleLoader.getSampleForPad(padIndex);
    if (sample == nullptr)
        return;

    // Read pitch from APVTS
    const float pitch = padParamCache[static_cast<size_t>(padIndex)].pitch->load(std::memory_order_relaxed);
    const int chokeGroup = chokeGroups[static_cast<size_t>(padIndex)].load(std::memory_order_relaxed);

    // Handle choke groups: kill voices on other pads in the same group
    if (chokeGroup != void_drum::CHOKE_GROUP_NONE)
    {
        for (int i = 0; i < void_drum::NUM_PADS; ++i)
        {
            if (i != padIndex &&
                chokeGroups[static_cast<size_t>(i)].load(std::memory_order_relaxed) == chokeGroup)
            {
                voiceAllocator.killVoicesForPad(i);
            }
        }
    }

    voiceAllocator.triggerVoice(padIndex, std::move(sample), velocity, pitch,
                                chokeGroup, /*oneShot=*/true);
}

void VOIDDrumEngineProcessor::handleNoteOff(int /*padIndex*/)
{
    // For one-shot drum samples, note-off is typically ignored.
    // If looping mode is added later, this would trigger envelope release.
}

// =========================================================================
// Process block
// =========================================================================

void VOIDDrumEngineProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    juce::ignoreUnused(getTotalNumOutputChannels());
    const int numSamples = buffer.getNumSamples();

    // Clear the output buffer
    buffer.clear();

    // Clear the reverb send buffer
    reverbSendBuffer.clear(0, numSamples);

    // -- Copy MIDI mapping for this block (avoid lock contention) -------------
    void_drum::MIDIMapping currentMapping;
    {
        std::lock_guard<std::mutex> lock(midiMappingMutex);
        currentMapping = midiMapping;
    }

    // -- Process MIDI events via MIDIMapper (Agent 6) ---------------------------
    // The MIDIMapper handles MIDI Learn capture, note-to-pad routing with
    // per-pad velocity curves, and CC-to-parameter mapping.
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            const int noteNumber = msg.getNoteNumber();
            const int rawVelocity = msg.getVelocity();

            // Route through MIDIMapper (handles learn mode + note lookup)
            int padIndex = midiMapper.processNoteOn(noteNumber, rawVelocity);
            if (padIndex >= 0)
            {
                // Apply per-pad velocity curve (falls back to global)
                float velocity = midiMapper.applyVelocityCurve(rawVelocity, padIndex);
                handleNoteOn(padIndex, velocity);
            }

            // Also check legacy mapping for compatibility
            if (padIndex < 0)
            {
                auto it = currentMapping.noteToPad.find(noteNumber);
                if (it != currentMapping.noteToPad.end())
                {
                    float velocity = static_cast<float>(rawVelocity) / 127.0f;

                    switch (currentMapping.velocityCurve)
                    {
                        case void_drum::VelocityCurve::Linear:
                            break;
                        case void_drum::VelocityCurve::Exponential:
                            velocity = velocity * velocity;
                            break;
                        case void_drum::VelocityCurve::SCurve:
                            velocity = velocity * velocity * (3.0f - 2.0f * velocity);
                            break;
                    }

                    handleNoteOn(it->second, velocity);
                }
            }
        }
        else if (msg.isNoteOff())
        {
            const int noteNumber = msg.getNoteNumber();
            // Check MIDIMapper first
            int padIndex = midiMapper.getPadForNote(noteNumber);
            if (padIndex >= 0)
            {
                handleNoteOff(padIndex);
            }
            else
            {
                // Legacy mapping fallback
                auto it = currentMapping.noteToPad.find(noteNumber);
                if (it != currentMapping.noteToPad.end())
                    handleNoteOff(it->second);
            }
        }
        else if (msg.isController())
        {
            // Route MIDI CC through MIDIMapper to update APVTS parameters
            midiMapper.processCCMessage(msg.getControllerNumber(),
                                        msg.getControllerValue(), apvts);
        }
    }

    // -- Read pad states and compute solo logic --------------------------------
    void_drum::PadBank padStates;
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
        padStates[static_cast<size_t>(i)] = readPadState(i);

    bool soloActive = false;
    std::array<bool, void_drum::NUM_PADS> padAudible;
    void_drum::MixBus::computeSoloState(padStates, soloActive, padAudible);

    // -- Process each pad's DSP chain -----------------------------------------
    auto& voices = voiceAllocator.getVoices();

    for (int i = 0; i < void_drum::NUM_PADS; ++i)
    {
        if (!padAudible[static_cast<size_t>(i)])
            continue;

        // Build the pad state for DSP (with mute already handled by audibility check)
        auto state = padStates[static_cast<size_t>(i)];
        state.mute = false; // Already filtered by padAudible

        padDSP[static_cast<size_t>(i)].processBlock(
            voices, i, state,
            void_drum::FilterMode::LowPass, // Default filter mode; could be parameterised
            buffer, reverbSendBuffer, numSamples);
    }

    // -- Process inactive voices (advance envelopes for voices on muted pads) --
    // Voices on inaudible pads still need their envelopes processed so they
    // eventually become inactive and don't leak. We process them silently.
    for (auto& voice : voices)
    {
        if (!voice.active)
            continue;

        int pi = voice.padIndex;
        if (pi >= 0 && pi < void_drum::NUM_PADS && !padAudible[static_cast<size_t>(pi)])
        {
            // Advance envelope without producing audio
            for (int s = 0; s < numSamples; ++s)
            {
                voice.envelope.getNextSample();
                if (!voice.envelope.isActive())
                {
                    voice.active = false;
                    voice.sample.reset();
                    break;
                }
            }
        }
    }

    // -- Mix bus: reverb, master volume, limiter, metering --------------------
    const float masterVol = masterVolumeParam->load(std::memory_order_relaxed);
    mixBus.processBlock(buffer, reverbSendBuffer, masterVol, numSamples);
}

// =========================================================================
// Editor
// =========================================================================

juce::AudioProcessorEditor* VOIDDrumEngineProcessor::createEditor()
{
    return new VOIDDrumEngineEditor(*this);
}

bool VOIDDrumEngineProcessor::hasEditor() const { return true; }

// =========================================================================
// Metadata
// =========================================================================

const juce::String VOIDDrumEngineProcessor::getName() const { return "VOID Drum Engine"; }
bool VOIDDrumEngineProcessor::acceptsMidi()  const { return true; }
bool VOIDDrumEngineProcessor::producesMidi() const { return false; }
bool VOIDDrumEngineProcessor::isMidiEffect() const { return false; }
double VOIDDrumEngineProcessor::getTailLengthSeconds() const { return 0.5; }

int VOIDDrumEngineProcessor::getNumPrograms()                               { return 1; }
int VOIDDrumEngineProcessor::getCurrentProgram()                            { return 0; }
void VOIDDrumEngineProcessor::setCurrentProgram(int)                        {}
const juce::String VOIDDrumEngineProcessor::getProgramName(int)             { return {}; }
void VOIDDrumEngineProcessor::changeProgramName(int, const juce::String&)   {}

// =========================================================================
// State persistence (DAW save / recall)
// =========================================================================

void VOIDDrumEngineProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Use HostIntegration to save full state (APVTS + MIDI mappings)
    if (hostIntegration)
    {
        hostIntegration->saveState(destData);
    }
    else
    {
        // Fallback: save APVTS only
        auto state = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }
}

void VOIDDrumEngineProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Use HostIntegration to restore full state (APVTS + MIDI mappings)
    if (hostIntegration)
    {
        hostIntegration->restoreState(data, sizeInBytes);
    }
    else
    {
        // Fallback: restore APVTS only
        std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
        if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

// =========================================================================
// JUCE plugin instantiation entry point
// =========================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VOIDDrumEngineProcessor();
}

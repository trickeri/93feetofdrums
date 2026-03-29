#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "interfaces/IPadState.h"
#include "interfaces/IMIDIMapping.h"
#include "SampleLoader.h"
#include "VoiceAllocator.h"
#include "PadDSP.h"
#include "MixBus.h"
#include "MIDIMapper.h"
#include "HostIntegration.h"
#include <array>
#include <memory>

// =========================================================================
// VOIDDrumEngineProcessor
//
// Top-level AudioProcessor for the VOID Drum Engine VST plugin.
// Owns the parameter tree and integrates all audio engine subsystems:
//   - SampleLoader (async sample loading)
//   - VoiceAllocator (64-voice polyphony)
//   - PadDSP[16] (per-pad DSP chains)
//   - MixBus (reverb, master limiter, metering)
// =========================================================================
class VOIDDrumEngineProcessor : public juce::AudioProcessor
{
public:
    VOIDDrumEngineProcessor();
    ~VOIDDrumEngineProcessor() override;

    // -- AudioProcessor overrides -------------------------------------------
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midiMessages) override;

    // -- Editor -------------------------------------------------------------
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    // -- Program / preset ---------------------------------------------------
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    // -- State persistence --------------------------------------------------
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // -- Parameter tree (public so the editor can attach) -------------------
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // -- Engine subsystem access (for UI and other agents) ------------------
    void_drum::SampleLoader&    getSampleLoader()    { return sampleLoader; }
    void_drum::VoiceAllocator&  getVoiceAllocator()  { return voiceAllocator; }
    void_drum::MixBus&          getMixBus()           { return mixBus; }

    /** Get the current pad state bank (snapshot of all pad parameters).
     *  Safe to call from any thread (reads atomic APVTS parameters). */
    void_drum::PadBank getPadStates() const;

    /** Load a sample onto a pad. Thread-safe (queues for background loading). */
    void loadSampleForPad(int padIndex, const juce::String& absolutePath,
                          const juce::String& sampleId);

    /** Get the master output levels for UI metering. */
    const void_drum::StereoLevel& getMasterLevels() const { return mixBus.getMasterLevels(); }

    /** Set the MIDI mapping. */
    void setMIDIMapping(const void_drum::MIDIMapping& mapping);

    /** Get the current MIDI mapping. */
    void_drum::MIDIMapping getMIDIMapping() const;

    // -- MIDI Mapper & Host Integration (Agent 6) ----------------------------
    void_drum::MIDIMapper&       getMIDIMapper()       { return midiMapper; }
    const void_drum::MIDIMapper& getMIDIMapper() const { return midiMapper; }

    void_drum::HostIntegration&       getHostIntegration()       { return *hostIntegration; }
    const void_drum::HostIntegration& getHostIntegration() const { return *hostIntegration; }

private:
    // Builds the full parameter layout for all 16 pads + master.
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    /** Read the current pad state from APVTS parameters. */
    void_drum::PadState readPadState(int padIndex) const;

    /** Handle a single MIDI note-on event. */
    void handleNoteOn(int padIndex, float velocity);

    /** Handle a single MIDI note-off event. */
    void handleNoteOff(int padIndex);

    // -- Parameter tree -------------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;

    // -- Cached parameter pointers (avoid string lookups in processBlock) -----
    struct PadParams
    {
        std::atomic<float>* volume        = nullptr;
        std::atomic<float>* pan           = nullptr;
        std::atomic<float>* pitch         = nullptr;
        std::atomic<float>* filterCutoff  = nullptr;
        std::atomic<float>* filterRes     = nullptr;
        std::atomic<float>* drive         = nullptr;
        std::atomic<float>* bitcrushDepth = nullptr;
        std::atomic<float>* bitcrushRate  = nullptr;
        std::atomic<float>* reverbSend    = nullptr;
    };
    std::array<PadParams, void_drum::NUM_PADS> padParamCache;
    std::atomic<float>* masterVolumeParam = nullptr;

    // -- Engine subsystems ----------------------------------------------------
    void_drum::SampleLoader    sampleLoader;
    void_drum::VoiceAllocator  voiceAllocator;
    std::array<void_drum::PadDSP, void_drum::NUM_PADS> padDSP;
    void_drum::MixBus          mixBus;

    // -- Reverb send buffer (pre-allocated) -----------------------------------
    juce::AudioBuffer<float> reverbSendBuffer;

    // -- MIDI mapping (protected by atomic swap via simple copy) ---------------
    void_drum::MIDIMapping midiMapping;
    mutable std::mutex midiMappingMutex;

    // -- MIDI Mapper (Agent 6: full-featured mapping with learn, CC, curves) --
    void_drum::MIDIMapper midiMapper;

    // -- Host Integration (Agent 6: parameter caching, A/B, state management) -
    std::unique_ptr<void_drum::HostIntegration> hostIntegration;

    // -- Per-pad mutable state (choke groups, mute/solo) ----------------------
    // These are stored in the PadState but also need real-time-safe access.
    std::array<std::atomic<int>, void_drum::NUM_PADS> chokeGroups;
    std::array<std::atomic<bool>, void_drum::NUM_PADS> mutePad;
    std::array<std::atomic<bool>, void_drum::NUM_PADS> soloPad;

    // -- Round-robin counters per pad -----------------------------------------
    std::array<int, void_drum::NUM_PADS> roundRobinCounters {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VOIDDrumEngineProcessor)
};

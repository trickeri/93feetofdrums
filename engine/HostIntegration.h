#pragma once

#include "interfaces/IPadState.h"
#include "MIDIMapper.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>

namespace void_drum {

// =========================================================================
// HostIntegration
//
// Bridges the APVTS parameter tree with the audio engine, providing:
//   - Audio-thread-safe cached parameter reads (no locking)
//   - Full state save/restore for DAW project recall
//   - A/B comparison: two state snapshots that can be toggled
// =========================================================================
class HostIntegration : public juce::AudioProcessorValueTreeState::Listener
{
public:
    /** Construct with references to the processor, APVTS, and MIDI mapper. */
    HostIntegration(juce::AudioProcessor& processor,
                    juce::AudioProcessorValueTreeState& apvts,
                    MIDIMapper& midiMapper);

    ~HostIntegration() override;

    // -- Audio-thread parameter cache -----------------------------------------

    /** Call once at the start of each processBlock to refresh the cached
     *  parameter snapshot. Reads atomic pointers -- no locking. */
    void updateParameterCache();

    /** Get the cached parameter value for a pad. Thread-safe for audio thread
     *  after calling updateParameterCache(). */
    float getPadVolume(int padIndex) const;
    float getPadPan(int padIndex) const;
    float getPadPitch(int padIndex) const;
    float getPadFilterCutoff(int padIndex) const;
    float getPadFilterResonance(int padIndex) const;
    float getPadDrive(int padIndex) const;
    float getPadBitcrushDepth(int padIndex) const;
    float getPadBitcrushRate(int padIndex) const;
    float getPadReverbSend(int padIndex) const;
    float getMasterVolume() const;

    // -- State save / restore -------------------------------------------------

    /** Serialise the full plugin state (APVTS + MIDI mappings) to a
     *  MemoryBlock for DAW project save. */
    void saveState(juce::MemoryBlock& destData) const;

    /** Restore the full plugin state from a binary block. */
    void restoreState(const void* data, int sizeInBytes);

    // -- A/B comparison -------------------------------------------------------

    /** Which slot is currently active. */
    enum class Slot { A, B };

    /** Get the currently active slot. */
    Slot getActiveSlot() const;

    /** Toggle between slot A and B. Saves current state to the current slot,
     *  then loads the other slot's state. */
    void toggleAB();

    /** Copy the current state into slot A. */
    void copyToSlotA();

    /** Copy the current state into slot B. */
    void copyToSlotB();

    /** Copy slot A to slot B. */
    void copyAToB();

    /** Copy slot B to slot A. */
    void copyBToA();

    /** Returns true if slot B has valid state (has been stored at least once). */
    bool hasSlotBState() const;

    // -- APVTS Listener (for parameter change tracking) -----------------------
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    /** Build a ValueTree snapshot of the full current state. */
    juce::ValueTree captureCurrentState() const;

    /** Apply a ValueTree snapshot to restore state. */
    void applyState(const juce::ValueTree& state);

    /** Helper: pad parameter ID string. */
    static juce::String padParamId(int padIndex, const juce::String& suffix);

    // References (non-owning)
    juce::AudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;
    MIDIMapper& midiMapper;

    // Audio-thread parameter cache -- raw pointers into APVTS atomic floats.
    // These never go null as long as APVTS is alive.
    struct PadParamCache
    {
        std::atomic<float>* volume       = nullptr;
        std::atomic<float>* pan          = nullptr;
        std::atomic<float>* pitch        = nullptr;
        std::atomic<float>* filterCutoff = nullptr;
        std::atomic<float>* filterRes    = nullptr;
        std::atomic<float>* drive        = nullptr;
        std::atomic<float>* crushDepth   = nullptr;
        std::atomic<float>* crushRate    = nullptr;
        std::atomic<float>* reverbSend   = nullptr;
    };

    std::array<PadParamCache, NUM_PADS> padCache;
    std::atomic<float>* masterVolumePtr = nullptr;

    // A/B comparison slots
    std::atomic<Slot> activeSlot { Slot::A };
    juce::ValueTree slotA;
    juce::ValueTree slotB;
    bool slotBInitialised = false;
    std::mutex abMutex; // protects slot ValueTrees (UI thread only)

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HostIntegration)
};

} // namespace void_drum

#pragma once

#include "interfaces/IMIDIMapping.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace void_drum {

// =========================================================================
// CCMapping: maps a MIDI CC number to an APVTS parameter ID.
// =========================================================================
struct CCMapping
{
    int ccNumber = -1;
    juce::String parameterId;
};

// =========================================================================
// MIDIMapper
//
// Concrete implementation of IMIDIMapper. Handles:
//   - Note-to-pad routing with default GM drum map
//   - MIDI Learn mode (thread-safe via atomics)
//   - Velocity curve processing (Linear / Exponential / S-Curve)
//   - Per-pad velocity curve overrides
//   - MIDI CC-to-parameter mapping
//   - Serialisation to/from juce::ValueTree
// =========================================================================
class MIDIMapper : public IMIDIMapper
{
public:
    MIDIMapper();
    ~MIDIMapper() override = default;

    // -- IMIDIMapper interface ------------------------------------------------
    MIDIMapping getMapping() const override;
    void setMapping(const MIDIMapping& mapping) override;
    void midiLearn(int padIndex) override;

    // -- Note routing ---------------------------------------------------------

    /** Look up pad index for a given MIDI note. Returns -1 if unmapped. */
    int getPadForNote(int midiNote) const;

    /** Reverse lookup: get the MIDI note currently mapped to a pad.
     *  Returns -1 if no note is mapped to the given pad. */
    int getNoteForPad(int padIndex) const;

    /** Assign a specific MIDI note to a pad. Removes any prior mapping of
     *  that note, and removes the prior note for that pad if one existed. */
    void assignNoteToPad(int midiNote, int padIndex);

    /** Remove all mappings and restore the default GM drum map. */
    void resetToDefaults();

    // -- Velocity curves ------------------------------------------------------

    /** Apply the active velocity curve to a raw 0-127 velocity value.
     *  Returns a gain in range 0.0 .. 1.0. */
    float applyVelocityCurve(int velocity) const;

    /** Apply a per-pad velocity curve (if set) or fall back to the global curve. */
    float applyVelocityCurve(int velocity, int padIndex) const;

    /** Set the global velocity curve type. */
    void setGlobalVelocityCurve(VelocityCurve curve);

    /** Get the global velocity curve type. */
    VelocityCurve getGlobalVelocityCurve() const;

    /** Set a per-pad velocity curve override (-1 to clear and use global). */
    void setPadVelocityCurve(int padIndex, VelocityCurve curve);

    /** Check whether a per-pad velocity curve is set. */
    bool hasPadVelocityCurve(int padIndex) const;

    // -- MIDI Learn state (thread-safe reads) ---------------------------------

    /** True while learn mode is active (waiting for a note). */
    bool isLearning() const;

    /** Returns the pad index that learn mode is targeting, or -1 if inactive. */
    int getLearningPad() const;

    // -- MIDI CC mapping ------------------------------------------------------

    /** Map a MIDI CC number to an APVTS parameter ID. */
    void mapCCToParameter(int ccNumber, const juce::String& parameterId);

    /** Remove a CC mapping. */
    void unmapCC(int ccNumber);

    /** Get the parameter ID for a CC number, or empty string if unmapped. */
    juce::String getParameterForCC(int ccNumber) const;

    /** Process an incoming MIDI CC message, updating the APVTS parameter.
     *  Must be called from the audio thread with a valid APVTS reference. */
    void processCCMessage(int ccNumber, int ccValue,
                          juce::AudioProcessorValueTreeState& apvts) const;

    // -- MIDI processing (called from processBlock) ---------------------------

    /** Process a single incoming MIDI note-on. If in learn mode, captures
     *  the note and assigns it. Returns the pad index triggered, or -1. */
    int processNoteOn(int midiNote, int velocity);

    // -- Serialisation --------------------------------------------------------

    /** Serialise the full mapper state (note map + CC map + curves) to a
     *  ValueTree that can be embedded in the plugin state. */
    juce::ValueTree toValueTree() const;

    /** Restore state from a ValueTree previously created by toValueTree(). */
    void fromValueTree(const juce::ValueTree& tree);

private:
    /** Compute the gain for a velocity using a specific curve type. */
    static float computeVelocityGain(int velocity, VelocityCurve curve);

    // Note-to-pad mapping (protected by mappingMutex for UI-thread writes,
    // but audio-thread reads use a snapshot copy for lock-freedom).
    mutable std::mutex mappingMutex;
    std::unordered_map<int, int> noteToPad;      // MIDI note -> pad index
    std::unordered_map<int, int> padToNote;       // pad index -> MIDI note (reverse)

    // Velocity curves
    std::atomic<VelocityCurve> globalVelocityCurve { VelocityCurve::Linear };
    std::array<std::atomic<int>, NUM_PADS> padVelocityCurves; // -1 = use global

    // MIDI Learn state (audio-thread safe via atomics)
    std::atomic<int> learningPad { -1 };

    // CC-to-parameter mapping (protected by ccMutex)
    mutable std::mutex ccMutex;
    std::unordered_map<int, juce::String> ccToParameter; // CC number -> param ID

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MIDIMapper)
};

} // namespace void_drum

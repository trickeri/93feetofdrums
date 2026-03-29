#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "interfaces/IPadState.h"
#include "SampleLoader.h"
#include <array>
#include <cstdint>

namespace void_drum {

// =========================================================================
// ADSR Envelope (lightweight, no allocation)
// =========================================================================
struct ADSREnvelope
{
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    float attackTime  = 0.001f;  ///< seconds
    float decayTime   = 0.05f;   ///< seconds
    float sustainLevel = 1.0f;   ///< 0..1
    float releaseTime = 0.2f;    ///< seconds

    Stage stage       = Stage::Idle;
    float level       = 0.0f;
    float attackRate   = 0.0f;
    float decayRate    = 0.0f;
    float releaseRate  = 0.0f;

    void prepare(double sampleRate)
    {
        attackRate  = (attackTime  > 0.0f) ? 1.0f / static_cast<float>(attackTime  * sampleRate) : 1.0f;
        decayRate   = (decayTime   > 0.0f) ? 1.0f / static_cast<float>(decayTime   * sampleRate) : 1.0f;
        releaseRate = (releaseTime > 0.0f) ? 1.0f / static_cast<float>(releaseTime * sampleRate) : 1.0f;
    }

    void noteOn()
    {
        stage = Stage::Attack;
        level = 0.0f;
    }

    void noteOff()
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    float getNextSample()
    {
        switch (stage)
        {
            case Stage::Idle:
                return 0.0f;

            case Stage::Attack:
                level += attackRate;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = Stage::Decay;
                }
                return level;

            case Stage::Decay:
                level -= decayRate * (1.0f - sustainLevel);
                if (level <= sustainLevel)
                {
                    level = sustainLevel;
                    stage = Stage::Sustain;
                }
                return level;

            case Stage::Sustain:
                return sustainLevel;

            case Stage::Release:
                level -= releaseRate;
                if (level <= 0.0f)
                {
                    level = 0.0f;
                    stage = Stage::Idle;
                }
                return level;
        }
        return 0.0f;
    }

    bool isActive() const { return stage != Stage::Idle; }
};

// =========================================================================
// Voice: a single playing sample instance.
// =========================================================================
struct Voice
{
    bool   active        = false;
    int    padIndex      = -1;       ///< Which pad triggered this voice
    double samplePosition = 0.0;    ///< Current read position in the sample
    double playbackRate  = 1.0;     ///< Pitch-adjusted playback rate
    float  velocity      = 1.0f;    ///< MIDI velocity (0..1)
    bool   oneShot       = true;    ///< true = one-shot, false = loop
    uint64_t age         = 0;       ///< Monotonic counter for voice stealing
    int    roundRobinIdx = 0;       ///< Which round-robin layer is playing
    int    sampleEndPos  = -1;     ///< End position in samples (-1 = play to end)

    ADSREnvelope envelope;

    std::shared_ptr<LoadedSample> sample; ///< Pointer to loaded audio data

    void reset()
    {
        active = false;
        padIndex = -1;
        samplePosition = 0.0;
        playbackRate = 1.0;
        velocity = 1.0f;
        oneShot = true;
        age = 0;
        roundRobinIdx = 0;
        sampleEndPos = -1;
        sample.reset();
        envelope.stage = ADSREnvelope::Stage::Idle;
        envelope.level = 0.0f;
    }
};

// =========================================================================
// VoiceAllocator
//
// Manages a pool of MAX_VOICES voices. Allocates voices on note-on,
// steals the oldest voice when the pool is full, handles choke groups.
// =========================================================================
class VoiceAllocator
{
public:
    VoiceAllocator();

    /** Prepare all voices for playback at the given sample rate. */
    void prepare(double sampleRate);

    /** Trigger a new voice for a pad.
     *  @param padIndex  Pad index (0..NUM_PADS-1)
     *  @param sample    Shared pointer to the loaded sample data
     *  @param velocity  MIDI velocity normalised to 0..1
     *  @param pitchSemitones  Pitch shift in semitones (fractional for cents)
     *  @param chokeGroup  Choke group ID (0 = none)
     *  @param oneShot   true for one-shot, false for looping
     *  @return pointer to the allocated voice, or nullptr on failure */
    Voice* triggerVoice(int padIndex,
                        std::shared_ptr<LoadedSample> sample,
                        float velocity,
                        float pitchSemitones,
                        int chokeGroup,
                        bool oneShot = true);

    /** Kill all active voices on a specific pad (fast release). */
    void killVoicesForPad(int padIndex);

    /** Kill all active voices on any pad whose index is in the provided list.
     *  Used for choke groups: the processor builds the list of pads sharing
     *  the same choke group and passes them here. */
    void killVoicesForPads(const int* padIndices, int count);

    /** Kill all active voices. */
    void killAll();

    /** Get the voice array for iteration in processBlock. */
    std::array<Voice, MAX_VOICES>& getVoices() { return voices; }
    const std::array<Voice, MAX_VOICES>& getVoices() const { return voices; }

    /** Get the number of currently active voices. */
    int getActiveVoiceCount() const;

private:
    /** Find a free voice, or steal the oldest if none available. */
    Voice* allocateVoice();

    std::array<Voice, MAX_VOICES> voices;
    uint64_t ageCounter = 0; ///< Monotonic counter for voice age tracking
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceAllocator)
};

} // namespace void_drum

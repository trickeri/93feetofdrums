#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "interfaces/IPadState.h"
#include "VoiceAllocator.h"
#include "SampleLoader.h"
#include <array>

namespace void_drum {

// =========================================================================
// Filter mode for the state-variable filter
// =========================================================================
enum class FilterMode
{
    LowPass,
    HighPass,
    BandPass
};

// =========================================================================
// PadDSP: per-pad DSP processing chain.
//
// Signal flow:
//   [Sample Playback] -> [Pitch Shift (via playback rate)]
//     -> [Filter (LP/HP/BP)] -> [Drive/Saturation] -> [Bitcrusher]
//     -> [Volume/Pan] -> outputs to main + reverb send buffers
//
// This class processes all active voices belonging to a single pad,
// mixes them, and applies the per-pad effects chain. No allocation
// in any processing method.
// =========================================================================
class PadDSP
{
public:
    PadDSP();

    /** Prepare DSP modules for playback. */
    void prepare(double sampleRate, int maxBlockSize);

    /** Reset all DSP state (filters, etc). */
    void reset();

    /** Render all active voices for this pad into the output buffer.
     *
     *  @param voices        The global voice array (only voices matching padIndex are processed)
     *  @param padIndex      Which pad this DSP chain belongs to
     *  @param state         Current pad parameter state
     *  @param filterMode    Filter mode (LP/HP/BP)
     *  @param outputBuffer  Stereo output buffer to ADD into (main bus)
     *  @param reverbBuffer  Stereo reverb send buffer to ADD into
     *  @param numSamples    Number of samples to process this block
     */
    void processBlock(std::array<Voice, MAX_VOICES>& voices,
                      int padIndex,
                      const PadState& state,
                      FilterMode filterMode,
                      juce::AudioBuffer<float>& outputBuffer,
                      juce::AudioBuffer<float>& reverbBuffer,
                      int numSamples);

private:
    // -- Internal processing buffers (pre-allocated) --------------------------
    juce::AudioBuffer<float> padBuffer;  ///< Stereo scratch buffer for this pad

    // -- State-variable filter ------------------------------------------------
    // We implement a simple SVF manually for real-time safety.
    float svfIc1eq = 0.0f;  ///< SVF integrator state 1
    float svfIc2eq = 0.0f;  ///< SVF integrator state 2

    // -- Bitcrusher state -----------------------------------------------------
    float crushHoldL = 0.0f; ///< Last held sample (left)
    float crushHoldR = 0.0f; ///< Last held sample (right)
    float crushPhase = 0.0f; ///< Phase accumulator for rate reduction

    // -- Sample rate ----------------------------------------------------------
    double currentSampleRate = 44100.0;
    int    maxBlock          = 512;

    // -- Internal DSP methods (inline for performance) ------------------------

    /** Render voices into padBuffer (sample playback + pitch via rate). */
    void renderVoices(std::array<Voice, MAX_VOICES>& voices,
                      int padIndex, int numSamples);

    /** Apply state-variable filter in-place. */
    void applyFilter(float* left, float* right, int numSamples,
                     float cutoffNorm, float resonance, FilterMode mode);

    /** Apply tanh soft-clip saturation in-place. */
    void applyDrive(float* left, float* right, int numSamples, float driveAmount);

    /** Apply bitcrusher in-place. */
    void applyBitcrush(float* left, float* right, int numSamples,
                       float depthNorm, float rateNorm);

    /** Apply volume and pan, then add to output and reverb buffers. */
    void applyVolumeAndPan(const float* left, const float* right, int numSamples,
                           float volume, float pan, float reverbSend, bool mute,
                           juce::AudioBuffer<float>& outputBuffer,
                           juce::AudioBuffer<float>& reverbBuffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PadDSP)
};

} // namespace void_drum

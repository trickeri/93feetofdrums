#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "interfaces/IPadState.h"
#include <array>
#include <atomic>

namespace void_drum {

// =========================================================================
// StereoLevel: atomic-safe level metering data for UI reads.
// =========================================================================
struct StereoLevel
{
    std::atomic<float> peakL  { 0.0f };
    std::atomic<float> peakR  { 0.0f };
    std::atomic<float> rmsL   { 0.0f };
    std::atomic<float> rmsR   { 0.0f };
};

// =========================================================================
// MixBus
//
// Collects the output of all 16 pad DSP chains into a stereo master bus.
// Provides:
//   - Global reverb send bus (juce::dsp::Reverb)
//   - Master volume + peak limiter
//   - Stereo level metering (peak + RMS) for UI
//   - Solo management
//
// The pad DSP chains write directly into the main and reverb buffers.
// MixBus then processes the reverb, applies master volume + limiter,
// and computes metering.
// =========================================================================
class MixBus
{
public:
    MixBus();

    /** Prepare all DSP for playback. */
    void prepare(double sampleRate, int maxBlockSize);

    /** Reset all internal state. */
    void reset();

    /** Process the reverb send, add to main, apply master volume + limiter.
     *
     *  @param mainBuffer    The main stereo output (already contains summed pad output)
     *  @param reverbBuffer  The reverb send input (summed by pad DSP chains)
     *  @param masterVolume  Master volume 0..1
     *  @param numSamples    Samples in this block
     */
    void processBlock(juce::AudioBuffer<float>& mainBuffer,
                      juce::AudioBuffer<float>& reverbBuffer,
                      float masterVolume,
                      int numSamples);

    /** Check if any pad has solo enabled, and return the solo state.
     *  This is called before pad processing so muted/non-solo pads can be skipped.
     *  @param padStates  Array of pad states to check
     *  @param outSoloActive  Set to true if any pad has solo enabled
     *  @param outPadAudible  Per-pad output: true if this pad should be heard
     */
    static void computeSoloState(const PadBank& padStates,
                                  bool& outSoloActive,
                                  std::array<bool, NUM_PADS>& outPadAudible);

    /** Get the master output levels for UI metering. */
    const StereoLevel& getMasterLevels() const { return masterLevels; }

    /** Set reverb parameters. */
    void setReverbParams(float roomSize, float damping, float wetLevel, float dryLevel);

private:
    // -- Reverb ---------------------------------------------------------------
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;

    // -- Limiter (simple peak limiter) ----------------------------------------
    float limiterThreshold = 0.95f;
    float limiterGain      = 1.0f;  ///< Gain reduction state

    // -- Metering -------------------------------------------------------------
    StereoLevel masterLevels;

    // -- Internal state -------------------------------------------------------
    double currentSampleRate = 44100.0;
    int    maxBlock          = 512;

    /** Apply a simple peak limiter in-place. */
    void applyLimiter(float* left, float* right, int numSamples);

    /** Update metering from the final output. */
    void updateMetering(const float* left, const float* right, int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixBus)
};

} // namespace void_drum

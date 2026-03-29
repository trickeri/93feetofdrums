#include "MixBus.h"
#include <cmath>
#include <algorithm>

namespace void_drum {

// =========================================================================
// Construction
// =========================================================================

MixBus::MixBus()
{
    // Default reverb settings: subtle room
    reverbParams.roomSize   = 0.5f;
    reverbParams.damping    = 0.5f;
    reverbParams.wetLevel   = 0.33f;
    reverbParams.dryLevel   = 0.0f;  // Dry signal already in main bus
    reverbParams.width      = 1.0f;
    reverbParams.freezeMode = 0.0f;
}

// =========================================================================
// Prepare / Reset
// =========================================================================

void MixBus::prepare(double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;
    maxBlock = maxBlockSize;

    // Prepare reverb
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels      = 2;
    reverb.prepare(spec);
    reverb.setParameters(reverbParams);

    // Reset limiter state
    limiterGain = 1.0f;

    // Reset metering
    masterLevels.peakL.store(0.0f, std::memory_order_relaxed);
    masterLevels.peakR.store(0.0f, std::memory_order_relaxed);
    masterLevels.rmsL.store(0.0f, std::memory_order_relaxed);
    masterLevels.rmsR.store(0.0f, std::memory_order_relaxed);
}

void MixBus::reset()
{
    reverb.reset();
    limiterGain = 1.0f;
}

// =========================================================================
// Set reverb parameters
// =========================================================================

void MixBus::setReverbParams(float roomSize, float damping, float wetLevel, float dryLevel)
{
    reverbParams.roomSize = roomSize;
    reverbParams.damping  = damping;
    reverbParams.wetLevel = wetLevel;
    reverbParams.dryLevel = dryLevel;
    reverb.setParameters(reverbParams);
}

// =========================================================================
// Solo state computation
// =========================================================================

void MixBus::computeSoloState(const PadBank& padStates,
                                bool& outSoloActive,
                                std::array<bool, NUM_PADS>& outPadAudible)
{
    // Check if any pad has solo enabled
    outSoloActive = false;
    for (int i = 0; i < NUM_PADS; ++i)
    {
        if (padStates[static_cast<size_t>(i)].solo)
        {
            outSoloActive = true;
            break;
        }
    }

    for (int i = 0; i < NUM_PADS; ++i)
    {
        const auto& ps = padStates[static_cast<size_t>(i)];

        if (ps.mute)
        {
            outPadAudible[static_cast<size_t>(i)] = false;
        }
        else if (outSoloActive)
        {
            // When solo is active, only solo'd pads are audible
            outPadAudible[static_cast<size_t>(i)] = ps.solo;
        }
        else
        {
            outPadAudible[static_cast<size_t>(i)] = true;
        }
    }
}

// =========================================================================
// Main process
// =========================================================================

void MixBus::processBlock(juce::AudioBuffer<float>& mainBuffer,
                           juce::AudioBuffer<float>& reverbBuffer,
                           float masterVolume,
                           int numSamples)
{
    // 1. Process reverb on the send buffer
    {
        juce::dsp::AudioBlock<float> reverbBlock(reverbBuffer);
        auto subBlock = reverbBlock.getSubBlock(0, static_cast<size_t>(numSamples));
        juce::dsp::ProcessContextReplacing<float> context(subBlock);
        reverb.process(context);
    }

    // 2. Add reverb output to main bus
    {
        float* mainL = mainBuffer.getWritePointer(0);
        float* mainR = mainBuffer.getWritePointer(1);
        const float* revL = reverbBuffer.getReadPointer(0);
        const float* revR = reverbBuffer.getReadPointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            mainL[i] += revL[i];
            mainR[i] += revR[i];
        }
    }

    // 3. Apply master volume
    if (std::abs(masterVolume - 1.0f) > 0.0001f)
    {
        mainBuffer.applyGain(0, numSamples, masterVolume);
    }

    // 4. Apply peak limiter
    {
        float* left  = mainBuffer.getWritePointer(0);
        float* right = mainBuffer.getWritePointer(1);
        applyLimiter(left, right, numSamples);
    }

    // 5. Update metering
    {
        const float* left  = mainBuffer.getReadPointer(0);
        const float* right = mainBuffer.getReadPointer(1);
        updateMetering(left, right, numSamples);
    }
}

// =========================================================================
// Peak limiter (simple soft-knee limiter with fast attack, slow release)
// =========================================================================

void MixBus::applyLimiter(float* left, float* right, int numSamples)
{
    const float threshold = limiterThreshold;
    const float attackCoeff  = 0.001f;  // Very fast attack
    const float releaseCoeff = 0.01f;   // Slower release (avoids pumping)

    for (int i = 0; i < numSamples; ++i)
    {
        const float peakSample = std::max(std::abs(left[i]), std::abs(right[i]));

        float targetGain = 1.0f;
        if (peakSample > threshold)
            targetGain = threshold / peakSample;

        // Smooth gain changes
        if (targetGain < limiterGain)
            limiterGain += attackCoeff * (targetGain - limiterGain);  // Fast attack
        else
            limiterGain += releaseCoeff * (targetGain - limiterGain); // Slow release

        left[i]  *= limiterGain;
        right[i] *= limiterGain;
    }
}

// =========================================================================
// Metering (peak + RMS)
// =========================================================================

void MixBus::updateMetering(const float* left, const float* right, int numSamples)
{
    if (numSamples <= 0)
        return;

    float peakL = 0.0f;
    float peakR = 0.0f;
    float sumSqL = 0.0f;
    float sumSqR = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float absL = std::abs(left[i]);
        const float absR = std::abs(right[i]);

        peakL = std::max(peakL, absL);
        peakR = std::max(peakR, absR);

        sumSqL += left[i] * left[i];
        sumSqR += right[i] * right[i];
    }

    const float rmsL = std::sqrt(sumSqL / static_cast<float>(numSamples));
    const float rmsR = std::sqrt(sumSqR / static_cast<float>(numSamples));

    // Smooth peak with decay (ballistic metering)
    const float peakDecay = 0.95f; // Per-block decay
    float prevPeakL = masterLevels.peakL.load(std::memory_order_relaxed);
    float prevPeakR = masterLevels.peakR.load(std::memory_order_relaxed);

    masterLevels.peakL.store(std::max(peakL, prevPeakL * peakDecay), std::memory_order_relaxed);
    masterLevels.peakR.store(std::max(peakR, prevPeakR * peakDecay), std::memory_order_relaxed);

    // Smooth RMS
    const float rmsSmooth = 0.8f;
    float prevRmsL = masterLevels.rmsL.load(std::memory_order_relaxed);
    float prevRmsR = masterLevels.rmsR.load(std::memory_order_relaxed);

    masterLevels.rmsL.store(prevRmsL * rmsSmooth + rmsL * (1.0f - rmsSmooth), std::memory_order_relaxed);
    masterLevels.rmsR.store(prevRmsR * rmsSmooth + rmsR * (1.0f - rmsSmooth), std::memory_order_relaxed);
}

} // namespace void_drum

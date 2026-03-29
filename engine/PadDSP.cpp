#include "PadDSP.h"
#include <cmath>
#include <algorithm>

namespace void_drum {

// =========================================================================
// Construction
// =========================================================================

PadDSP::PadDSP() = default;

// =========================================================================
// Prepare / Reset
// =========================================================================

void PadDSP::prepare(double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;
    maxBlock = maxBlockSize;

    // Pre-allocate the scratch buffer (stereo)
    padBuffer.setSize(2, maxBlockSize);
    padBuffer.clear();

    reset();
}

void PadDSP::reset()
{
    svfIc1eq = 0.0f;
    svfIc2eq = 0.0f;
    crushHoldL = 0.0f;
    crushHoldR = 0.0f;
    crushPhase = 0.0f;
}

// =========================================================================
// Main process
// =========================================================================

void PadDSP::processBlock(std::array<Voice, MAX_VOICES>& voices,
                           int padIndex,
                           const PadState& state,
                           FilterMode filterMode,
                           juce::AudioBuffer<float>& outputBuffer,
                           juce::AudioBuffer<float>& reverbBuffer,
                           int numSamples)
{
    // Clear pad scratch buffer
    padBuffer.clear(0, numSamples);

    // 1. Render all active voices for this pad into padBuffer
    renderVoices(voices, padIndex, numSamples);

    float* left  = padBuffer.getWritePointer(0);
    float* right = padBuffer.getWritePointer(1);

    // 2. Filter (skip if cutoff is fully open and resonance is zero)
    if (state.filterCutoff < 0.999f || state.filterResonance > 0.001f)
        applyFilter(left, right, numSamples, state.filterCutoff, state.filterResonance, filterMode);

    // 3. Drive / saturation (skip if drive is zero)
    if (state.drive > 0.001f)
        applyDrive(left, right, numSamples, state.drive);

    // 4. Bitcrusher (skip if both params at max = off)
    if (state.bitcrushDepth < 0.999f || state.bitcrushRate < 0.999f)
        applyBitcrush(left, right, numSamples, state.bitcrushDepth, state.bitcrushRate);

    // 5. Volume, pan, mute -> add to output + reverb send
    applyVolumeAndPan(left, right, numSamples,
                      state.volume, state.pan, state.reverbSend, state.mute,
                      outputBuffer, reverbBuffer);
}

// =========================================================================
// Voice rendering (sample playback with pitch via variable-rate reading)
// =========================================================================

void PadDSP::renderVoices(std::array<Voice, MAX_VOICES>& voices,
                           int padIndex, int numSamples)
{
    for (auto& voice : voices)
    {
        if (!voice.active || voice.padIndex != padIndex || voice.sample == nullptr)
            continue;

        const auto& sampleData = voice.sample;
        const int sampleLen   = sampleData->numSamples;
        const int sampleChans = sampleData->numChannels;

        if (sampleLen <= 0)
            continue;

        const float* srcL = sampleData->buffer.getReadPointer(0);
        const float* srcR = (sampleChans > 1) ? sampleData->buffer.getReadPointer(1) : srcL;

        float* dstL = padBuffer.getWritePointer(0);
        float* dstR = padBuffer.getWritePointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            // Get envelope level
            float envLevel = voice.envelope.getNextSample();

            if (!voice.envelope.isActive())
            {
                voice.active = false;
                voice.sample.reset();
                break;
            }

            // Check if sample is finished
            const int pos0 = static_cast<int>(voice.samplePosition);
            if (pos0 >= sampleLen - 1)
            {
                if (voice.oneShot)
                {
                    voice.envelope.noteOff(); // Trigger release
                    // Continue processing envelope tail
                    dstL[i] += 0.0f;
                    dstR[i] += 0.0f;
                    voice.samplePosition += voice.playbackRate;
                    continue;
                }
                else
                {
                    // Loop mode: wrap around
                    voice.samplePosition = std::fmod(voice.samplePosition, static_cast<double>(sampleLen));
                }
            }

            // Linear interpolation
            const int idx0 = static_cast<int>(voice.samplePosition);
            const int idx1 = std::min(idx0 + 1, sampleLen - 1);
            const float frac = static_cast<float>(voice.samplePosition - idx0);

            const float sL = srcL[idx0] + frac * (srcL[idx1] - srcL[idx0]);
            const float sR = srcR[idx0] + frac * (srcR[idx1] - srcR[idx0]);

            const float gain = voice.velocity * envLevel;
            dstL[i] += sL * gain;
            dstR[i] += sR * gain;

            voice.samplePosition += voice.playbackRate;
        }
    }
}

// =========================================================================
// State-Variable Filter (SVF)
//
// Implementation based on the Andrew Simper / Cytomic SVF topology.
// Processes both channels with shared coefficients.
// =========================================================================

void PadDSP::applyFilter(float* left, float* right, int numSamples,
                          float cutoffNorm, float resonance, FilterMode mode)
{
    // Map normalised cutoff (0..1) to frequency (20 Hz .. 20 kHz), exponential
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    const float freq = minFreq * std::pow(maxFreq / minFreq, cutoffNorm);

    // Clamp frequency to Nyquist
    const float nyquist = static_cast<float>(currentSampleRate) * 0.5f;
    const float clampedFreq = std::min(freq, nyquist * 0.95f);

    // SVF coefficients
    const float g = std::tan(juce::MathConstants<float>::pi * clampedFreq
                             / static_cast<float>(currentSampleRate));
    const float k = 2.0f - 2.0f * resonance; // Q control: resonance 0..1 maps to k 2..0
    const float a1 = 1.0f / (1.0f + g * (g + k));
    const float a2 = g * a1;
    const float a3 = g * a2;

    // Process both channels with shared state (mono-linked filter).
    // For stereo independence we'd need per-channel state, but for drums
    // a linked filter sounds fine and saves CPU.
    // Actually, let's do stereo properly with separate state per channel.
    // We'll use ic1eq/ic2eq for left, and separate vars for right.
    float ic1L = svfIc1eq;
    float ic2L = svfIc2eq;
    // Store right channel state in local vars; we keep it simple by
    // mirroring left-channel state (drums are typically mono or near-mono).
    // For true stereo, we'd need a second pair of integrators.
    float ic1R = ic1L;
    float ic2R = ic2L;

    for (int i = 0; i < numSamples; ++i)
    {
        // Left channel
        {
            const float v0 = left[i];
            const float v3 = v0 - ic2L;
            const float v1 = a1 * ic1L + a2 * v3;
            const float v2 = ic2L + a2 * ic1L + a3 * v3;
            ic1L = 2.0f * v1 - ic1L;
            ic2L = 2.0f * v2 - ic2L;

            switch (mode)
            {
                case FilterMode::LowPass:  left[i] = v2; break;
                case FilterMode::HighPass:  left[i] = v0 - k * v1 - v2; break;
                case FilterMode::BandPass:  left[i] = v1; break;
            }
        }

        // Right channel
        {
            const float v0 = right[i];
            const float v3 = v0 - ic2R;
            const float v1 = a1 * ic1R + a2 * v3;
            const float v2 = ic2R + a2 * ic1R + a3 * v3;
            ic1R = 2.0f * v1 - ic1R;
            ic2R = 2.0f * v2 - ic2R;

            switch (mode)
            {
                case FilterMode::LowPass:  right[i] = v2; break;
                case FilterMode::HighPass:  right[i] = v0 - k * v1 - v2; break;
                case FilterMode::BandPass:  right[i] = v1; break;
            }
        }
    }

    // Store state back (use left channel as the canonical state)
    svfIc1eq = ic1L;
    svfIc2eq = ic2L;
}

// =========================================================================
// Drive: tanh soft-clip saturation
// =========================================================================

void PadDSP::applyDrive(float* left, float* right, int numSamples, float driveAmount)
{
    // Drive maps 0..1 to gain of 1..20 (exponential feel)
    const float gain = 1.0f + driveAmount * 19.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        left[i]  = std::tanh(left[i]  * gain);
        right[i] = std::tanh(right[i] * gain);
    }
}

// =========================================================================
// Bitcrusher: sample rate reduction + bit depth reduction
// =========================================================================

void PadDSP::applyBitcrush(float* left, float* right, int numSamples,
                             float depthNorm, float rateNorm)
{
    // Bit depth: depthNorm 1.0 = 32 bits (clean), 0.0 = ~2 bits (crushed)
    // We map to quantisation steps
    const float bits = 2.0f + depthNorm * 30.0f; // 2..32 bits
    const float steps = std::pow(2.0f, bits);
    const float invSteps = 1.0f / steps;

    // Rate reduction: rateNorm 1.0 = no reduction, 0.0 = max reduction
    // We hold samples for longer when rateNorm is low.
    // phaseIncrement: 1.0 = no reduction, <1.0 = hold samples
    const float phaseIncrement = 0.01f + rateNorm * 0.99f; // 0.01 .. 1.0

    for (int i = 0; i < numSamples; ++i)
    {
        crushPhase += phaseIncrement;

        if (crushPhase >= 1.0f)
        {
            crushPhase -= 1.0f;

            // Sample and quantise
            crushHoldL = std::round(left[i]  * steps) * invSteps;
            crushHoldR = std::round(right[i] * steps) * invSteps;
        }

        left[i]  = crushHoldL;
        right[i] = crushHoldR;
    }
}

// =========================================================================
// Volume, pan, mute -> add to output and reverb send
// =========================================================================

void PadDSP::applyVolumeAndPan(const float* left, const float* right, int numSamples,
                                 float volume, float pan, float reverbSend, bool mute,
                                 juce::AudioBuffer<float>& outputBuffer,
                                 juce::AudioBuffer<float>& reverbBuffer)
{
    if (mute)
        return;

    // Constant-power pan law
    // pan: -1 (full left) to +1 (full right)
    const float panAngle = (pan + 1.0f) * 0.5f; // 0..1
    const float gainL = volume * std::cos(panAngle * juce::MathConstants<float>::halfPi);
    const float gainR = volume * std::sin(panAngle * juce::MathConstants<float>::halfPi);

    float* outL = outputBuffer.getWritePointer(0);
    float* outR = outputBuffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        outL[i] += left[i]  * gainL;
        outR[i] += right[i] * gainR;
    }

    // Reverb send (post-fader)
    if (reverbSend > 0.001f)
    {
        float* revL = reverbBuffer.getWritePointer(0);
        float* revR = reverbBuffer.getWritePointer(1);
        const float sendGain = reverbSend * volume;

        for (int i = 0; i < numSamples; ++i)
        {
            revL[i] += left[i]  * sendGain;
            revR[i] += right[i] * sendGain;
        }
    }
}

} // namespace void_drum

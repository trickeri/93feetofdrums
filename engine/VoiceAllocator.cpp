#include "VoiceAllocator.h"
#include <cmath>
#include <limits>

namespace void_drum {

// =========================================================================
// Construction
// =========================================================================

VoiceAllocator::VoiceAllocator()
{
    for (auto& v : voices)
        v.reset();
}

// =========================================================================
// Prepare
// =========================================================================

void VoiceAllocator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;

    for (auto& v : voices)
    {
        v.envelope.prepare(sampleRate);
        v.reset();
    }
}

// =========================================================================
// Voice allocation with oldest-note-first stealing
// =========================================================================

Voice* VoiceAllocator::allocateVoice()
{
    // First pass: find a free (inactive) voice
    for (auto& v : voices)
    {
        if (!v.active)
            return &v;
    }

    // No free voice -- steal the oldest (lowest age counter)
    Voice* oldest = &voices[0];
    uint64_t oldestAge = std::numeric_limits<uint64_t>::max();

    for (auto& v : voices)
    {
        if (v.age < oldestAge)
        {
            oldestAge = v.age;
            oldest = &v;
        }
    }

    oldest->reset();
    return oldest;
}

// =========================================================================
// Trigger
// =========================================================================

Voice* VoiceAllocator::triggerVoice(int padIndex,
                                     std::shared_ptr<LoadedSample> sample,
                                     float velocity,
                                     float pitchSemitones,
                                     int chokeGroupId,
                                     bool oneShot)
{
    if (sample == nullptr || padIndex < 0 || padIndex >= NUM_PADS)
        return nullptr;

    // Choke group handling is done by the processor before calling triggerVoice.
    // It builds the list of pads sharing the group and calls killVoicesForPads.
    juce::ignoreUnused(chokeGroupId);

    Voice* voice = allocateVoice();
    if (voice == nullptr)
        return nullptr;

    voice->active         = true;
    voice->padIndex       = padIndex;
    voice->samplePosition = 0.0;
    voice->velocity       = velocity;
    voice->oneShot        = oneShot;
    voice->age            = ++ageCounter;
    voice->sample         = std::move(sample);

    // Calculate playback rate from pitch shift
    // semitones to rate: rate = 2^(semitones/12)
    voice->playbackRate = std::pow(2.0, static_cast<double>(pitchSemitones) / 12.0);

    // Set up envelope
    voice->envelope.attackTime   = 0.001f;  // 1ms attack (instant for drums)
    voice->envelope.decayTime    = 0.0f;
    voice->envelope.sustainLevel = 1.0f;
    voice->envelope.releaseTime  = 0.2f;    // 200ms release tail
    voice->envelope.prepare(currentSampleRate);
    voice->envelope.noteOn();

    return voice;
}

// =========================================================================
// Kill voices by pad
// =========================================================================

void VoiceAllocator::killVoicesForPad(int padIndex)
{
    for (auto& v : voices)
    {
        if (v.active && v.padIndex == padIndex)
        {
            // Fast release for a natural choke sound
            v.envelope.releaseTime = 0.005f; // 5ms fade-out
            v.envelope.prepare(currentSampleRate);
            v.envelope.noteOff();
        }
    }
}

void VoiceAllocator::killVoicesForPads(const int* padIndices, int count)
{
    for (int i = 0; i < count; ++i)
        killVoicesForPad(padIndices[i]);
}

void VoiceAllocator::killAll()
{
    for (auto& v : voices)
        v.reset();
}

// =========================================================================
// Active voice count
// =========================================================================

int VoiceAllocator::getActiveVoiceCount() const
{
    int count = 0;
    for (const auto& v : voices)
    {
        if (v.active)
            ++count;
    }
    return count;
}

} // namespace void_drum

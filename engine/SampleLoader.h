#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include "interfaces/ISampleRegistry.h"
#include "interfaces/IPadState.h"
#include <array>
#include <atomic>
#include <memory>
#include <mutex>

namespace void_drum {

// =========================================================================
// LoadedSample: an audio buffer resampled to host rate, ready for playback.
// =========================================================================
struct LoadedSample
{
    juce::AudioBuffer<float> buffer;          ///< Audio data at host sample rate
    int numChannels   = 0;
    int numSamples    = 0;
    double sampleRate = 44100.0;              ///< Host sample rate it was resampled to
    std::array<float, 128> waveformMinimap{}; ///< 128-point peak array for UI
    juce::String sampleId;                    ///< Canonical ID (relative path)
};

// =========================================================================
// SampleLoader
//
// Loads samples asynchronously on a background thread. Resamples to host
// sample rate. Provides lock-free access for the audio thread via atomic
// shared_ptr swaps.
// =========================================================================
class SampleLoader : private juce::Thread
{
public:
    SampleLoader();
    ~SampleLoader() override;

    /** Set the host sample rate (call from prepareToPlay). */
    void setHostSampleRate(double rate);

    /** Request async load of a sample for a given pad.
     *  The sample will be loaded on the background thread. */
    void loadSampleForPad(int padIndex, const juce::String& absolutePath,
                          const juce::String& sampleId);

    /** Get the currently loaded sample for a pad (lock-free, audio-thread safe).
     *  Returns nullptr if nothing is loaded. */
    std::shared_ptr<LoadedSample> getSampleForPad(int padIndex) const;

    /** Unload sample from a pad. */
    void clearPad(int padIndex);

    /** Generate a 128-point waveform minimap from an audio buffer. */
    static std::array<float, 128> generateWaveformMinimap(
        const juce::AudioBuffer<float>& buffer, int numSamples);

private:
    // -- Background thread work ------------------------------------------------
    void run() override;

    struct LoadRequest
    {
        int padIndex;
        juce::String absolutePath;
        juce::String sampleId;
    };

    // Load request queue (protected by mutex -- only touched by message/UI thread
    // and background thread, never the audio thread)
    std::mutex requestMutex;
    std::vector<LoadRequest> pendingRequests;

    // Loaded sample storage -- atomic shared_ptr for lock-free audio thread reads
    // std::atomic<std::shared_ptr<T>> requires C++20; we use a spin-lock-free
    // approach with a regular shared_ptr behind a lightweight mechanism.
    mutable std::mutex sampleMutex;
    std::array<std::shared_ptr<LoadedSample>, NUM_PADS> loadedSamples;

    // Audio format manager (owned, used only on background thread)
    juce::AudioFormatManager formatManager;

    std::atomic<double> hostSampleRate { 44100.0 };

    /** Perform the actual loading, resampling, and minimap generation. */
    std::shared_ptr<LoadedSample> loadFromFile(const juce::String& absolutePath,
                                                const juce::String& sampleId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleLoader)
};

} // namespace void_drum

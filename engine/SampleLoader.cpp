#include "SampleLoader.h"
#include <cmath>

namespace void_drum {

// =========================================================================
// Construction / destruction
// =========================================================================

SampleLoader::SampleLoader()
    : juce::Thread("VOID_SampleLoader")
{
    formatManager.registerBasicFormats(); // WAV, AIFF, FLAC, OGG (if available)
    startThread(juce::Thread::Priority::background);
}

SampleLoader::~SampleLoader()
{
    signalThreadShouldExit();
    notify();
    stopThread(5000);
}

// =========================================================================
// Public API
// =========================================================================

void SampleLoader::setHostSampleRate(double rate)
{
    hostSampleRate.store(rate, std::memory_order_relaxed);
}

void SampleLoader::loadSampleForPad(int padIndex, const juce::String& absolutePath,
                                     const juce::String& sampleId)
{
    if (padIndex < 0 || padIndex >= NUM_PADS)
        return;

    {
        std::lock_guard<std::mutex> lock(requestMutex);
        // Remove any existing pending request for this pad
        pendingRequests.erase(
            std::remove_if(pendingRequests.begin(), pendingRequests.end(),
                           [padIndex](const LoadRequest& r) { return r.padIndex == padIndex; }),
            pendingRequests.end());
        pendingRequests.push_back({ padIndex, absolutePath, sampleId });
    }

    notify(); // Wake background thread
}

std::shared_ptr<LoadedSample> SampleLoader::getSampleForPad(int padIndex) const
{
    if (padIndex < 0 || padIndex >= NUM_PADS)
        return nullptr;

    // This lock is extremely brief (just a shared_ptr copy).
    // In a production build you could use a lock-free mechanism, but
    // shared_ptr copy under this lightweight mutex is safe and fast enough
    // for real-time use since contention is near-zero (writes happen
    // only when a new sample finishes loading).
    std::lock_guard<std::mutex> lock(sampleMutex);
    return loadedSamples[static_cast<size_t>(padIndex)];
}

void SampleLoader::clearPad(int padIndex)
{
    if (padIndex < 0 || padIndex >= NUM_PADS)
        return;

    std::lock_guard<std::mutex> lock(sampleMutex);
    loadedSamples[static_cast<size_t>(padIndex)].reset();
}

// =========================================================================
// Waveform minimap generation
// =========================================================================

std::array<float, 128> SampleLoader::generateWaveformMinimap(
    const juce::AudioBuffer<float>& buffer, int numSamples)
{
    std::array<float, 128> minimap{};

    if (numSamples <= 0 || buffer.getNumChannels() == 0)
        return minimap;

    const int numChannels = buffer.getNumChannels();
    const double samplesPerBucket = static_cast<double>(numSamples) / 128.0;

    for (int bucket = 0; bucket < 128; ++bucket)
    {
        const int startSample = static_cast<int>(bucket * samplesPerBucket);
        const int endSample   = std::min(static_cast<int>((bucket + 1) * samplesPerBucket),
                                         numSamples);

        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* data = buffer.getReadPointer(ch);
            for (int s = startSample; s < endSample; ++s)
                peak = std::max(peak, std::abs(data[s]));
        }

        minimap[static_cast<size_t>(bucket)] = peak;
    }

    // Normalise to 0..1
    float maxPeak = *std::max_element(minimap.begin(), minimap.end());
    if (maxPeak > 0.0f)
    {
        for (auto& v : minimap)
            v /= maxPeak;
    }

    return minimap;
}

// =========================================================================
// Background thread
// =========================================================================

void SampleLoader::run()
{
    while (!threadShouldExit())
    {
        LoadRequest request;
        bool hasWork = false;

        {
            std::lock_guard<std::mutex> lock(requestMutex);
            if (!pendingRequests.empty())
            {
                request = pendingRequests.front();
                pendingRequests.erase(pendingRequests.begin());
                hasWork = true;
            }
        }

        if (hasWork)
        {
            auto loaded = loadFromFile(request.absolutePath, request.sampleId);
            if (loaded != nullptr)
            {
                std::lock_guard<std::mutex> lock(sampleMutex);
                loadedSamples[static_cast<size_t>(request.padIndex)] = std::move(loaded);
            }
        }
        else
        {
            // No work -- sleep until notified
            wait(-1);
        }
    }
}

// =========================================================================
// File loading + resampling
// =========================================================================

std::shared_ptr<LoadedSample> SampleLoader::loadFromFile(
    const juce::String& absolutePath, const juce::String& sampleId)
{
    juce::File file(absolutePath);
    if (!file.existsAsFile())
        return nullptr;

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
        return nullptr;

    const double targetRate = hostSampleRate.load(std::memory_order_relaxed);
    const int srcNumSamples  = static_cast<int>(reader->lengthInSamples);
    const int srcNumChannels = static_cast<int>(reader->numChannels);

    if (srcNumSamples <= 0 || srcNumChannels <= 0)
        return nullptr;

    // Read entire file into a temporary buffer at its native sample rate
    juce::AudioBuffer<float> srcBuffer(srcNumChannels, srcNumSamples);
    reader->read(&srcBuffer, 0, srcNumSamples, 0, true, true);

    // Resample if needed
    juce::AudioBuffer<float> finalBuffer;
    int finalNumSamples = srcNumSamples;

    if (std::abs(reader->sampleRate - targetRate) > 0.01)
    {
        // Calculate resampled length
        const double ratio = targetRate / reader->sampleRate;
        finalNumSamples = static_cast<int>(std::ceil(srcNumSamples * ratio));

        finalBuffer.setSize(srcNumChannels, finalNumSamples);
        finalBuffer.clear();

        // Simple high-quality linear interpolation resampling.
        // For production, a sinc-based resampler would be better, but
        // linear interpolation is adequate for drum samples.
        for (int ch = 0; ch < srcNumChannels; ++ch)
        {
            const float* src = srcBuffer.getReadPointer(ch);
            float* dst = finalBuffer.getWritePointer(ch);

            for (int i = 0; i < finalNumSamples; ++i)
            {
                const double srcPos = i / ratio;
                const int idx0 = static_cast<int>(srcPos);
                const int idx1 = std::min(idx0 + 1, srcNumSamples - 1);
                const float frac = static_cast<float>(srcPos - idx0);

                dst[i] = src[idx0] + frac * (src[idx1] - src[idx0]);
            }
        }
    }
    else
    {
        finalBuffer = std::move(srcBuffer);
    }

    // Build the LoadedSample
    auto loaded = std::make_shared<LoadedSample>();
    loaded->buffer      = std::move(finalBuffer);
    loaded->numChannels = srcNumChannels;
    loaded->numSamples  = finalNumSamples;
    loaded->sampleRate  = targetRate;
    loaded->sampleId    = sampleId;
    loaded->waveformMinimap = generateWaveformMinimap(loaded->buffer, finalNumSamples);

    return loaded;
}

} // namespace void_drum

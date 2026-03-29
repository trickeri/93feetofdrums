#include "SampleRegistry.h"

namespace void_drum {

// =========================================================================
// Construction
// =========================================================================

SampleRegistry::SampleRegistry()
{
    formatManager.registerBasicFormats(); // WAV, AIFF, FLAC, OGG, etc.
}

SampleRegistry::~SampleRegistry() = default;

// =========================================================================
// ISampleRegistry interface -- all reads acquire a read lock
// =========================================================================

std::vector<SampleEntry> SampleRegistry::getSamples() const
{
    const juce::ScopedReadLock sl(registryLock);
    std::vector<SampleEntry> result;
    result.reserve(entries.size());
    for (const auto& [id, entry] : entries)
        result.push_back(entry);
    return result;
}

std::vector<SampleEntry> SampleRegistry::getSamplesByCategory(const juce::String& category) const
{
    const juce::ScopedReadLock sl(registryLock);
    std::vector<SampleEntry> result;
    for (const auto& [id, entry] : entries)
    {
        if (entry.category.equalsIgnoreCase(category))
            result.push_back(entry);
    }
    return result;
}

const SampleEntry* SampleRegistry::findSample(const juce::String& sampleId) const
{
    const juce::ScopedReadLock sl(registryLock);
    auto it = entries.find(sampleId);
    if (it != entries.end())
        return &it->second;
    return nullptr;
}

std::vector<juce::String> SampleRegistry::getCategories() const
{
    const juce::ScopedReadLock sl(registryLock);
    std::set<juce::String> cats;
    for (const auto& [id, entry] : entries)
        cats.insert(entry.category.toLowerCase());

    return { cats.begin(), cats.end() };
}

// =========================================================================
// Search / filter
// =========================================================================

std::vector<SampleEntry> SampleRegistry::searchByName(const juce::String& query) const
{
    const juce::ScopedReadLock sl(registryLock);
    std::vector<SampleEntry> result;
    auto lowerQuery = query.toLowerCase();
    for (const auto& [id, entry] : entries)
    {
        if (entry.displayName.toLowerCase().contains(lowerQuery))
            result.push_back(entry);
    }
    return result;
}

std::vector<SampleEntry> SampleRegistry::filter(const juce::String& category,
                                                 const juce::String& nameQuery) const
{
    const juce::ScopedReadLock sl(registryLock);
    std::vector<SampleEntry> result;
    auto lowerQuery = nameQuery.toLowerCase();

    for (const auto& [id, entry] : entries)
    {
        if (category.isNotEmpty() && !entry.category.equalsIgnoreCase(category))
            continue;
        if (lowerQuery.isNotEmpty() && !entry.displayName.toLowerCase().contains(lowerQuery))
            continue;
        result.push_back(entry);
    }
    return result;
}

// =========================================================================
// Rebuild -- called by FolderScanner from background thread
// =========================================================================

void SampleRegistry::rebuildFromFiles(const juce::File& rootDir,
                                      const juce::Array<juce::File>& audioFiles)
{
    // Build new entries outside the write lock to minimise lock duration
    std::map<juce::String, SampleEntry> newEntries;

    for (const auto& file : audioFiles)
    {
        auto entry = buildEntry(rootDir, file);
        if (entry.id.isNotEmpty())
            newEntries[entry.id] = std::move(entry);
    }

    // Swap in the new data under a write lock
    {
        const juce::ScopedWriteLock sl(registryLock);
        entries = std::move(newEntries);
    }

    // Persist updated metadata to cache
    saveCache();

    // Notify listeners on the message thread
    notifyListeners();
}

// =========================================================================
// Entry building with metadata cache
// =========================================================================

SampleEntry SampleRegistry::buildEntry(const juce::File& rootDir,
                                       const juce::File& audioFile) const
{
    auto relativePath = audioFile.getRelativePathFrom(rootDir);
    // Normalise path separators to forward slash for portability
    relativePath = relativePath.replace("\\", "/");

    auto modTimeMs = audioFile.getLastModificationTime().toMilliseconds();

    // Check if we have a valid cached entry
    {
        const juce::ScopedLock sl(cacheLock);
        auto it = metadataCache.find(relativePath);
        if (it != metadataCache.end() && it->second.lastModifiedMs == modTimeMs)
        {
            // Update absolute path in case the root moved
            auto cached = it->second.entry;
            cached.absolutePath = audioFile.getFullPathName();
            return cached;
        }
    }

    // No valid cache -- read metadata from the file
    SampleEntry entry;
    entry.id = relativePath;
    entry.absolutePath = audioFile.getFullPathName();
    entry.category = deriveCategory(rootDir, audioFile);
    entry.displayName = deriveDisplayName(rootDir, audioFile);

    // Read audio properties
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(audioFile));

    if (reader != nullptr)
    {
        entry.sampleRate = reader->sampleRate;
        entry.channels = static_cast<int>(reader->numChannels);
        entry.durationMs = (reader->lengthInSamples / reader->sampleRate) * 1000.0;
    }

    // Generate waveform cache
    entry.waveformCache = generateWaveformCache(audioFile, const_cast<juce::AudioFormatManager&>(formatManager));

    // Store in the metadata cache
    {
        const juce::ScopedLock sl(cacheLock);
        CachedMeta meta;
        meta.entry = entry;
        meta.lastModifiedMs = modTimeMs;
        metadataCache[relativePath] = std::move(meta);
    }

    return entry;
}

// =========================================================================
// Category / display name derivation
// =========================================================================

juce::String SampleRegistry::deriveCategory(const juce::File& rootDir,
                                            const juce::File& audioFile)
{
    auto relative = audioFile.getRelativePathFrom(rootDir);
    relative = relative.replace("\\", "/");

    // The category is the first path component (top-level subfolder)
    auto slashIndex = relative.indexOf("/");
    if (slashIndex > 0)
        return relative.substring(0, slashIndex).toLowerCase();

    // File is directly in the root -- use "uncategorised"
    return "uncategorised";
}

juce::String SampleRegistry::deriveDisplayName(const juce::File& rootDir,
                                               const juce::File& audioFile)
{
    auto relative = audioFile.getRelativePathFrom(rootDir);
    relative = relative.replace("\\", "/");

    // Strip the top-level category folder to get the sub-path
    auto slashIndex = relative.indexOf("/");
    juce::String subPath;
    if (slashIndex > 0)
        subPath = relative.substring(slashIndex + 1);
    else
        subPath = relative;

    // Remove the file extension for display
    auto dotIndex = subPath.lastIndexOf(".");
    if (dotIndex > 0)
        subPath = subPath.substring(0, dotIndex);

    return subPath;
}

// =========================================================================
// Waveform minimap generation (128 peak points)
// =========================================================================

std::array<float, 128> SampleRegistry::generateWaveformCache(
    const juce::File& audioFile,
    juce::AudioFormatManager& formatMgr)
{
    std::array<float, 128> cache {};

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatMgr.createReaderFor(audioFile));

    if (reader == nullptr || reader->lengthInSamples == 0)
        return cache;

    const auto totalSamples = reader->lengthInSamples;
    const auto samplesPerBucket = static_cast<juce::int64>(
        std::max(juce::int64(1), totalSamples / 128));

    // Read in chunks
    const int chunkSize = 8192;
    juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels), chunkSize);

    juce::int64 samplePos = 0;
    int bucketIndex = 0;

    while (samplePos < totalSamples && bucketIndex < 128)
    {
        auto bucketEnd = std::min(totalSamples, (bucketIndex + 1) * samplesPerBucket);
        float peak = 0.0f;

        while (samplePos < bucketEnd && samplePos < totalSamples)
        {
            auto samplesToRead = static_cast<int>(
                std::min(static_cast<juce::int64>(chunkSize), bucketEnd - samplePos));

            reader->read(&buffer, 0, samplesToRead, samplePos, true, true);

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                auto magnitude = buffer.getMagnitude(ch, 0, samplesToRead);
                peak = std::max(peak, magnitude);
            }

            samplePos += samplesToRead;
        }

        cache[static_cast<size_t>(bucketIndex)] = peak;
        ++bucketIndex;
    }

    // Normalise so the highest peak is 1.0
    float maxPeak = *std::max_element(cache.begin(), cache.end());
    if (maxPeak > 0.0f)
    {
        for (auto& v : cache)
            v /= maxPeak;
    }

    return cache;
}

// =========================================================================
// Cache persistence (JSON)
// =========================================================================

void SampleRegistry::setCacheFile(const juce::File& file)
{
    cacheFilePath = file;
}

void SampleRegistry::loadCache()
{
    if (!cacheFilePath.existsAsFile())
        return;

    auto text = cacheFilePath.loadFileAsString();
    auto parsed = juce::JSON::parse(text);

    if (auto* arr = parsed.getArray())
    {
        const juce::ScopedLock sl(cacheLock);
        metadataCache.clear();

        for (const auto& item : *arr)
        {
            if (auto* obj = item.getDynamicObject())
            {
                CachedMeta meta;
                auto& e = meta.entry;

                e.id = obj->getProperty("id").toString();
                e.displayName = obj->getProperty("displayName").toString();
                e.category = obj->getProperty("category").toString();
                e.absolutePath = obj->getProperty("absolutePath").toString();
                e.sampleRate = static_cast<double>(obj->getProperty("sampleRate"));
                e.channels = static_cast<int>(obj->getProperty("channels"));
                e.durationMs = static_cast<double>(obj->getProperty("durationMs"));
                meta.lastModifiedMs = static_cast<juce::int64>(obj->getProperty("lastModifiedMs"));

                // Load waveform cache array
                if (auto* wfArr = obj->getProperty("waveformCache").getArray())
                {
                    for (int i = 0; i < std::min(128, wfArr->size()); ++i)
                        e.waveformCache[static_cast<size_t>(i)] = static_cast<float>((*wfArr)[i]);
                }

                if (e.id.isNotEmpty())
                    metadataCache[e.id] = std::move(meta);
            }
        }
    }
}

void SampleRegistry::saveCache() const
{
    if (cacheFilePath == juce::File())
        return;

    juce::Array<juce::var> arr;

    {
        const juce::ScopedLock sl(cacheLock);
        for (const auto& [id, meta] : metadataCache)
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty("id", meta.entry.id);
            obj->setProperty("displayName", meta.entry.displayName);
            obj->setProperty("category", meta.entry.category);
            obj->setProperty("absolutePath", meta.entry.absolutePath);
            obj->setProperty("sampleRate", meta.entry.sampleRate);
            obj->setProperty("channels", meta.entry.channels);
            obj->setProperty("durationMs", meta.entry.durationMs);
            obj->setProperty("lastModifiedMs", meta.lastModifiedMs);

            juce::Array<juce::var> wfArr;
            for (auto v : meta.entry.waveformCache)
                wfArr.add(static_cast<double>(v));
            obj->setProperty("waveformCache", wfArr);

            arr.add(juce::var(obj));
        }
    }

    auto json = juce::JSON::toString(juce::var(arr));
    cacheFilePath.replaceWithText(json);
}

// =========================================================================
// Listener notification (dispatched to message thread)
// =========================================================================

void SampleRegistry::addListener(SampleRegistryListener* listener)
{
    listeners.add(listener);
}

void SampleRegistry::removeListener(SampleRegistryListener* listener)
{
    listeners.remove(listener);
}

void SampleRegistry::notifyListeners()
{
    juce::MessageManager::callAsync([this]()
    {
        listeners.call([](SampleRegistryListener& l) { l.sampleRegistryUpdated(); });
    });
}

} // namespace void_drum

#pragma once

#include "interfaces/ISampleRegistry.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <map>
#include <vector>

namespace void_drum {

// =========================================================================
// SampleRegistry::Listener
//
// UI components subscribe to registry changes through this interface.
// Callbacks are dispatched on the message thread.
// =========================================================================
class SampleRegistryListener
{
public:
    virtual ~SampleRegistryListener() = default;

    /** Called when the registry has been rebuilt (samples added/removed/changed). */
    virtual void sampleRegistryUpdated() = 0;
};

// =========================================================================
// SampleRegistry
//
// Concrete implementation of ISampleRegistry. Thread-safe via a
// juce::ReadWriteLock -- the FolderScanner writes from a background thread
// while the UI and audio engine read concurrently.
//
// Metadata is persisted to a JSON cache file to speed up subsequent
// startups. Unchanged files (same modification time) are loaded from
// cache rather than re-scanned.
// =========================================================================
class SampleRegistry : public ISampleRegistry
{
public:
    SampleRegistry();
    ~SampleRegistry() override;

    // -- ISampleRegistry interface -----------------------------------------
    std::vector<SampleEntry> getSamples() const override;
    std::vector<SampleEntry> getSamplesByCategory(const juce::String& category) const override;
    const SampleEntry* findSample(const juce::String& sampleId) const override;
    std::vector<juce::String> getCategories() const override;

    // -- Search / filter (flat API for browser UI) -------------------------
    /** Return samples whose display name contains the query (case-insensitive). */
    std::vector<SampleEntry> searchByName(const juce::String& query) const;

    /** Combined filter: category + name search. Either parameter may be empty. */
    std::vector<SampleEntry> filter(const juce::String& category,
                                    const juce::String& nameQuery) const;

    // -- Mutation (called only by FolderScanner on background thread) ------
    /** Rebuild the entire registry from a set of discovered audio files. */
    void rebuildFromFiles(const juce::File& rootDir,
                          const juce::Array<juce::File>& audioFiles);

    // -- Cache persistence -------------------------------------------------
    /** Set the path for the metadata cache JSON file. */
    void setCacheFile(const juce::File& cacheFile);

    /** Load cached metadata from disk. */
    void loadCache();

    /** Save current metadata to disk. */
    void saveCache() const;

    // -- Listener management -----------------------------------------------
    void addListener(SampleRegistryListener* listener);
    void removeListener(SampleRegistryListener* listener);

private:
    /** Build a SampleEntry from a file, using cache if modification time matches. */
    SampleEntry buildEntry(const juce::File& rootDir,
                           const juce::File& audioFile);

    /** Derive the category from the top-level subfolder relative to root. */
    static juce::String deriveCategory(const juce::File& rootDir,
                                       const juce::File& audioFile);

    /** Derive the display name (may include nested subfolder path). */
    static juce::String deriveDisplayName(const juce::File& rootDir,
                                          const juce::File& audioFile);

    /** Generate the 128-point waveform peak cache for a sample file. */
    static std::array<float, 128> generateWaveformCache(const juce::File& audioFile,
                                                        juce::AudioFormatManager& formatMgr);

    /** Notify all listeners on the message thread. */
    void notifyListeners();

    // -- Data storage ------------------------------------------------------
    mutable juce::ReadWriteLock registryLock;

    /** Canonical storage: map from sample ID (relative path) -> SampleEntry. */
    std::map<juce::String, SampleEntry> entries;

    // -- Cache -------------------------------------------------------------
    juce::File cacheFilePath;

    /** Cached modification times: sample ID -> last mod time (ms since epoch). */
    struct CachedMeta
    {
        SampleEntry entry;
        juce::int64 lastModifiedMs = 0;
    };
    std::map<juce::String, CachedMeta> metadataCache;
    mutable juce::CriticalSection cacheLock;

    // -- Format manager for reading audio metadata -------------------------
    juce::AudioFormatManager formatManager;

    // -- Listeners ---------------------------------------------------------
    juce::ListenerList<SampleRegistryListener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleRegistry)
};

} // namespace void_drum

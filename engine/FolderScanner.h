#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <memory>

namespace void_drum {

class SampleRegistry;

// =========================================================================
// FolderScanner
//
// Watches the root samples directory (recursively) for changes using a
// timer-based polling approach (every 2 seconds). When filesystem changes
// are detected, a debounced rebuild is scheduled (500ms after the last
// detected change) so that bulk file copies don't thrash the registry.
//
// The scan runs entirely on a background thread -- it never blocks the
// audio thread.
// =========================================================================
class FolderScanner : private juce::Timer,
                      private juce::Thread
{
public:
    explicit FolderScanner(SampleRegistry& registry);
    ~FolderScanner() override;

    /** Set the root samples directory and start watching. */
    void setRootDirectory(const juce::File& rootDir);

    /** Get the currently configured root directory. */
    juce::File getRootDirectory() const;

    /** Force an immediate rescan (called from UI "Refresh" button). */
    void triggerRescan();

    /** Supported audio file extensions (lowercase, without dot). */
    static bool isSupportedExtension(const juce::String& extension);

private:
    // -- Timer callback (polls for filesystem changes every 2s) -----------
    void timerCallback() override;

    // -- Background thread (performs the actual scan) ----------------------
    void run() override;

    /** Collect all supported audio files recursively under rootDir. */
    juce::Array<juce::File> collectAudioFiles(const juce::File& dir) const;

    /** Build a snapshot of file paths + modification times for change detection. */
    using FileSnapshot = std::map<juce::String, juce::int64>;
    FileSnapshot buildSnapshot(const juce::Array<juce::File>& files) const;

    /** Check if the filesystem has changed since the last snapshot. */
    bool hasFilesystemChanged();

    // -- Members ----------------------------------------------------------
    SampleRegistry& registry;
    juce::File rootDirectory;
    mutable juce::CriticalSection rootDirLock;

    FileSnapshot lastSnapshot;
    juce::CriticalSection snapshotLock;

    std::atomic<bool> rescanRequested { false };
    std::atomic<int64_t> lastChangeDetectedMs { 0 };
    static constexpr int64_t debounceMs = 500;
    static constexpr int pollIntervalMs = 2000;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FolderScanner)
};

} // namespace void_drum

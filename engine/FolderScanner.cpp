#include "FolderScanner.h"
#include "SampleRegistry.h"

namespace void_drum {

// =========================================================================
// Construction / destruction
// =========================================================================

FolderScanner::FolderScanner(SampleRegistry& reg)
    : juce::Thread("VOID_FolderScanner"),
      registry(reg)
{
}

FolderScanner::~FolderScanner()
{
    stopTimer();
    signalThreadShouldExit();
    notify();                 // wake the thread if sleeping
    stopThread(5000);         // wait up to 5s for graceful shutdown
}

// =========================================================================
// Public API
// =========================================================================

void FolderScanner::setRootDirectory(const juce::File& rootDir)
{
    {
        const juce::ScopedLock sl(rootDirLock);
        rootDirectory = rootDir;
    }

    // Kick off an initial scan
    triggerRescan();

    // Start the polling timer
    startTimer(pollIntervalMs);

    // Ensure the background thread is running
    if (!isThreadRunning())
        startThread(juce::Thread::Priority::background);
}

juce::File FolderScanner::getRootDirectory() const
{
    const juce::ScopedLock sl(rootDirLock);
    return rootDirectory;
}

void FolderScanner::triggerRescan()
{
    rescanRequested.store(true);
    notify(); // wake background thread
}

bool FolderScanner::isSupportedExtension(const juce::String& extension)
{
    auto ext = extension.toLowerCase();
    return ext == "wav" || ext == "flac" || ext == "ogg" || ext == "aiff";
}

// =========================================================================
// Timer callback -- polls for filesystem changes (runs on message thread)
// =========================================================================

void FolderScanner::timerCallback()
{
    if (hasFilesystemChanged())
    {
        // Record the time of the last detected change for debouncing
        lastChangeDetectedMs.store(juce::Time::currentTimeMillis());
        notify(); // wake background thread to handle debounce
    }
}

// =========================================================================
// Background thread -- performs scan after debounce or on explicit request
// =========================================================================

void FolderScanner::run()
{
    while (!threadShouldExit())
    {
        bool shouldScan = false;

        if (rescanRequested.exchange(false))
        {
            shouldScan = true;
        }
        else
        {
            auto lastChange = lastChangeDetectedMs.load();
            if (lastChange > 0)
            {
                auto elapsed = juce::Time::currentTimeMillis() - lastChange;
                if (elapsed >= debounceMs)
                {
                    shouldScan = true;
                    lastChangeDetectedMs.store(0);
                }
                else
                {
                    // Not yet debounced -- wait the remaining time
                    wait(static_cast<int>(debounceMs - elapsed + 10));
                    continue;
                }
            }
        }

        if (shouldScan)
        {
            juce::File root;
            {
                const juce::ScopedLock sl(rootDirLock);
                root = rootDirectory;
            }

            if (root.isDirectory())
            {
                auto audioFiles = collectAudioFiles(root);
                registry.rebuildFromFiles(root, audioFiles);

                // Update the snapshot after a successful rebuild
                {
                    const juce::ScopedLock sl(snapshotLock);
                    lastSnapshot = buildSnapshot(audioFiles);
                }
            }
        }

        // Sleep until notified or until next poll cycle
        if (!threadShouldExit())
            wait(pollIntervalMs);
    }
}

// =========================================================================
// File collection
// =========================================================================

juce::Array<juce::File> FolderScanner::collectAudioFiles(const juce::File& dir) const
{
    juce::Array<juce::File> results;

    for (const auto& entry : juce::RangedDirectoryIterator(
             dir, true, "*", juce::File::findFiles))
    {
        auto ext = entry.getFile().getFileExtension().trimCharactersAtStart(".");
        if (isSupportedExtension(ext))
            results.add(entry.getFile());
    }

    return results;
}

// =========================================================================
// Change detection via snapshots
// =========================================================================

FolderScanner::FileSnapshot FolderScanner::buildSnapshot(const juce::Array<juce::File>& files) const
{
    FileSnapshot snap;
    for (const auto& f : files)
        snap[f.getFullPathName()] = f.getLastModificationTime().toMilliseconds();
    return snap;
}

bool FolderScanner::hasFilesystemChanged()
{
    juce::File root;
    {
        const juce::ScopedLock sl(rootDirLock);
        root = rootDirectory;
    }

    if (!root.isDirectory())
        return false;

    auto files = collectAudioFiles(root);
    auto newSnapshot = buildSnapshot(files);

    const juce::ScopedLock sl(snapshotLock);
    if (newSnapshot != lastSnapshot)
        return true;

    return false;
}

} // namespace void_drum

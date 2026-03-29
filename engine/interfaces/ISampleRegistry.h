#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <string>
#include <vector>

namespace void_drum {

// -------------------------------------------------------------------------
// SampleEntry: metadata for a single sample discovered in the samples folder.
//
// The canonical ID is the relative path from the samples root directory
// (e.g. "kicks/808_quantum_01.wav"). All preset references use this ID so
// that kits are portable as long as the folder structure ships together.
// -------------------------------------------------------------------------
struct SampleEntry
{
    /** Unique identifier -- relative path from samples root (e.g. "kicks/808_deep.wav"). */
    juce::String id;

    /** Human-readable display name (filename without extension). */
    juce::String displayName;

    /** Category derived from the top-level subfolder name (e.g. "kicks"). */
    juce::String category;

    /** Absolute filesystem path for loading. */
    juce::String absolutePath;

    /** Native sample rate of the file in Hz. */
    double sampleRate = 0.0;

    /** Number of audio channels (1 = mono, 2 = stereo). */
    int channels = 0;

    /** Duration in milliseconds. */
    double durationMs = 0.0;

    /** 128-point normalised waveform peak cache for UI minimap display. */
    std::array<float, 128> waveformCache {};
};

// -------------------------------------------------------------------------
// ISampleRegistry: abstract read-only view of the discovered sample database.
//
// The FolderScanner owns the concrete implementation and publishes updates.
// UI and engine code consume this interface -- they never write to it.
// -------------------------------------------------------------------------
class ISampleRegistry
{
public:
    virtual ~ISampleRegistry() = default;

    /** Return every sample currently in the registry. */
    virtual std::vector<SampleEntry> getSamples() const = 0;

    /** Return all samples belonging to a given category (case-insensitive match). */
    virtual std::vector<SampleEntry> getSamplesByCategory(const juce::String& category) const = 0;

    /** Look up a single sample by its canonical ID (relative path). Returns nullptr if not found. */
    virtual const SampleEntry* findSample(const juce::String& sampleId) const = 0;

    /** Return a sorted list of every discovered category name. */
    virtual std::vector<juce::String> getCategories() const = 0;
};

} // namespace void_drum

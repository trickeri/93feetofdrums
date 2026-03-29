#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <string>

namespace void_drum {

// -------------------------------------------------------------------------
// Constants
// -------------------------------------------------------------------------

/** Number of pads in a single bank. */
inline constexpr int NUM_PADS = 16;

/** Maximum simultaneous voices across all pads. */
inline constexpr int MAX_VOICES = 64;

/** No choke group assigned. */
inline constexpr int CHOKE_GROUP_NONE = 0;

// -------------------------------------------------------------------------
// PadState: complete per-pad parameter snapshot.
//
// The audio engine owns the authoritative copy. UI and MIDI agents read
// from and write to this state via the AudioProcessorValueTreeState
// parameter tree (thread-safe atomic reads / message-thread writes).
// -------------------------------------------------------------------------
struct PadState
{
    // -- Sample assignment --------------------------------------------------
    /** Canonical sample ID (relative path from samples root), empty if unassigned. */
    juce::String assignedSampleId;

    // -- Level & panning ----------------------------------------------------
    float volume          = 0.8f;   ///< 0.0 .. 1.0
    float pan             = 0.0f;   ///< -1.0 (L) .. +1.0 (R)

    // -- Pitch --------------------------------------------------------------
    float pitch           = 0.0f;   ///< Semitones, -24 .. +24

    // -- Filter -------------------------------------------------------------
    float filterCutoff    = 1.0f;   ///< Normalised 0.0 .. 1.0 (maps to 20 Hz .. 20 kHz)
    float filterResonance = 0.0f;   ///< 0.0 .. 1.0

    // -- Drive / saturation -------------------------------------------------
    float drive           = 0.0f;   ///< 0.0 (clean) .. 1.0 (max drive)

    // -- Bitcrusher ---------------------------------------------------------
    float bitcrushDepth   = 1.0f;   ///< Bit depth, normalised 0.0 .. 1.0 (1.0 = full resolution)
    float bitcrushRate    = 1.0f;   ///< Sample rate reduction, normalised 0.0 .. 1.0 (1.0 = no crush)

    // -- Sends --------------------------------------------------------------
    float reverbSend      = 0.0f;   ///< 0.0 .. 1.0

    // -- Routing / grouping -------------------------------------------------
    bool  mute            = false;
    bool  solo            = false;
    int   chokeGroup      = CHOKE_GROUP_NONE; ///< 0 = none, 1..N = group ID

    // -- Playback mode (future) ---------------------------------------------
    // bool loop           = false;
    // float attackMs      = 0.0f;
    // float releaseMs     = 200.0f;
};

// -------------------------------------------------------------------------
// PadBank: a bank of NUM_PADS pad states.
// -------------------------------------------------------------------------
using PadBank = std::array<PadState, NUM_PADS>;

} // namespace void_drum

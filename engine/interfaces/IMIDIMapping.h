#pragma once

#include "IPadState.h"
#include <juce_core/juce_core.h>
#include <array>
#include <functional>
#include <map>

namespace void_drum {

// -------------------------------------------------------------------------
// Velocity curve shapes used when converting MIDI velocity (0-127) to a
// gain multiplier (0.0 .. 1.0).
// -------------------------------------------------------------------------
enum class VelocityCurve
{
    Linear,      ///< 1:1 mapping
    Exponential, ///< Gentle at low velocity, steep at high
    SCurve       ///< Soft at extremes, steep in the middle
};

// -------------------------------------------------------------------------
// MIDIMapping: snapshot of the current note-to-pad routing configuration.
// -------------------------------------------------------------------------
struct MIDIMapping
{
    /** Maps a MIDI note number (0-127) to a pad index (0 .. NUM_PADS-1).
     *  Notes not present in the map are ignored. */
    std::map<int, int> noteToPad;

    /** Active velocity curve. */
    VelocityCurve velocityCurve = VelocityCurve::Linear;
};

// -------------------------------------------------------------------------
// Default General MIDI drum map for pads 0-15.
// -------------------------------------------------------------------------
inline MIDIMapping createDefaultMIDIMapping()
{
    MIDIMapping m;
    // Standard GM drum assignments
    m.noteToPad[36] = 0;   // Kick
    m.noteToPad[38] = 1;   // Snare
    m.noteToPad[42] = 2;   // Closed Hi-Hat
    m.noteToPad[46] = 3;   // Open Hi-Hat
    m.noteToPad[41] = 4;   // Low Floor Tom
    m.noteToPad[43] = 5;   // High Floor Tom
    m.noteToPad[45] = 6;   // Low Tom
    m.noteToPad[47] = 7;   // Low-Mid Tom
    m.noteToPad[48] = 8;   // Hi-Mid Tom
    m.noteToPad[50] = 9;   // High Tom
    m.noteToPad[49] = 10;  // Crash Cymbal 1
    m.noteToPad[51] = 11;  // Ride Cymbal 1
    m.noteToPad[52] = 12;  // Chinese Cymbal
    m.noteToPad[53] = 13;  // Ride Bell
    m.noteToPad[39] = 14;  // Hand Clap
    m.noteToPad[37] = 15;  // Side Stick
    m.velocityCurve = VelocityCurve::Linear;
    return m;
}

// -------------------------------------------------------------------------
// IMIDIMapper: abstract interface for MIDI-to-pad routing.
//
// Agent 6 (MIDI & Host Integration) owns the concrete implementation.
// The audio engine reads the mapping each processBlock.
// -------------------------------------------------------------------------
class IMIDIMapper
{
public:
    virtual ~IMIDIMapper() = default;

    /** Get the current mapping (thread-safe snapshot). */
    virtual MIDIMapping getMapping() const = 0;

    /** Replace the entire mapping. */
    virtual void setMapping(const MIDIMapping& mapping) = 0;

    /** Enter or exit MIDI Learn mode. When active, the next received note
     *  is assigned to the target pad and learn mode exits automatically.
     *  @param padIndex  The pad to assign the next note to, or -1 to cancel. */
    virtual void midiLearn(int padIndex) = 0;
};

} // namespace void_drum

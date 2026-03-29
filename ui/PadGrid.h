#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../engine/interfaces/IPadState.h"
#include "VoidLookAndFeel.h"
#include <array>
#include <functional>

// =========================================================================
// PadGrid
//
// 4x4 grid of square drum pads (16 pads total). Each pad displays:
//   - Sample name (truncated)
//   - MIDI note assignment
//   - Category colour indicator
//   - Greek letter designator
//   - Pad number
//
// Supports click-to-trigger, hit flash animation, pulsing selected-pad
// border, and drag-and-drop for sample assignment.
// =========================================================================
class PadGrid : public juce::Component,
                public juce::DragAndDropTarget,
                private juce::Timer
{
public:
    static constexpr int GRID_COLS = 4;
    static constexpr int GRID_ROWS = 4;

    /** Callback: pad triggered by click. (padIndex 0..15, velocity 0..1) */
    using PadTriggerCallback = std::function<void(int padIndex, float velocity)>;

    /** Callback: sample dropped on pad. (padIndex, sampleId) */
    using SampleDropCallback = std::function<void(int padIndex, const juce::String& sampleId)>;

    PadGrid();
    ~PadGrid() override = default;

    void setPadTriggerCallback(PadTriggerCallback cb)  { onPadTrigger = std::move(cb); }
    void setSampleDropCallback(SampleDropCallback cb)  { onSampleDrop = std::move(cb); }

    /** Set the selected/active pad (highlighted with pulsing border). */
    void setSelectedPad(int padIndex);
    int  getSelectedPad() const { return selectedPad; }

    /** Update the display info for a pad. */
    void setPadInfo(int padIndex, const juce::String& sampleName,
                    const juce::String& category, int midiNote);

    /** Externally trigger a hit flash on a pad (e.g. from MIDI). */
    void triggerHit(int padIndex, float velocity);

    // -- Component overrides ------------------------------------------------
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    // -- DragAndDropTarget overrides ----------------------------------------
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;

private:
    void timerCallback() override;

    // -- Per-pad data -------------------------------------------------------
    struct PadInfo
    {
        juce::String sampleName;
        juce::String category;
        int          midiNote       = 0;
        float        hitBrightness  = 0.0f;
    };

    std::array<PadInfo, void_drum::NUM_PADS> pads;
    int selectedPad = 0;
    int dropTargetPad = -1;      // pad currently being hovered with drag
    float pulsePhase = 0.0f;     // for selected pad pulsing glow

    PadTriggerCallback onPadTrigger;
    SampleDropCallback onSampleDrop;

    // Greek letters for pads 0-15
    static constexpr const char* kGreekLetters[16] = {
        "\xCE\xA9",  // 0  Omega  (Kick)
        "\xCE\xA3",  // 1  Sigma  (Snare)
        "\xCE\x98",  // 2  Theta  (Hat)
        "\xCE\x94",  // 3  Delta  (Tom1)
        "\xCE\xA6",  // 4  Phi    (Tom2)
        "\xCE\xA8",  // 5  Psi    (Tom3)
        "\xCE\xA0",  // 6  Pi     (Ride)
        "\xCE\x9B",  // 7  Lambda (Crash)
        "\xCE\x9E",  // 8  Xi
        "\xCE\x93",  // 9  Gamma
        "\xCE\x92",  // 10 Beta
        "\xCE\x91",  // 11 Alpha
        "\xCE\x96",  // 12 Zeta
        "\xCE\x97",  // 13 Eta
        "\xCE\x9A",  // 14 Kappa
        "\xCE\x9C",  // 15 Mu
    };

    // Default MIDI notes (GM mapping)
    static constexpr int kDefaultMidiNotes[16] = {
        36, 38, 42, 46, 41, 43, 45, 47, 48, 50, 49, 51, 52, 53, 39, 37
    };

    juce::Rectangle<int> getPadBounds(int padIndex) const;
    int hitTestPad(juce::Point<int> pos) const;
    juce::Colour getCategoryColour(const juce::String& category) const;

    void drawPad(juce::Graphics& g, int padIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PadGrid)
};

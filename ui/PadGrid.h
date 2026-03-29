#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../engine/interfaces/IPadState.h"
#include "VoidLookAndFeel.h"
#include <array>
#include <functional>
#include <memory>

// =========================================================================
// PadCell — a single pad with inline mixer controls on the right
// =========================================================================
class PadCell : public juce::Component
{
public:
    PadCell(int index);
    ~PadCell() override = default;

    void connectToParameters(juce::AudioProcessorValueTreeState& apvts);
    void setSelected(bool selected);
    void setSampleName(const juce::String& name);
    void setCategory(const juce::String& cat);
    void setMidiNote(int note);
    void triggerHit(float velocity);

    int getPadIndex() const { return padIndex; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Expose for animation timer
    void decayHit();
    bool isHitActive() const { return hitBrightness > 0.0f; }

private:
    int padIndex = 0;
    bool isSelected = false;
    juce::String sampleName;
    juce::String category;
    int midiNote = 36;
    float hitBrightness = 0.0f;

    // Mixer controls
    juce::Slider volumeFader;
    juce::Slider panKnob;
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;

    juce::Colour getCategoryColour() const;

    // Greek letters for pads 0-15
    static constexpr const char* kGreekLetters[16] = {
        "\xCE\xA9", "\xCE\xA3", "\xCE\x98", "\xCE\x94",
        "\xCE\xA6", "\xCE\xA8", "\xCE\xA0", "\xCE\x9B",
        "\xCE\x9E", "\xCE\x93", "\xCE\x92", "\xCE\x91",
        "\xCE\x96", "\xCE\x97", "\xCE\x9A", "\xCE\x9C",
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PadCell)
};

// =========================================================================
// PadGrid
//
// 4x4 grid of PadCells. Each cell has the pad display on the left (~72%)
// and inline mixer controls (volume fader, pan knob, M/S) on the right.
// Replaces the separate MixerStrip — all controls are right next to the pad.
// =========================================================================
class PadGrid : public juce::Component,
                public juce::DragAndDropTarget,
                private juce::Timer
{
public:
    static constexpr int GRID_COLS = 4;
    static constexpr int GRID_ROWS = 4;

    using PadTriggerCallback = std::function<void(int padIndex, float velocity)>;
    using SampleDropCallback = std::function<void(int padIndex, const juce::String& sampleId)>;

    PadGrid();
    ~PadGrid() override = default;

    void setPadTriggerCallback(PadTriggerCallback cb)  { onPadTrigger = std::move(cb); }
    void setSampleDropCallback(SampleDropCallback cb)  { onSampleDrop = std::move(cb); }

    void setSelectedPad(int padIndex);
    int  getSelectedPad() const { return selectedPad; }

    void setPadInfo(int padIndex, const juce::String& sampleName,
                    const juce::String& category, int midiNote);

    void triggerHit(int padIndex, float velocity);

    /** Connect all pad cells to APVTS parameters. */
    void connectToParameters(juce::AudioProcessorValueTreeState& apvts);

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    // DragAndDropTarget overrides
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;

private:
    void timerCallback() override;

    std::array<std::unique_ptr<PadCell>, void_drum::NUM_PADS> cells;
    int selectedPad = 0;
    int dropTargetPad = -1;
    float pulsePhase = 0.0f;

    PadTriggerCallback onPadTrigger;
    SampleDropCallback onSampleDrop;

    static constexpr int kDefaultMidiNotes[16] = {
        36, 38, 42, 46, 41, 43, 45, 47, 48, 50, 49, 51, 52, 53, 39, 37
    };

    juce::Rectangle<int> getCellBounds(int padIndex) const;
    int hitTestPad(juce::Point<int> pos) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PadGrid)
};

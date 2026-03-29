#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "VoidLookAndFeel.h"
#include "../engine/interfaces/IPadState.h"
#include <array>
#include <memory>

// =========================================================================
// MixerStrip
//
// Bottom panel — horizontal strip of 16 mini channel strips + master.
// Each channel: vertical fader (volume), pan knob, mute/solo buttons,
// stereo level meter (peak hold), pad number label.
//
// Colours:
//   Meter green    -- #00FF66
//   Peak hold      -- accent-red (#FF2244)
//   Surface bg     -- #0D0D0D
//   Selected pad   -- accent-cyan (#00F0FF) border
//   Fader track    -- ghost (#333333)
//   Fader thumb    -- text-primary (#F0F0F0)
// =========================================================================

// =========================================================================
// LevelMeter — stereo peak meter with hold
// =========================================================================
class LevelMeter : public juce::Component,
                   public juce::Timer
{
public:
    LevelMeter();
    ~LevelMeter() override = default;

    /** Set the current peak levels (0..1) for left and right channels. */
    void setLevels(float left, float right);

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    float levelL = 0.0f, levelR = 0.0f;
    float peakL  = 0.0f, peakR  = 0.0f;
    int peakHoldCounterL = 0, peakHoldCounterR = 0;

    static constexpr int kPeakHoldFrames = 30; // ~1 second at 30fps
    static constexpr float kDecayRate = 0.92f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};

// =========================================================================
// ChannelStrip — a single pad's mixer channel
// =========================================================================
class ChannelStrip : public juce::Component
{
public:
    ChannelStrip();
    ~ChannelStrip() override = default;

    /** Set the pad index (0-based) and connect to APVTS parameters. */
    void connectToParameters(juce::AudioProcessorValueTreeState& apvts, int padIndex);

    /** Set whether this strip is the currently selected pad. */
    void setSelected(bool selected);

    /** Set the display label text (pad number). */
    void setPadLabel(const juce::String& label);

    /** Update level meter values (called from a timer on the parent). */
    void setMeterLevels(float left, float right);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    int padIdx = 0;
    bool isSelected = false;

    juce::Slider volumeFader;
    juce::Slider panKnob;
    juce::TextButton muteButton { "M" };
    juce::TextButton soloButton { "S" };
    LevelMeter meter;
    juce::Label padLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> panAttachment;

    bool muted = false;
    bool soloed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
};

// =========================================================================
// MixerStrip — the full bottom-panel mixer
// =========================================================================
class MixerStrip : public juce::Component,
                   public juce::Timer
{
public:
    MixerStrip();
    ~MixerStrip() override;

    /** Connect all channel strips and master fader to the APVTS. */
    void connectToParameters(juce::AudioProcessorValueTreeState& apvts);

    /** Set the currently selected pad index (0-based). Highlights that strip. */
    void setSelectedPad(int padIndex);

    /** Set meter levels for a given pad (called from processBlock via async update). */
    void setMeterLevels(int padIndex, float left, float right);

    /** Set master meter levels. */
    void setMasterMeterLevels(float left, float right);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    std::array<std::unique_ptr<ChannelStrip>, void_drum::NUM_PADS> channels;

    // Master channel
    juce::Slider masterFader;
    LevelMeter masterMeter;
    juce::Label masterLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterVolumeAttachment;

    int selectedPad = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerStrip)
};

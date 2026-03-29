#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "VoidLookAndFeel.h"
#include "WaveformDisplay.h"
#include "../engine/interfaces/IPadState.h"
#include <array>
#include <memory>

// =========================================================================
// VoidKnob
//
// Custom-drawn rotary knob: thin arc indicator on void-black background,
// monospace value readout below. Used throughout the FX panel.
// =========================================================================
class VoidKnob : public juce::Component
{
public:
    VoidKnob(const juce::String& name, const juce::String& suffix = "");
    ~VoidKnob() override = default;

    juce::Slider& getSlider() { return slider; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Slider slider;
    juce::Label nameLabel;
    juce::String valueSuffix;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidKnob)
};

// =========================================================================
// FXSection
//
// A group of knobs with a bypass toggle button and section header.
// =========================================================================
class FXSection : public juce::Component
{
public:
    FXSection(const juce::String& title);
    ~FXSection() override = default;

    /** Add a knob to this section. Ownership is transferred. */
    void addKnob(VoidKnob* knob);

    /** Get the bypass button (so parent can read its state). */
    juce::TextButton& getBypassButton() { return bypassButton; }

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::String sectionTitle;
    juce::TextButton bypassButton { "BYP" };
    juce::OwnedArray<VoidKnob> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXSection)
};

// =========================================================================
// FXPanel
//
// Right panel — context-sensitive FX controls for the currently selected pad.
// Shows: Filter (cutoff + resonance), Drive, Bitcrush (depth + rate),
// Reverb send, plus a waveform display with start/end markers.
//
// When the selected pad changes, all knob attachments are re-created to
// point at the new pad's APVTS parameters.
// =========================================================================
class FXPanel : public juce::Component
{
public:
    FXPanel();
    ~FXPanel() override = default;

    /** Connect to the plugin's APVTS. Must be called once after construction. */
    void setAPVTS(juce::AudioProcessorValueTreeState* apvts);

    /** Callback for when waveform markers are dragged. Set by the editor. */
    std::function<void(int padIndex, float startNorm, float endNorm)> onMarkersDragged;

    /** Switch to display/control a different pad's FX. 0-based index. */
    void setSelectedPad(int padIndex);

    /** Update the waveform display with peak data for the current pad. */
    void setWaveformData(const std::array<float, 128>& peaks);

    /** Set start/end marker positions. */
    void setMarkerPositions(float startNorm, float endNorm);

    // -- Component overrides -------------------------------------------------
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState* apvtsPtr = nullptr;
    int currentPad = 0;

    // -- Header --------------------------------------------------------------
    juce::Label headerLabel;

    // -- FX Sections ---------------------------------------------------------
    FXSection filterSection  { "FILTER" };
    FXSection driveSection   { "DRIVE" };
    FXSection crushSection   { "CRUSH" };
    FXSection reverbSection  { "REVERB" };

    // -- Knobs (raw pointers; owned by their FXSection) ----------------------
    VoidKnob* cutoffKnob     = nullptr;
    VoidKnob* resonanceKnob  = nullptr;
    VoidKnob* driveKnob      = nullptr;
    VoidKnob* crushDepthKnob = nullptr;
    VoidKnob* crushRateKnob  = nullptr;
    VoidKnob* reverbSendKnob = nullptr;

    // -- Parameter attachments (rebuilt when selected pad changes) ------------
    struct Attachments
    {
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoff;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonance;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> drive;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crushDepth;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crushRate;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> reverbSend;
    };
    Attachments attachments;

    // -- Waveform display ----------------------------------------------------
    WaveformDisplay waveformDisplay;

    // -- Helpers -------------------------------------------------------------
    void rebuildAttachments();
    static juce::String padParamId(int padIndex, const juce::String& suffix);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FXPanel)
};

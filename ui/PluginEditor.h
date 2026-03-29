#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "VoidLookAndFeel.h"
#include "DrumKitView.h"
#include "PadGrid.h"
#include "SampleBrowser.h"
#include "FXPanel.h"

// Forward declaration -- defined in engine/PluginProcessor.h
class VOIDDrumEngineProcessor;

// =========================================================================
// VOIDDrumEngineEditor
// =========================================================================
class VOIDDrumEngineEditor : public juce::AudioProcessorEditor,
                             public juce::DragAndDropContainer,
                             public SampleBrowserListener
{
public:
    explicit VOIDDrumEngineEditor(VOIDDrumEngineProcessor& processor);
    ~VOIDDrumEngineEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // -- SampleBrowserListener -----------------------------------------------
    void samplePreviewRequested(const juce::String& sampleId) override;
    void sampleAssignRequested(const juce::String& sampleId) override;

private:
    VOIDDrumEngineProcessor& processorRef;

    VoidLookAndFeel voidLookAndFeel;
    DrumKitView     drumKitView;
    PadGrid         padGrid;
    SampleBrowser   sampleBrowser;
    FXPanel         fxPanel;

    // Master volume (was in MixerStrip, now standalone at bottom-right)
    juce::Slider masterFader;
    juce::Label  masterLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterVolumeAttachment;

    void loadSampleToPad(int padIndex, const juce::String& sampleId);

    int selectedPad = 0;

    // Layout constants
    static constexpr int kTitleBarHeight = 40;
    static constexpr int kBrowserWidth   = 200;
    static constexpr int kFXPanelWidth   = 200;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VOIDDrumEngineEditor)
};

#include "PluginEditor.h"
#include "../engine/PluginProcessor.h"

// =========================================================================
// Construction
// =========================================================================

VOIDDrumEngineEditor::VOIDDrumEngineEditor(VOIDDrumEngineProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p)
{
    // Apply custom look and feel
    setLookAndFeel(&voidLookAndFeel);

    // --- DrumKitView setup ---
    addAndMakeVisible(drumKitView);
    drumKitView.setPadTriggerCallback([this](int padIndex, float velocity)
    {
        // Trigger the hit animation on the pad grid too
        padGrid.triggerHit(padIndex, velocity);
        padGrid.setSelectedPad(padIndex);

        // TODO (Agent 2 integration): send MIDI note-on to processor
        //   processorRef.triggerPad(padIndex, velocity);
    });

    // --- PadGrid setup ---
    addAndMakeVisible(padGrid);
    padGrid.setPadTriggerCallback([this](int padIndex, float velocity)
    {
        // Trigger the hit animation on the kit view too
        drumKitView.triggerHit(padIndex, velocity);

        // TODO (Agent 2 integration): send MIDI note-on to processor
        //   processorRef.triggerPad(padIndex, velocity);
    });

    padGrid.setSampleDropCallback([this](int padIndex, const juce::String& sampleId)
    {
        // TODO (Agent 3 integration): assign sample to pad
        //   processorRef.assignSampleToPad(padIndex, sampleId);
        juce::ignoreUnused(padIndex, sampleId);
    });

    // --- SampleBrowser setup ---
    addAndMakeVisible(sampleBrowser);
    sampleBrowser.addListener(this);

    // --- MixerStrip setup ---
    addAndMakeVisible(mixerStrip);
    mixerStrip.connectToParameters(processorRef.getAPVTS());
    mixerStrip.setSelectedPad(selectedPad);

    // --- FXPanel setup ---
    addAndMakeVisible(fxPanel);
    fxPanel.setAPVTS(&processorRef.getAPVTS());
    fxPanel.setSelectedPad(selectedPad);

    // Default window size matching the wireframe layout
    setSize(1100, 750);
    setResizable(true, true);
    setResizeLimits(800, 600, 1920, 1080);
}

VOIDDrumEngineEditor::~VOIDDrumEngineEditor()
{
    sampleBrowser.removeListener(this);
    setLookAndFeel(nullptr);
}

// =========================================================================
// Paint
// =========================================================================

void VOIDDrumEngineEditor::paint(juce::Graphics& g)
{
    // Void-black background
    g.fillAll(VoidLookAndFeel::voidBlack());

    // --- Title bar ---
    auto titleBar = getLocalBounds().removeFromTop(kTitleBarHeight).toFloat();

    // Title bar background
    g.setColour(VoidLookAndFeel::surface());
    g.fillRect(titleBar);

    // Title bar bottom border
    g.setColour(VoidLookAndFeel::ghost());
    g.fillRect(titleBar.getX(), titleBar.getBottom() - 1.0f, titleBar.getWidth(), 1.0f);

    // Title text
    g.setFont(VoidLookAndFeel::getMonoFontBold(18.0f));
    g.setColour(VoidLookAndFeel::textPrimary());
    g.drawText(juce::CharPointer_UTF8("\xCE\x98  V\xC3\x98ID DRUM ENGINE"),
               titleBar.reduced(12.0f, 0.0f),
               juce::Justification::centredLeft, true);

    // Version text (right side)
    g.setFont(VoidLookAndFeel::getMonoFont(11.0f));
    g.setColour(VoidLookAndFeel::textDim());
    g.drawText("v0.1.0", titleBar.reduced(12.0f, 0.0f),
               juce::Justification::centredRight, true);

    // --- Section divider between kit view and pad grid ---
    // (drawn in the gap between the two components)
    auto contentBounds = getLocalBounds().withTrimmedTop(kTitleBarHeight);
    float splitY = contentBounds.getY() + contentBounds.getHeight() * 0.58f;

    g.setColour(VoidLookAndFeel::ghost().withAlpha(0.4f));
    g.fillRect(contentBounds.getX() + 20.0f, splitY - 0.5f,
               static_cast<float>(contentBounds.getWidth() - 40), 1.0f);

    // Decorative Greek accent on divider
    g.setFont(VoidLookAndFeel::getMonoFont(10.0f));
    g.setColour(VoidLookAndFeel::textDim());
    g.drawText(juce::CharPointer_UTF8("\xCE\xA3\xCE\xA9\xCE\x98  PAD GRID"),
               static_cast<float>(contentBounds.getCentreX() - 60),
               splitY - 8.0f, 120.0f, 16.0f,
               juce::Justification::centred, false);
}

// =========================================================================
// Layout
// =========================================================================

void VOIDDrumEngineEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(kTitleBarHeight);

    // Bottom: mixer strip
    mixerStrip.setBounds(bounds.removeFromBottom(kMixerHeight));

    // Left: sample browser
    int browserW = juce::jmax(kBrowserWidth, bounds.getWidth() / 5);
    sampleBrowser.setBounds(bounds.removeFromLeft(browserW));

    // Right: FX panel
    int fxW = juce::jmax(kFXPanelWidth, bounds.getWidth() / 4);
    fxPanel.setBounds(bounds.removeFromRight(fxW));

    // Centre: DrumKitView (top 58%) and PadGrid (bottom 42%)
    auto centreArea = bounds;
    int kitHeight = static_cast<int>(centreArea.getHeight() * 0.58f);
    auto kitArea = centreArea.removeFromTop(kitHeight).reduced(4);
    auto gridArea = centreArea.reduced(4);

    drumKitView.setBounds(kitArea);
    padGrid.setBounds(gridArea);
}

// =========================================================================
// SampleBrowserListener callbacks
// =========================================================================

void VOIDDrumEngineEditor::samplePreviewRequested(const juce::String& sampleId)
{
    // TODO (Agent 2): forward to preview playback engine
    juce::ignoreUnused(sampleId);
    DBG("Preview requested: " + sampleId);
}

void VOIDDrumEngineEditor::sampleAssignRequested(const juce::String& sampleId)
{
    // TODO (Agent 2/3): assign sample to currently selected pad
    juce::ignoreUnused(sampleId);
    DBG("Assign to pad " + juce::String(selectedPad) + ": " + sampleId);
}

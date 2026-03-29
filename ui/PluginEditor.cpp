#include "PluginEditor.h"
#include "../engine/PluginProcessor.h"

static void editorLog(const juce::String& msg)
{
    auto logFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("VOID Drum Engine")
                       .getChildFile("debug_log.txt");
    logFile.appendText("[EDITOR] " + msg + "\n", false, false, nullptr);
}


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
        editorLog("DrumKitView trigger: pad=" + juce::String(padIndex) + " vel=" + juce::String(velocity));
        padGrid.triggerHit(padIndex, velocity);
        padGrid.setSelectedPad(padIndex);
        processorRef.triggerPad(padIndex, velocity);
    });

    // --- PadGrid setup (now includes mixer controls per-pad) ---
    addAndMakeVisible(padGrid);
    padGrid.connectToParameters(processorRef.getAPVTS());
    padGrid.setPadTriggerCallback([this](int padIndex, float velocity)
    {
        editorLog("PadGrid trigger: pad=" + juce::String(padIndex) + " vel=" + juce::String(velocity));
        drumKitView.triggerHit(padIndex, velocity);
        selectedPad = padIndex;
        fxPanel.setSelectedPad(padIndex);
        processorRef.triggerPad(padIndex, velocity);
    });

    padGrid.setSampleDropCallback([this](int padIndex, const juce::String& sampleId)
    {
        loadSampleToPad(padIndex, sampleId);
    });

    // --- SampleBrowser setup ---
    addAndMakeVisible(sampleBrowser);
    sampleBrowser.addListener(this);
    sampleBrowser.setSampleRegistry(&processorRef.getSampleRegistry());
    sampleBrowser.refreshFromRegistry();

    // --- FXPanel setup ---
    addAndMakeVisible(fxPanel);
    fxPanel.setAPVTS(&processorRef.getAPVTS());
    fxPanel.setSelectedPad(selectedPad);
    fxPanel.onMarkersDragged = [this](int padIndex, float startNorm, float endNorm)
    {
        processorRef.setPadSampleRange(padIndex, startNorm, endNorm);
    };

    // --- Master volume fader ---
    masterFader.setSliderStyle(juce::Slider::LinearVertical);
    masterFader.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    masterFader.setRange(0.0, 1.0, 0.001);
    masterFader.setValue(0.8);
    masterFader.setColour(juce::Slider::trackColourId, juce::Colour(VoidLookAndFeel::kGhost));
    masterFader.setColour(juce::Slider::thumbColourId, juce::Colour(VoidLookAndFeel::kTextPrimary));
    masterFader.setColour(juce::Slider::backgroundColourId, juce::Colour(VoidLookAndFeel::kSurface));
    addAndMakeVisible(masterFader);

    masterLabel.setText("MST", juce::dontSendNotification);
    masterLabel.setFont(VoidLookAndFeel::getMonoFontBold(9.0f));
    masterLabel.setColour(juce::Label::textColourId, juce::Colour(VoidLookAndFeel::kAccentCyan));
    masterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterLabel);

    masterVolumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getAPVTS(), "master_volume", masterFader);

    // Default window size
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
    g.fillAll(VoidLookAndFeel::voidBlack());

    // --- Title bar ---
    auto titleBar = getLocalBounds().removeFromTop(kTitleBarHeight).toFloat();

    g.setColour(VoidLookAndFeel::surface());
    g.fillRect(titleBar);

    g.setColour(VoidLookAndFeel::ghost());
    g.fillRect(titleBar.getX(), titleBar.getBottom() - 1.0f, titleBar.getWidth(), 1.0f);

    g.setFont(VoidLookAndFeel::getMonoFontBold(18.0f));
    g.setColour(VoidLookAndFeel::textPrimary());
    g.drawText(juce::String(juce::CharPointer_UTF8("\xCE\x98")) + "  V" + juce::String(juce::CharPointer_UTF8("\xC3\x98")) + "ID DRUM ENGINE",
               titleBar.reduced(12.0f, 0.0f),
               juce::Justification::centredLeft, true);

    g.setFont(VoidLookAndFeel::getMonoFont(11.0f));
    g.setColour(VoidLookAndFeel::textDim());
    g.drawText("v0.1.0", titleBar.reduced(12.0f, 0.0f),
               juce::Justification::centredRight, true);
}

// =========================================================================
// Layout
// =========================================================================

void VOIDDrumEngineEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(kTitleBarHeight);

    // Left: sample browser
    int browserW = juce::jmax(kBrowserWidth, bounds.getWidth() / 5);
    sampleBrowser.setBounds(bounds.removeFromLeft(browserW));

    // Right: FX panel
    int fxW = juce::jmax(kFXPanelWidth, bounds.getWidth() / 4);
    fxPanel.setBounds(bounds.removeFromRight(fxW));

    // Centre: DrumKitView (top 55%) and PadGrid (bottom 45%)
    // Master fader sits in a narrow strip to the right of the pad grid
    auto centreArea = bounds;
    int kitHeight = static_cast<int>(centreArea.getHeight() * 0.55f);
    drumKitView.setBounds(centreArea.removeFromTop(kitHeight).reduced(4));

    // Master fader strip on the right of the pad grid area
    auto gridArea = centreArea;
    auto masterArea = gridArea.removeFromRight(32);
    masterLabel.setBounds(masterArea.removeFromBottom(14));
    masterFader.setBounds(masterArea.reduced(4, 4));

    padGrid.setBounds(gridArea.reduced(4));
}

// =========================================================================
// SampleBrowserListener callbacks
// =========================================================================

void VOIDDrumEngineEditor::samplePreviewRequested(const juce::String& sampleId)
{
    juce::ignoreUnused(sampleId);
    DBG("Preview requested: " + sampleId);
}

void VOIDDrumEngineEditor::sampleAssignRequested(const juce::String& sampleId)
{
    // Double-click in browser: load sample to currently selected pad
    loadSampleToPad(padGrid.getSelectedPad(), sampleId);
}

void VOIDDrumEngineEditor::loadSampleToPad(int padIndex, const juce::String& sampleId)
{
    editorLog("loadSampleToPad: pad=" + juce::String(padIndex) + " sampleId=" + sampleId);

    // Strip the "voidsample:" prefix if present
    auto cleanId = sampleId.startsWith("voidsample:") ? sampleId.fromFirstOccurrenceOf("voidsample:", false, false) : sampleId;

    editorLog("loadSampleToPad: cleanId=" + cleanId);

    // Look up the sample in the registry to get the absolute path
    auto* entry = processorRef.getSampleRegistry().findSample(cleanId);
    if (entry != nullptr)
    {
        editorLog("loadSampleToPad: found entry, path=" + entry->absolutePath);
        processorRef.loadSampleForPad(padIndex, entry->absolutePath, cleanId);
        padGrid.setPadInfo(padIndex, entry->displayName, entry->category, -1);

        // Update FX panel if this is the selected pad
        if (padIndex == padGrid.getSelectedPad())
        {
            fxPanel.setSelectedPad(padIndex);
            fxPanel.setWaveformData(entry->waveformCache);
        }
    }
    else
    {
        editorLog("loadSampleToPad: SAMPLE NOT FOUND in registry for cleanId=" + cleanId);
    }
}

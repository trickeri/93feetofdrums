#include "FXPanel.h"

// =========================================================================
// VoidKnob
// =========================================================================

VoidKnob::VoidKnob(const juce::String& name, const juce::String& suffix)
    : valueSuffix(suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    slider.setRange(0.0, 1.0, 0.001);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(VoidLookAndFeel::kAccentCyan));
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(VoidLookAndFeel::kGhost));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(VoidLookAndFeel::kTextPrimary));
    addAndMakeVisible(slider);

    nameLabel.setText(name, juce::dontSendNotification);
    nameLabel.setFont(VoidLookAndFeel::getMonoFont(9.0f));
    nameLabel.setColour(juce::Label::textColourId, juce::Colour(VoidLookAndFeel::kTextDim));
    nameLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(nameLabel);
}

void VoidKnob::paint(juce::Graphics& g)
{
    // Draw the custom arc-style knob on top of the slider
    auto sliderBounds = slider.getBounds().toFloat();
    float cx = sliderBounds.getCentreX();
    float cy = sliderBounds.getCentreY();
    float radius = juce::jmin(sliderBounds.getWidth(), sliderBounds.getHeight()) * 0.4f;

    // Background arc (ghost colour)
    float startAngle = juce::MathConstants<float>::pi * 1.25f;
    float endAngle   = juce::MathConstants<float>::pi * 2.75f;

    juce::Path bgArc;
    bgArc.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour(VoidLookAndFeel::ghost());
    g.strokePath(bgArc, juce::PathStrokeType(2.0f));

    // Value arc (accent-cyan)
    float normValue = static_cast<float>(slider.getValue() - slider.getMinimum()) /
                      static_cast<float>(slider.getMaximum() - slider.getMinimum());
    float valueAngle = startAngle + normValue * (endAngle - startAngle);

    juce::Path valueArc;
    valueArc.addCentredArc(cx, cy, radius, radius, 0.0f, startAngle, valueAngle, true);
    g.setColour(VoidLookAndFeel::accentCyan());
    g.strokePath(valueArc, juce::PathStrokeType(2.5f));

    // Value readout below the knob in monospace
    auto valueBounds = sliderBounds.translated(0, sliderBounds.getHeight() * 0.3f);
    g.setFont(VoidLookAndFeel::getMonoFont(9.0f));
    g.setColour(VoidLookAndFeel::textPrimary());

    juce::String valText;
    if (valueSuffix.isNotEmpty())
        valText = juce::String(slider.getValue(), 2) + valueSuffix;
    else
        valText = juce::String(slider.getValue(), 2);

    g.drawText(valText, valueBounds.toNearestInt(), juce::Justification::centred, true);
}

void VoidKnob::resized()
{
    auto area = getLocalBounds();
    nameLabel.setBounds(area.removeFromBottom(14));
    slider.setBounds(area);
}

// =========================================================================
// FXSection
// =========================================================================

FXSection::FXSection(const juce::String& title)
    : sectionTitle(title)
{
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, juce::Colour(VoidLookAndFeel::kSurfaceRaised));
    bypassButton.setColour(juce::TextButton::textColourOffId, juce::Colour(VoidLookAndFeel::kTextDim));
    bypassButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(VoidLookAndFeel::kAccentRed));
    bypassButton.setColour(juce::TextButton::textColourOnId, juce::Colour(VoidLookAndFeel::kTextPrimary));
    addAndMakeVisible(bypassButton);
}

void FXSection::addKnob(VoidKnob* knob)
{
    knobs.add(knob);
    addAndMakeVisible(knob);
}

void FXSection::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Section background
    g.setColour(VoidLookAndFeel::surfaceRaised());
    g.fillRoundedRectangle(bounds, 2.0f);

    // Section header line
    g.setColour(VoidLookAndFeel::ghost());
    g.drawHorizontalLine(18, bounds.getX(), bounds.getRight());

    // Title
    g.setFont(VoidLookAndFeel::getMonoFontBold(10.0f));
    g.setColour(VoidLookAndFeel::accentCyan());
    g.drawText(sectionTitle, juce::Rectangle<float>(4.0f, 0.0f, bounds.getWidth() - 40.0f, 18.0f),
               juce::Justification::centredLeft, true);

    // Dimmed if bypassed
    if (bypassButton.getToggleState())
    {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds.withTrimmedTop(18.0f), 2.0f);
    }
}

void FXSection::resized()
{
    auto area = getLocalBounds();

    // Header row: title + bypass button
    auto header = area.removeFromTop(18);
    bypassButton.setBounds(header.removeFromRight(32).reduced(2));

    // Remaining space: knobs laid out horizontally
    area.removeFromTop(4); // padding
    if (knobs.isEmpty()) return;

    int knobW = area.getWidth() / knobs.size();
    for (auto* knob : knobs)
    {
        knob->setBounds(area.removeFromLeft(knobW));
    }
}

// =========================================================================
// FXPanel
// =========================================================================

juce::String FXPanel::padParamId(int padIndex, const juce::String& suffix)
{
    return "pad_" + juce::String(padIndex + 1).paddedLeft('0', 2) + "_" + suffix;
}

FXPanel::FXPanel()
{
    // Header label
    headerLabel.setFont(VoidLookAndFeel::getMonoFontBold(14.0f));
    headerLabel.setColour(juce::Label::textColourId, juce::Colour(VoidLookAndFeel::kAccentCyan));
    headerLabel.setJustificationType(juce::Justification::centredLeft);
    headerLabel.setText(juce::CharPointer_UTF8("\xce\xa3 FX  PAD 01"), juce::dontSendNotification);
    addAndMakeVisible(headerLabel);

    // Create knobs and add to sections
    cutoffKnob     = new VoidKnob("CUTOFF");
    resonanceKnob  = new VoidKnob("RESO");
    filterSection.addKnob(cutoffKnob);
    filterSection.addKnob(resonanceKnob);
    addAndMakeVisible(filterSection);

    driveKnob = new VoidKnob("DRIVE");
    driveSection.addKnob(driveKnob);
    addAndMakeVisible(driveSection);

    crushDepthKnob = new VoidKnob("DEPTH");
    crushRateKnob  = new VoidKnob("RATE");
    crushSection.addKnob(crushDepthKnob);
    crushSection.addKnob(crushRateKnob);
    addAndMakeVisible(crushSection);

    reverbSendKnob = new VoidKnob("SEND");
    reverbSection.addKnob(reverbSendKnob);
    addAndMakeVisible(reverbSection);

    // Waveform display
    waveformDisplay.setMarkersEnabled(true);
    waveformDisplay.setDrawGrid(true);
    waveformDisplay.setFilled(true);
    waveformDisplay.onMarkersChanged = [this](float startNorm, float endNorm)
    {
        if (onMarkersDragged)
            onMarkersDragged(currentPad, startNorm, endNorm);
    };
    addAndMakeVisible(waveformDisplay);
}

void FXPanel::setAPVTS(juce::AudioProcessorValueTreeState* apvts)
{
    apvtsPtr = apvts;
    rebuildAttachments();
}

void FXPanel::setSelectedPad(int padIndex)
{
    if (padIndex == currentPad && apvtsPtr != nullptr)
        return;

    currentPad = juce::jlimit(0, void_drum::NUM_PADS - 1, padIndex);

    // Update header
    juce::String padNum = juce::String(currentPad + 1).paddedLeft('0', 2);
    headerLabel.setText(juce::String(juce::CharPointer_UTF8("\xce\xa3")) + " FX  PAD " + padNum,
                        juce::dontSendNotification);

    rebuildAttachments();
}

void FXPanel::setWaveformData(const std::array<float, 128>& peaks)
{
    waveformDisplay.setWaveformData(peaks);
}

void FXPanel::setMarkerPositions(float startNorm, float endNorm)
{
    waveformDisplay.setMarkerPositions(startNorm, endNorm);
}

void FXPanel::rebuildAttachments()
{
    // Destroy old attachments first (order matters — detach before re-attach)
    attachments.cutoff.reset();
    attachments.resonance.reset();
    attachments.drive.reset();
    attachments.crushDepth.reset();
    attachments.crushRate.reset();
    attachments.reverbSend.reset();

    if (apvtsPtr == nullptr) return;

    attachments.cutoff = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        *apvtsPtr, padParamId(currentPad, "filter_cutoff"), cutoffKnob->getSlider());

    attachments.resonance = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        *apvtsPtr, padParamId(currentPad, "filter_resonance"), resonanceKnob->getSlider());

    attachments.drive = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        *apvtsPtr, padParamId(currentPad, "drive"), driveKnob->getSlider());

    attachments.crushDepth = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        *apvtsPtr, padParamId(currentPad, "bitcrush_depth"), crushDepthKnob->getSlider());

    attachments.crushRate = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        *apvtsPtr, padParamId(currentPad, "bitcrush_rate"), crushRateKnob->getSlider());

    attachments.reverbSend = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        *apvtsPtr, padParamId(currentPad, "reverb_send"), reverbSendKnob->getSlider());
}

// =========================================================================
// Paint
// =========================================================================

void FXPanel::paint(juce::Graphics& g)
{
    g.fillAll(VoidLookAndFeel::surface());

    // Border
    g.setColour(VoidLookAndFeel::ghost());
    g.drawRect(getLocalBounds(), 1);
}

// =========================================================================
// Layout
// =========================================================================

void FXPanel::resized()
{
    auto area = getLocalBounds().reduced(4);

    // Header
    headerLabel.setBounds(area.removeFromTop(24));

    // Waveform display at bottom
    int waveformH = juce::jmax(60, area.getHeight() / 5);
    waveformDisplay.setBounds(area.removeFromBottom(waveformH));

    area.removeFromBottom(4); // Spacing

    // FX sections stacked vertically, dividing remaining space
    int sectionGap = 4;
    int numSections = 4;
    int sectionH = (area.getHeight() - sectionGap * (numSections - 1)) / numSections;

    filterSection.setBounds(area.removeFromTop(sectionH));
    area.removeFromTop(sectionGap);

    driveSection.setBounds(area.removeFromTop(sectionH));
    area.removeFromTop(sectionGap);

    crushSection.setBounds(area.removeFromTop(sectionH));
    area.removeFromTop(sectionGap);

    reverbSection.setBounds(area);
}

#include "MixerStrip.h"

// =========================================================================
// Helper: build parameter ID string matching PluginProcessor convention
// =========================================================================
static juce::String padParamId(int padIndex, const juce::String& suffix)
{
    return "pad_" + juce::String(padIndex + 1).paddedLeft('0', 2) + "_" + suffix;
}

// =========================================================================
// LevelMeter
// =========================================================================

LevelMeter::LevelMeter()
{
    startTimerHz(30);
}

void LevelMeter::setLevels(float left, float right)
{
    levelL = left;
    levelR = right;

    if (left > peakL)  { peakL = left;  peakHoldCounterL = kPeakHoldFrames; }
    if (right > peakR) { peakR = right; peakHoldCounterR = kPeakHoldFrames; }
}

void LevelMeter::timerCallback()
{
    // Decay peaks
    if (peakHoldCounterL > 0)
        --peakHoldCounterL;
    else
        peakL *= kDecayRate;

    if (peakHoldCounterR > 0)
        --peakHoldCounterR;
    else
        peakR *= kDecayRate;

    repaint();
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(VoidLookAndFeel::surface());

    float halfW = bounds.getWidth() * 0.5f;
    float h = bounds.getHeight();

    auto drawChannel = [&](float x, float w, float level, float peak)
    {
        // Active level bar (bottom to top)
        float barH = level * h;
        g.setColour(VoidLookAndFeel::meterGreen());
        g.fillRect(x, h - barH, w, barH);

        // Peak hold line
        if (peak > 0.01f)
        {
            float peakY = h - peak * h;
            g.setColour(VoidLookAndFeel::accentRed());
            g.fillRect(x, peakY, w, 1.5f);
        }
    };

    drawChannel(bounds.getX(), halfW - 0.5f, levelL, peakL);
    drawChannel(bounds.getX() + halfW + 0.5f, halfW - 0.5f, levelR, peakR);

    // Centre divider
    g.setColour(VoidLookAndFeel::ghost());
    g.drawVerticalLine(static_cast<int>(bounds.getX() + halfW), 0.0f, h);
}

// =========================================================================
// ChannelStrip
// =========================================================================

ChannelStrip::ChannelStrip()
{
    // Volume fader — vertical
    volumeFader.setSliderStyle(juce::Slider::LinearVertical);
    volumeFader.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volumeFader.setRange(0.0, 1.0, 0.001);
    volumeFader.setValue(0.8);
    volumeFader.setColour(juce::Slider::trackColourId, juce::Colour(VoidLookAndFeel::kGhost));
    volumeFader.setColour(juce::Slider::thumbColourId, juce::Colour(VoidLookAndFeel::kTextPrimary));
    volumeFader.setColour(juce::Slider::backgroundColourId, juce::Colour(VoidLookAndFeel::kSurface));
    addAndMakeVisible(volumeFader);

    // Pan knob — small rotary
    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    panKnob.setRange(-1.0, 1.0, 0.01);
    panKnob.setValue(0.0);
    addAndMakeVisible(panKnob);

    // Mute button
    muteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(VoidLookAndFeel::kSurfaceRaised));
    muteButton.setColour(juce::TextButton::textColourOffId, juce::Colour(VoidLookAndFeel::kTextDim));
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(VoidLookAndFeel::kAccentRed));
    muteButton.setColour(juce::TextButton::textColourOnId, juce::Colour(VoidLookAndFeel::kTextPrimary));
    muteButton.setClickingTogglesState(true);
    muteButton.onClick = [this]() { muted = muteButton.getToggleState(); };
    addAndMakeVisible(muteButton);

    // Solo button
    soloButton.setColour(juce::TextButton::buttonColourId, juce::Colour(VoidLookAndFeel::kSurfaceRaised));
    soloButton.setColour(juce::TextButton::textColourOffId, juce::Colour(VoidLookAndFeel::kTextDim));
    soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(VoidLookAndFeel::kAccentCyan));
    soloButton.setColour(juce::TextButton::textColourOnId, juce::Colour(VoidLookAndFeel::kVoidBlack));
    soloButton.setClickingTogglesState(true);
    soloButton.onClick = [this]() { soloed = soloButton.getToggleState(); };
    addAndMakeVisible(soloButton);

    // Level meter
    addAndMakeVisible(meter);

    // Pad label
    padLabel.setFont(VoidLookAndFeel::getMonoFont(10.0f));
    padLabel.setColour(juce::Label::textColourId, juce::Colour(VoidLookAndFeel::kTextDim));
    padLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(padLabel);
}

void ChannelStrip::connectToParameters(juce::AudioProcessorValueTreeState& apvts, int padIndex)
{
    padIdx = padIndex;

    // Disconnect old attachments
    volumeAttachment.reset();
    panAttachment.reset();

    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, padParamId(padIndex, "volume"), volumeFader);
    panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, padParamId(padIndex, "pan"), panKnob);

    padLabel.setText(juce::String(padIndex + 1), juce::dontSendNotification);
}

void ChannelStrip::setSelected(bool selected)
{
    isSelected = selected;
    repaint();
}

void ChannelStrip::setPadLabel(const juce::String& label)
{
    padLabel.setText(label, juce::dontSendNotification);
}

void ChannelStrip::setMeterLevels(float left, float right)
{
    meter.setLevels(left, right);
}

void ChannelStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(VoidLookAndFeel::surface());

    // Selected pad highlight border
    if (isSelected)
    {
        g.setColour(VoidLookAndFeel::accentCyan());
        g.drawRect(bounds, 1.5f);
    }
    else
    {
        g.setColour(VoidLookAndFeel::ghost());
        g.drawRect(bounds, 0.5f);
    }
}

void ChannelStrip::resized()
{
    auto area = getLocalBounds().reduced(1);

    // Bottom: pad label
    padLabel.setBounds(area.removeFromBottom(14));

    // Bottom: mute + solo buttons side by side
    auto buttonRow = area.removeFromBottom(16);
    muteButton.setBounds(buttonRow.removeFromLeft(buttonRow.getWidth() / 2).reduced(1));
    soloButton.setBounds(buttonRow.reduced(1));

    // Top: pan knob
    auto panArea = area.removeFromTop(juce::jmin(24, area.getHeight() / 4));
    int knobSize = juce::jmin(panArea.getWidth(), panArea.getHeight());
    panKnob.setBounds(panArea.withSizeKeepingCentre(knobSize, knobSize));

    // Remaining: fader on left, meter on right
    auto meterW = juce::jmax(6, area.getWidth() / 4);
    meter.setBounds(area.removeFromRight(meterW).reduced(1));
    volumeFader.setBounds(area.reduced(1));
}

// =========================================================================
// MixerStrip (full panel)
// =========================================================================

MixerStrip::MixerStrip()
{
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
    {
        channels[static_cast<size_t>(i)] = std::make_unique<ChannelStrip>();
        addAndMakeVisible(*channels[static_cast<size_t>(i)]);
    }

    // Master fader
    masterFader.setSliderStyle(juce::Slider::LinearVertical);
    masterFader.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    masterFader.setRange(0.0, 1.0, 0.001);
    masterFader.setValue(0.8);
    masterFader.setColour(juce::Slider::trackColourId, juce::Colour(VoidLookAndFeel::kGhost));
    masterFader.setColour(juce::Slider::thumbColourId, juce::Colour(VoidLookAndFeel::kTextPrimary));
    masterFader.setColour(juce::Slider::backgroundColourId, juce::Colour(VoidLookAndFeel::kSurface));
    addAndMakeVisible(masterFader);

    addAndMakeVisible(masterMeter);

    masterLabel.setText("MASTER", juce::dontSendNotification);
    masterLabel.setFont(VoidLookAndFeel::getMonoFontBold(10.0f));
    masterLabel.setColour(juce::Label::textColourId, juce::Colour(VoidLookAndFeel::kAccentCyan));
    masterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterLabel);

    startTimerHz(30); // Meter refresh
}

MixerStrip::~MixerStrip()
{
    stopTimer();
}

void MixerStrip::connectToParameters(juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
        channels[static_cast<size_t>(i)]->connectToParameters(apvts, i);

    masterVolumeAttachment.reset();
    masterVolumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "master_volume", masterFader);
}

void MixerStrip::setSelectedPad(int padIndex)
{
    selectedPad = padIndex;
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
        channels[static_cast<size_t>(i)]->setSelected(i == padIndex);
}

void MixerStrip::setMeterLevels(int padIndex, float left, float right)
{
    if (padIndex >= 0 && padIndex < void_drum::NUM_PADS)
        channels[static_cast<size_t>(padIndex)]->setMeterLevels(left, right);
}

void MixerStrip::setMasterMeterLevels(float left, float right)
{
    masterMeter.setLevels(left, right);
}

void MixerStrip::timerCallback()
{
    // Meters auto-repaint via their own timers
}

void MixerStrip::paint(juce::Graphics& g)
{
    g.fillAll(VoidLookAndFeel::surface());

    // Top border line
    g.setColour(VoidLookAndFeel::ghost());
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));

    // Separator before master
    auto bounds = getLocalBounds();
    int masterW = juce::jmax(60, bounds.getWidth() / 12);
    int separatorX = bounds.getWidth() - masterW - 4;
    g.setColour(VoidLookAndFeel::ghost());
    g.drawVerticalLine(separatorX, 0.0f, static_cast<float>(getHeight()));
}

void MixerStrip::resized()
{
    auto area = getLocalBounds();

    // Master section on the right
    int masterW = juce::jmax(60, area.getWidth() / 12);
    auto masterArea = area.removeFromRight(masterW);

    // Master layout
    masterLabel.setBounds(masterArea.removeFromBottom(14));
    int meterW = juce::jmax(8, masterArea.getWidth() / 3);
    masterMeter.setBounds(masterArea.removeFromRight(meterW).reduced(2));
    masterFader.setBounds(masterArea.reduced(2));

    // Separator gap
    area.removeFromRight(4);

    // 16 channel strips evenly distributed
    int stripW = area.getWidth() / void_drum::NUM_PADS;
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
    {
        auto stripArea = area.removeFromLeft(stripW);
        channels[static_cast<size_t>(i)]->setBounds(stripArea);
    }
}

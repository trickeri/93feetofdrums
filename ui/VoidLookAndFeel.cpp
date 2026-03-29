#include "VoidLookAndFeel.h"

// =========================================================================
// Construction
// =========================================================================

VoidLookAndFeel::VoidLookAndFeel()
{
    // Global colour scheme
    setColour(juce::ResizableWindow::backgroundColourId, voidBlack());

    setColour(juce::Label::textColourId, textPrimary());

    setColour(juce::TextButton::buttonColourId, surfaceRaised());
    setColour(juce::TextButton::buttonOnColourId, surfaceHot());
    setColour(juce::TextButton::textColourOffId, textPrimary());
    setColour(juce::TextButton::textColourOnId, accentCyan());

    setColour(juce::Slider::rotarySliderFillColourId, accentCyan());
    setColour(juce::Slider::rotarySliderOutlineColourId, ghost());
    setColour(juce::Slider::thumbColourId, accentCyan());
    setColour(juce::Slider::trackColourId, ghost());
    setColour(juce::Slider::backgroundColourId, voidBlack());

    setColour(juce::ScrollBar::thumbColourId, accentCyan().withAlpha(0.5f));
    setColour(juce::ScrollBar::trackColourId, surface());

    setColour(juce::TreeView::backgroundColourId, voidBlack());
    setColour(juce::TreeView::linesColourId, ghost());
    setColour(juce::TreeView::selectedItemBackgroundColourId, surfaceHot());
}

// =========================================================================
// Rotary Slider (Knobs) -- thin arc indicator on void-black
// =========================================================================

void VoidLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y,
                                        int width, int height,
                                        float sliderPos,
                                        float rotaryStartAngle,
                                        float rotaryEndAngle,
                                        juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centreX = bounds.getCentreX();
    auto centreY = bounds.getCentreY();
    auto arcRadius = radius - 4.0f;
    auto lineThickness = 2.5f;

    // Background arc (ghost track)
    juce::Path bgArc;
    bgArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                         0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(ghost());
    g.strokePath(bgArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Value arc (accent cyan)
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Path valueArc;
    valueArc.addCentredArc(centreX, centreY, arcRadius, arcRadius,
                            0.0f, rotaryStartAngle, toAngle, true);
    g.setColour(accentCyan());
    g.strokePath(valueArc, juce::PathStrokeType(lineThickness, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    // Centre value readout
    auto valueStr = juce::String(slider.getValue(), 1);
    g.setFont(getMonoFont(radius * 0.5f));
    g.setColour(textPrimary());
    g.drawText(valueStr, bounds, juce::Justification::centred, false);
}

// =========================================================================
// Button -- sharp edges, industrial, no rounded corners
// =========================================================================

void VoidLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& /*backgroundColour*/,
                                            bool isHighlighted, bool isDown)
{
    auto bounds = button.getLocalBounds().toFloat();

    // Fill
    if (isDown)
        g.setColour(surfaceHot());
    else if (isHighlighted)
        g.setColour(surfaceRaised().brighter(0.1f));
    else if (button.getToggleState())
        g.setColour(surfaceHot());
    else
        g.setColour(surfaceRaised());

    g.fillRect(bounds);

    // Border -- sharp rectangle, no rounding
    if (button.getToggleState())
        g.setColour(accentCyan());
    else if (isHighlighted)
        g.setColour(ghost().brighter(0.2f));
    else
        g.setColour(ghost());

    g.drawRect(bounds, 1.0f);
}

void VoidLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                      bool /*isHighlighted*/, bool isDown)
{
    auto font = getMonoFont(juce::jmin(14.0f, button.getHeight() * 0.6f));
    g.setFont(font);

    auto colour = button.getToggleState() ? accentCyan() : textPrimary();
    if (isDown) colour = colour.brighter(0.2f);
    g.setColour(colour);

    g.drawText(button.getButtonText(), button.getLocalBounds().reduced(2),
               juce::Justification::centred, true);
}

// =========================================================================
// Linear Slider -- flat fader with accent colours
// =========================================================================

void VoidLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y,
                                        int width, int height,
                                        float sliderPos, float /*minSliderPos*/,
                                        float /*maxSliderPos*/,
                                        juce::Slider::SliderStyle style,
                                        juce::Slider& /*slider*/)
{
    bool isVertical = (style == juce::Slider::LinearVertical ||
                       style == juce::Slider::LinearBarVertical);

    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
                                          static_cast<float>(width), static_cast<float>(height));

    // Track background
    g.setColour(ghost());

    if (isVertical)
    {
        auto trackX = bounds.getCentreX() - 2.0f;
        g.fillRect(trackX, bounds.getY(), 4.0f, bounds.getHeight());

        // Filled portion from bottom up
        g.setColour(accentCyan());
        auto fillHeight = bounds.getBottom() - sliderPos;
        g.fillRect(trackX, sliderPos, 4.0f, fillHeight);

        // Thumb (sharp rectangle)
        g.setColour(textPrimary());
        g.fillRect(bounds.getCentreX() - 6.0f, sliderPos - 3.0f, 12.0f, 6.0f);
    }
    else
    {
        auto trackY = bounds.getCentreY() - 2.0f;
        g.fillRect(bounds.getX(), trackY, bounds.getWidth(), 4.0f);

        // Filled portion from left
        g.setColour(accentCyan());
        g.fillRect(bounds.getX(), trackY, sliderPos - bounds.getX(), 4.0f);

        // Thumb (sharp rectangle)
        g.setColour(textPrimary());
        g.fillRect(sliderPos - 3.0f, bounds.getCentreY() - 6.0f, 6.0f, 12.0f);
    }
}

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// =========================================================================
// VoidLookAndFeel
//
// Custom JUCE LookAndFeel implementing the 93feetofsmoke aesthetic:
// void-black backgrounds, sharp geometric edges, accent-cyan / magenta
// highlights, monospace typography, industrial feel.
// =========================================================================
class VoidLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // -- Colour palette (static constants) ----------------------------------
    static constexpr juce::uint32 kVoidBlack     = 0xFF000000;
    static constexpr juce::uint32 kSurface       = 0xFF0D0D0D;
    static constexpr juce::uint32 kSurfaceRaised = 0xFF1A1A1A;
    static constexpr juce::uint32 kSurfaceHot    = 0xFF252525;
    static constexpr juce::uint32 kTextPrimary   = 0xFFF0F0F0;
    static constexpr juce::uint32 kTextDim       = 0xFF666666;
    static constexpr juce::uint32 kAccentCyan    = 0xFF00F0FF;
    static constexpr juce::uint32 kAccentMagenta = 0xFFFF00AA;
    static constexpr juce::uint32 kAccentRed     = 0xFFFF2244;
    static constexpr juce::uint32 kHitFlash      = 0xFFFFFFFF;
    static constexpr juce::uint32 kMeterGreen    = 0xFF00FF66;
    static constexpr juce::uint32 kGhost         = 0xFF333333;

    // Helper to get juce::Colour from the palette
    static juce::Colour voidBlack()     { return juce::Colour(kVoidBlack); }
    static juce::Colour surface()       { return juce::Colour(kSurface); }
    static juce::Colour surfaceRaised() { return juce::Colour(kSurfaceRaised); }
    static juce::Colour surfaceHot()    { return juce::Colour(kSurfaceHot); }
    static juce::Colour textPrimary()   { return juce::Colour(kTextPrimary); }
    static juce::Colour textDim()       { return juce::Colour(kTextDim); }
    static juce::Colour accentCyan()    { return juce::Colour(kAccentCyan); }
    static juce::Colour accentMagenta() { return juce::Colour(kAccentMagenta); }
    static juce::Colour accentRed()     { return juce::Colour(kAccentRed); }
    static juce::Colour hitFlash()      { return juce::Colour(kHitFlash); }
    static juce::Colour meterGreen()    { return juce::Colour(kMeterGreen); }
    static juce::Colour ghost()         { return juce::Colour(kGhost); }

    // -- Monospace font helper ----------------------------------------------
    static juce::Font getMonoFont(float height)
    {
        return juce::Font(juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain);
    }

    static juce::Font getMonoFontBold(float height)
    {
        return juce::Font(juce::Font::getDefaultMonospacedFontName(), height, juce::Font::bold);
    }

    VoidLookAndFeel();
    ~VoidLookAndFeel() override = default;

    // -- Custom drawing overrides -------------------------------------------
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidLookAndFeel)
};

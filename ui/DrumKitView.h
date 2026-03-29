#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../engine/interfaces/IPadState.h"
#include "VoidLookAndFeel.h"
#include <array>
#include <functional>

// =========================================================================
// DrumKitView
//
// Interactive top-down overhead drum kit rendered as geometric / blueprint
// wireframe shapes. Each kit piece is clickable and triggers the
// corresponding pad. Hit animations include flash, glow, cymbal shimmer,
// and kick pulse effects.
//
// Kit layout (8 visible pieces mapped to pads 0-7):
//   Pad 0: Kick     (centre-bottom)  -- Greek: Omega
//   Pad 1: Snare    (front-centre)   -- Greek: Sigma
//   Pad 2: Hi-Hat   (left)           -- Greek: Theta
//   Pad 3: Tom 1    (high tom)       -- Greek: Delta
//   Pad 4: Tom 2    (mid tom)        -- Greek: Phi
//   Pad 5: Tom 3    (floor tom)      -- Greek: Psi
//   Pad 6: Ride     (right)          -- Greek: Pi
//   Pad 7: Crash    (left-top)       -- Greek: Lambda
// =========================================================================
class DrumKitView : public juce::Component,
                    private juce::Timer
{
public:
    /** Number of visible kit pieces. */
    static constexpr int NUM_KIT_PIECES = 8;

    /** Callback signature: pad index (0..15), velocity (0.0..1.0). */
    using PadTriggerCallback = std::function<void(int padIndex, float velocity)>;

    DrumKitView();
    ~DrumKitView() override = default;

    /** Set the callback invoked when a kit piece is clicked. */
    void setPadTriggerCallback(PadTriggerCallback cb) { onPadTrigger = std::move(cb); }

    /** Externally trigger a hit animation on the given pad (e.g. from MIDI input). */
    void triggerHit(int padIndex, float velocity);

    // -- Component overrides ------------------------------------------------
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    // -- Timer (animation driver) -------------------------------------------
    void timerCallback() override;

    // -- Kit piece definition -----------------------------------------------
    enum class PieceType { Kick, Snare, HiHat, Tom, Cymbal };

    struct KitPiece
    {
        int          padIndex;
        PieceType    type;
        const char*  greekLetter;   // UTF-8 Greek glyph
        const char*  label;         // Short label

        // Layout: proportional position and size (0.0 - 1.0 of component bounds)
        float propX, propY;         // centre position
        float propW, propH;         // bounding size

        // Animation state
        float hitBrightness = 0.0f;          // 0..1 flash intensity
        float cymbalWobble  = 0.0f;          // radians of wobble
        float kickRipple    = 0.0f;          // 0..1 ripple expansion
        bool  cymbalWobbleDir = false;

        juce::Rectangle<float> getBounds(float parentW, float parentH) const
        {
            float cx = propX * parentW;
            float cy = propY * parentH;
            float w  = propW * parentW;
            float h  = propH * parentH;
            return { cx - w * 0.5f, cy - h * 0.5f, w, h };
        }
    };

    std::array<KitPiece, NUM_KIT_PIECES> pieces;

    PadTriggerCallback onPadTrigger;
    bool animating = false;

    // -- Drawing helpers ----------------------------------------------------
    void drawBackground(juce::Graphics& g, juce::Rectangle<float> area);
    void drawScanLines(juce::Graphics& g, juce::Rectangle<float> area);
    void drawCRTVignette(juce::Graphics& g, juce::Rectangle<float> area);
    void drawPiece(juce::Graphics& g, const KitPiece& piece, float parentW, float parentH);
    void drawKickDrum(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece);
    void drawSnare(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece);
    void drawHiHat(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece);
    void drawTom(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece);
    void drawCymbal(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece);
    void drawGreekLabel(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece);
    void drawPadConnectors(juce::Graphics& g, float parentW, float parentH);

    int hitTestPiece(juce::Point<float> pos) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumKitView)
};

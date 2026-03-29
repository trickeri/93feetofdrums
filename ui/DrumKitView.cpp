#include "DrumKitView.h"

using VLF = VoidLookAndFeel;

// =========================================================================
// Construction
// =========================================================================

DrumKitView::DrumKitView()
{
    // Define kit pieces with proportional positions within the component.
    // Layout is a top-down overhead drum kit view.
    //                      padIdx  type                 greek   label    propX  propY  propW   propH
    pieces[0] = KitPiece{  0, PieceType::Kick,   "\xCE\xA9", "KICK",  0.50f, 0.78f, 0.22f, 0.22f };
    pieces[1] = KitPiece{  1, PieceType::Snare,  "\xCE\xA3", "SNARE", 0.38f, 0.60f, 0.13f, 0.13f };
    pieces[2] = KitPiece{  2, PieceType::HiHat,  "\xCE\x98", "HAT",   0.18f, 0.52f, 0.10f, 0.10f };
    pieces[3] = KitPiece{  3, PieceType::Tom,    "\xCE\x94", "TOM1",  0.38f, 0.35f, 0.12f, 0.10f };
    pieces[4] = KitPiece{  4, PieceType::Tom,    "\xCE\xA6", "TOM2",  0.53f, 0.32f, 0.12f, 0.10f };
    pieces[5] = KitPiece{  5, PieceType::Tom,    "\xCE\xA8", "TOM3",  0.72f, 0.58f, 0.14f, 0.14f };
    pieces[6] = KitPiece{  6, PieceType::Cymbal, "\xCE\xA0", "RIDE",  0.78f, 0.35f, 0.15f, 0.09f };
    pieces[7] = KitPiece{  7, PieceType::Cymbal, "\xCE\x9B", "CRASH", 0.22f, 0.25f, 0.14f, 0.08f };
}

// =========================================================================
// Trigger Hit (external or internal)
// =========================================================================

void DrumKitView::triggerHit(int padIndex, float velocity)
{
    for (auto& piece : pieces)
    {
        if (piece.padIndex == padIndex)
        {
            piece.hitBrightness = juce::jlimit(0.3f, 1.0f, velocity);

            if (piece.type == PieceType::Cymbal)
                piece.cymbalWobble = 0.3f;

            if (piece.type == PieceType::Kick)
                piece.kickRipple = 0.01f; // start ripple

            if (!animating)
            {
                animating = true;
                startTimerHz(60);
            }
            break;
        }
    }
    repaint();
}

// =========================================================================
// Mouse handling
// =========================================================================

void DrumKitView::mouseDown(const juce::MouseEvent& event)
{
    auto pos = event.position;
    int hit = hitTestPiece(pos);
    if (hit >= 0)
    {
        // Velocity based on distance from center of piece (closer = harder)
        auto& piece = pieces[static_cast<size_t>(hit)];
        auto bounds = piece.getBounds(static_cast<float>(getWidth()), static_cast<float>(getHeight()));
        auto centre = bounds.getCentre();
        float dist = pos.getDistanceFrom(centre);
        float maxDist = juce::jmax(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        float velocity = juce::jlimit(0.3f, 1.0f, 1.0f - (dist / maxDist) * 0.5f);

        triggerHit(piece.padIndex, velocity);

        if (onPadTrigger)
            onPadTrigger(piece.padIndex, velocity);
    }
}

int DrumKitView::hitTestPiece(juce::Point<float> pos) const
{
    float w = static_cast<float>(getWidth());
    float h = static_cast<float>(getHeight());

    // Test in reverse order (topmost drawn last = first hit)
    for (int i = NUM_KIT_PIECES - 1; i >= 0; --i)
    {
        auto bounds = pieces[static_cast<size_t>(i)].getBounds(w, h);
        // Use elliptical hit test for round pieces
        auto centre = bounds.getCentre();
        float rx = bounds.getWidth() * 0.5f;
        float ry = bounds.getHeight() * 0.5f;
        float dx = (pos.x - centre.x) / rx;
        float dy = (pos.y - centre.y) / ry;
        if (dx * dx + dy * dy <= 1.0f)
            return i;
    }
    return -1;
}

// =========================================================================
// Timer -- animation driver
// =========================================================================

void DrumKitView::timerCallback()
{
    bool anyActive = false;
    constexpr float decayRate = 0.08f;
    constexpr float wobbleDecay = 0.06f;
    constexpr float rippleSpeed = 0.04f;

    for (auto& piece : pieces)
    {
        // Hit flash decay
        if (piece.hitBrightness > 0.0f)
        {
            piece.hitBrightness -= decayRate;
            if (piece.hitBrightness < 0.0f) piece.hitBrightness = 0.0f;
            else anyActive = true;
        }

        // Cymbal wobble decay
        if (piece.cymbalWobble > 0.001f)
        {
            piece.cymbalWobble *= (1.0f - wobbleDecay);
            piece.cymbalWobbleDir = !piece.cymbalWobbleDir;
            if (piece.cymbalWobble < 0.001f) piece.cymbalWobble = 0.0f;
            else anyActive = true;
        }

        // Kick ripple expansion
        if (piece.kickRipple > 0.0f && piece.kickRipple < 1.0f)
        {
            piece.kickRipple += rippleSpeed;
            if (piece.kickRipple >= 1.0f) piece.kickRipple = 0.0f;
            else anyActive = true;
        }
    }

    repaint();

    if (!anyActive)
    {
        stopTimer();
        animating = false;
    }
}

// =========================================================================
// Paint
// =========================================================================

void DrumKitView::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    float w = area.getWidth();
    float h = area.getHeight();

    drawBackground(g, area);
    drawPadConnectors(g, w, h);

    for (auto& piece : pieces)
        drawPiece(g, piece, w, h);

    drawScanLines(g, area);
    drawCRTVignette(g, area);
}

void DrumKitView::resized()
{
    // All layout is proportional -- nothing to do here.
}

// =========================================================================
// Background: dot grid on void-black
// =========================================================================

void DrumKitView::drawBackground(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(VLF::voidBlack());
    g.fillRect(area);

    // Faint dot grid
    g.setColour(VLF::ghost().withAlpha(0.15f));
    float spacing = 20.0f;
    float dotSize = 1.5f;
    for (float yy = area.getY(); yy < area.getBottom(); yy += spacing)
        for (float xx = area.getX(); xx < area.getRight(); xx += spacing)
            g.fillEllipse(xx, yy, dotSize, dotSize);
}

// =========================================================================
// Scan-line overlay: horizontal lines at 2px intervals, 5% opacity
// =========================================================================

void DrumKitView::drawScanLines(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(juce::Colours::black.withAlpha(0.05f));
    for (float yy = area.getY(); yy < area.getBottom(); yy += 2.0f)
        g.fillRect(area.getX(), yy, area.getWidth(), 1.0f);
}

// =========================================================================
// CRT curvature vignette: darkening gradient at edges
// =========================================================================

void DrumKitView::drawCRTVignette(juce::Graphics& g, juce::Rectangle<float> area)
{
    auto cx = area.getCentreX();
    auto cy = area.getCentreY();
    float radius = juce::jmax(area.getWidth(), area.getHeight()) * 0.7f;

    juce::ColourGradient vignette(juce::Colours::transparentBlack, cx, cy,
                                   juce::Colours::black.withAlpha(0.6f), cx + radius, cy, true);
    vignette.addColour(0.6, juce::Colours::transparentBlack);
    g.setGradientFill(vignette);
    g.fillRect(area);
}

// =========================================================================
// Pad connectors: thin ghost lines from each piece to its pad number label
// =========================================================================

void DrumKitView::drawPadConnectors(juce::Graphics& g, float parentW, float parentH)
{
    g.setColour(VLF::ghost().withAlpha(0.3f));
    float bottomY = parentH * 0.95f;

    for (int i = 0; i < NUM_KIT_PIECES; ++i)
    {
        auto& piece = pieces[static_cast<size_t>(i)];
        auto bounds = piece.getBounds(parentW, parentH);
        auto centre = bounds.getCentre();

        // Pad number label along the bottom
        float labelX = (static_cast<float>(i) + 0.5f) / static_cast<float>(NUM_KIT_PIECES) * parentW;
        g.drawLine(centre.x, bounds.getBottom(), labelX, bottomY, 0.5f);

        // Pad number text
        g.setFont(VLF::getMonoFont(10.0f));
        g.setColour(VLF::textDim());
        g.drawText(juce::String(piece.padIndex + 1),
                   juce::Rectangle<float>(labelX - 10.0f, bottomY - 12.0f, 20.0f, 12.0f),
                   juce::Justification::centred, false);
        g.setColour(VLF::ghost().withAlpha(0.3f));
    }
}

// =========================================================================
// Individual piece drawing (dispatcher)
// =========================================================================

void DrumKitView::drawPiece(juce::Graphics& g, const KitPiece& piece, float parentW, float parentH)
{
    auto bounds = piece.getBounds(parentW, parentH);

    switch (piece.type)
    {
        case PieceType::Kick:   drawKickDrum(g, bounds, piece); break;
        case PieceType::Snare:  drawSnare(g, bounds, piece);    break;
        case PieceType::HiHat:  drawHiHat(g, bounds, piece);    break;
        case PieceType::Tom:    drawTom(g, bounds, piece);       break;
        case PieceType::Cymbal: drawCymbal(g, bounds, piece);    break;
    }

    drawGreekLabel(g, bounds, piece);
}

// =========================================================================
// Kick drum: large circle with bass ripple
// =========================================================================

void DrumKitView::drawKickDrum(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece)
{
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

    // Fill
    auto fillColour = VLF::surfaceRaised();
    if (piece.hitBrightness > 0.0f)
        fillColour = fillColour.interpolatedWith(VLF::hitFlash(), piece.hitBrightness * 0.4f);
    g.setColour(fillColour);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    // Outer ring
    auto strokeColour = VLF::accentCyan();
    if (piece.hitBrightness > 0.0f)
        strokeColour = strokeColour.interpolatedWith(VLF::hitFlash(), piece.hitBrightness);
    g.setColour(strokeColour);
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.0f);

    // Inner ring (beater area)
    float innerR = radius * 0.45f;
    g.setColour(VLF::ghost());
    g.drawEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, 1.0f);

    // Cross-hairs
    g.drawLine(cx - innerR * 0.7f, cy, cx + innerR * 0.7f, cy, 0.5f);
    g.drawLine(cx, cy - innerR * 0.7f, cx, cy + innerR * 0.7f, 0.5f);

    // Kick ripple animation
    if (piece.kickRipple > 0.0f)
    {
        float rippleRadius = radius * (1.0f + piece.kickRipple * 0.8f);
        float rippleAlpha = 1.0f - piece.kickRipple;
        g.setColour(VLF::accentCyan().withAlpha(rippleAlpha * 0.5f));
        g.drawEllipse(cx - rippleRadius, cy - rippleRadius,
                      rippleRadius * 2.0f, rippleRadius * 2.0f, 1.5f);

        // Second smaller ripple
        float ripple2 = juce::jmax(0.0f, piece.kickRipple - 0.2f);
        if (ripple2 > 0.0f)
        {
            float r2 = radius * (1.0f + ripple2 * 0.6f);
            g.setColour(VLF::accentCyan().withAlpha((1.0f - ripple2) * 0.3f));
            g.drawEllipse(cx - r2, cy - r2, r2 * 2.0f, r2 * 2.0f, 1.0f);
        }
    }

    // Hit glow
    if (piece.hitBrightness > 0.0f)
    {
        juce::ColourGradient glow(VLF::accentCyan().withAlpha(piece.hitBrightness * 0.3f),
                                   cx, cy,
                                   VLF::accentCyan().withAlpha(0.0f),
                                   cx + radius * 1.2f, cy, true);
        g.setGradientFill(glow);
        g.fillEllipse(cx - radius * 1.2f, cy - radius * 1.2f,
                      radius * 2.4f, radius * 2.4f);
    }
}

// =========================================================================
// Snare: smaller circle with wire lines
// =========================================================================

void DrumKitView::drawSnare(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece)
{
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

    // Fill
    auto fillColour = VLF::surfaceRaised();
    if (piece.hitBrightness > 0.0f)
        fillColour = fillColour.interpolatedWith(VLF::hitFlash(), piece.hitBrightness * 0.5f);
    g.setColour(fillColour);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    // Stroke
    auto strokeColour = VLF::accentCyan();
    if (piece.hitBrightness > 0.0f)
        strokeColour = strokeColour.interpolatedWith(VLF::hitFlash(), piece.hitBrightness);
    g.setColour(strokeColour);
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    // Snare wires (horizontal lines across)
    g.setColour(VLF::ghost().withAlpha(0.6f));
    int numWires = 5;
    for (int i = 0; i < numWires; ++i)
    {
        float t = (static_cast<float>(i) + 1.0f) / (static_cast<float>(numWires) + 1.0f);
        float wy = cy - radius + t * radius * 2.0f;
        float halfChord = std::sqrt(juce::jmax(0.0f, radius * radius - (wy - cy) * (wy - cy)));
        g.drawLine(cx - halfChord * 0.8f, wy, cx + halfChord * 0.8f, wy, 0.5f);
    }

    // Hit glow
    if (piece.hitBrightness > 0.0f)
    {
        juce::ColourGradient glow(VLF::accentCyan().withAlpha(piece.hitBrightness * 0.35f),
                                   cx, cy,
                                   VLF::accentCyan().withAlpha(0.0f),
                                   cx + radius * 1.3f, cy, true);
        g.setGradientFill(glow);
        g.fillEllipse(cx - radius * 1.3f, cy - radius * 1.3f,
                      radius * 2.6f, radius * 2.6f);
    }
}

// =========================================================================
// Hi-Hat: small circle with stand line
// =========================================================================

void DrumKitView::drawHiHat(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece)
{
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

    // Fill
    auto fillColour = VLF::surfaceRaised();
    if (piece.hitBrightness > 0.0f)
        fillColour = fillColour.interpolatedWith(VLF::hitFlash(), piece.hitBrightness * 0.5f);
    g.setColour(fillColour);
    g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

    // Stroke
    auto strokeColour = VLF::accentMagenta();
    if (piece.hitBrightness > 0.0f)
        strokeColour = strokeColour.interpolatedWith(VLF::hitFlash(), piece.hitBrightness);
    g.setColour(strokeColour);
    g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.5f);

    // Stand line going down
    g.setColour(VLF::ghost());
    g.drawLine(cx, cy + radius, cx, cy + radius * 2.5f, 1.0f);

    // Clutch dot
    g.setColour(VLF::ghost());
    g.fillEllipse(cx - 2.0f, cy - 2.0f, 4.0f, 4.0f);

    // Second hat plate hint (slightly offset)
    g.setColour(VLF::ghost().withAlpha(0.4f));
    g.drawEllipse(cx - radius * 0.9f, cy - radius * 0.9f + 2.0f,
                  radius * 1.8f, radius * 1.8f, 0.7f);

    // Hit glow
    if (piece.hitBrightness > 0.0f)
    {
        juce::ColourGradient glow(VLF::accentMagenta().withAlpha(piece.hitBrightness * 0.3f),
                                   cx, cy,
                                   VLF::accentMagenta().withAlpha(0.0f),
                                   cx + radius * 1.4f, cy, true);
        g.setGradientFill(glow);
        g.fillEllipse(cx - radius * 1.4f, cy - radius * 1.4f,
                      radius * 2.8f, radius * 2.8f);
    }
}

// =========================================================================
// Tom: oval / ellipse with rim detail
// =========================================================================

void DrumKitView::drawTom(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece)
{
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto rx = bounds.getWidth() * 0.5f;
    auto ry = bounds.getHeight() * 0.5f;

    // Fill
    auto fillColour = VLF::surfaceRaised();
    if (piece.hitBrightness > 0.0f)
        fillColour = fillColour.interpolatedWith(VLF::hitFlash(), piece.hitBrightness * 0.5f);
    g.setColour(fillColour);
    g.fillEllipse(cx - rx, cy - ry, rx * 2.0f, ry * 2.0f);

    // Stroke
    auto strokeColour = VLF::accentCyan();
    if (piece.hitBrightness > 0.0f)
        strokeColour = strokeColour.interpolatedWith(VLF::hitFlash(), piece.hitBrightness);
    g.setColour(strokeColour);
    g.drawEllipse(cx - rx, cy - ry, rx * 2.0f, ry * 2.0f, 1.5f);

    // Inner rim
    g.setColour(VLF::ghost().withAlpha(0.5f));
    g.drawEllipse(cx - rx * 0.75f, cy - ry * 0.75f, rx * 1.5f, ry * 1.5f, 0.7f);

    // Lug dots
    g.setColour(VLF::ghost().withAlpha(0.6f));
    for (int i = 0; i < 6; ++i)
    {
        float angle = static_cast<float>(i) * juce::MathConstants<float>::twoPi / 6.0f;
        float lx = cx + rx * 0.88f * std::cos(angle);
        float ly = cy + ry * 0.88f * std::sin(angle);
        g.fillEllipse(lx - 1.5f, ly - 1.5f, 3.0f, 3.0f);
    }

    // Hit glow
    if (piece.hitBrightness > 0.0f)
    {
        juce::ColourGradient glow(VLF::accentCyan().withAlpha(piece.hitBrightness * 0.3f),
                                   cx, cy,
                                   VLF::accentCyan().withAlpha(0.0f),
                                   cx + rx * 1.3f, cy, true);
        g.setGradientFill(glow);
        g.fillEllipse(cx - rx * 1.3f, cy - ry * 1.3f,
                      rx * 2.6f, ry * 2.6f);
    }
}

// =========================================================================
// Cymbal: ellipse with wobble transform and shimmer
// =========================================================================

void DrumKitView::drawCymbal(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece)
{
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto rx = bounds.getWidth() * 0.5f;
    auto ry = bounds.getHeight() * 0.5f;

    // Apply wobble transform
    float wobbleOffset = 0.0f;
    if (piece.cymbalWobble > 0.001f)
        wobbleOffset = piece.cymbalWobble * (piece.cymbalWobbleDir ? 1.0f : -1.0f) * rx * 0.15f;

    float drawCx = cx + wobbleOffset;

    // Fill
    auto fillColour = VLF::surfaceRaised().brighter(0.05f);
    if (piece.hitBrightness > 0.0f)
        fillColour = fillColour.interpolatedWith(VLF::hitFlash(), piece.hitBrightness * 0.5f);
    g.setColour(fillColour);
    g.fillEllipse(drawCx - rx, cy - ry, rx * 2.0f, ry * 2.0f);

    // Stroke
    auto strokeColour = VLF::accentCyan();
    if (piece.hitBrightness > 0.0f)
        strokeColour = strokeColour.interpolatedWith(VLF::hitFlash(), piece.hitBrightness);
    g.setColour(strokeColour);
    g.drawEllipse(drawCx - rx, cy - ry, rx * 2.0f, ry * 2.0f, 1.5f);

    // Bell (small circle at centre)
    float bellR = juce::jmin(rx, ry) * 0.25f;
    g.setColour(VLF::ghost());
    g.drawEllipse(drawCx - bellR, cy - bellR, bellR * 2.0f, bellR * 2.0f, 1.0f);

    // Radial lines (lathing grooves)
    g.setColour(VLF::ghost().withAlpha(0.3f));
    for (int i = 0; i < 8; ++i)
    {
        float angle = static_cast<float>(i) * juce::MathConstants<float>::twoPi / 8.0f;
        float x1 = drawCx + bellR * std::cos(angle);
        float y1 = cy + bellR * std::sin(angle) * (ry / rx);
        float x2 = drawCx + rx * 0.85f * std::cos(angle);
        float y2 = cy + ry * 0.85f * std::sin(angle);
        g.drawLine(x1, y1, x2, y2, 0.3f);
    }

    // Stand line
    g.setColour(VLF::ghost());
    g.drawLine(drawCx, cy + ry, drawCx, cy + ry * 3.0f, 0.8f);

    // Hit glow
    if (piece.hitBrightness > 0.0f)
    {
        juce::ColourGradient glow(VLF::accentCyan().withAlpha(piece.hitBrightness * 0.25f),
                                   drawCx, cy,
                                   VLF::accentCyan().withAlpha(0.0f),
                                   drawCx + rx * 1.5f, cy, true);
        g.setGradientFill(glow);
        g.fillEllipse(drawCx - rx * 1.5f, cy - ry * 2.0f,
                      rx * 3.0f, ry * 4.0f);
    }
}

// =========================================================================
// Greek letter label on each piece
// =========================================================================

void DrumKitView::drawGreekLabel(juce::Graphics& g, juce::Rectangle<float> bounds, const KitPiece& piece)
{
    auto centre = bounds.getCentre();
    float fontSize = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.35f;
    fontSize = juce::jmax(fontSize, 10.0f);

    // Greek letter (above centre)
    g.setFont(VLF::getMonoFontBold(fontSize));
    auto greekColour = VLF::textPrimary();
    if (piece.hitBrightness > 0.0f)
        greekColour = greekColour.interpolatedWith(VLF::accentCyan(), piece.hitBrightness);
    g.setColour(greekColour);

    auto textArea = juce::Rectangle<float>(centre.x - 20.0f, centre.y - fontSize * 0.6f, 40.0f, fontSize);
    g.drawText(juce::CharPointer_UTF8(piece.greekLetter), textArea, juce::Justification::centred, false);

    // Small label below
    g.setFont(VLF::getMonoFont(juce::jmax(8.0f, fontSize * 0.5f)));
    g.setColour(VLF::textDim());
    auto labelArea = textArea.translated(0.0f, fontSize * 0.6f);
    g.drawText(piece.label, labelArea, juce::Justification::centred, false);
}

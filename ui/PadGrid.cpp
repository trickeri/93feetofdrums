#include "PadGrid.h"

using VLF = VoidLookAndFeel;

// =========================================================================
// Construction
// =========================================================================

PadGrid::PadGrid()
{
    // Initialise default pad info
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
    {
        pads[static_cast<size_t>(i)].midiNote = kDefaultMidiNotes[i];
        pads[static_cast<size_t>(i)].category = (i == 0) ? "kicks" :
                                                 (i == 1) ? "snares" :
                                                 (i == 2 || i == 3) ? "hats" : "percs";
    }

    // Start timer for selected-pad pulse animation
    startTimerHz(30);
}

// =========================================================================
// Public API
// =========================================================================

void PadGrid::setSelectedPad(int padIndex)
{
    if (padIndex >= 0 && padIndex < void_drum::NUM_PADS)
    {
        selectedPad = padIndex;
        repaint();
    }
}

void PadGrid::setPadInfo(int padIndex, const juce::String& sampleName,
                          const juce::String& category, int midiNote)
{
    if (padIndex >= 0 && padIndex < void_drum::NUM_PADS)
    {
        auto& pad = pads[static_cast<size_t>(padIndex)];
        pad.sampleName = sampleName;
        pad.category   = category;
        pad.midiNote   = midiNote;
        repaint();
    }
}

void PadGrid::triggerHit(int padIndex, float velocity)
{
    if (padIndex >= 0 && padIndex < void_drum::NUM_PADS)
    {
        pads[static_cast<size_t>(padIndex)].hitBrightness = juce::jlimit(0.3f, 1.0f, velocity);
        repaint();
    }
}

// =========================================================================
// Timer -- animation driver for pulse + hit decay
// =========================================================================

void PadGrid::timerCallback()
{
    bool needsRepaint = false;

    // Pulse phase for selected pad glow
    pulsePhase += 0.08f;
    if (pulsePhase > juce::MathConstants<float>::twoPi)
        pulsePhase -= juce::MathConstants<float>::twoPi;
    needsRepaint = true; // pulse always animates

    // Decay hit brightness
    for (auto& pad : pads)
    {
        if (pad.hitBrightness > 0.0f)
        {
            pad.hitBrightness -= 0.07f;
            if (pad.hitBrightness < 0.0f) pad.hitBrightness = 0.0f;
            needsRepaint = true;
        }
    }

    if (needsRepaint)
        repaint();
}

// =========================================================================
// Layout helpers
// =========================================================================

juce::Rectangle<int> PadGrid::getPadBounds(int padIndex) const
{
    int col = padIndex % GRID_COLS;
    int row = padIndex / GRID_COLS;

    int totalW = getWidth();
    int totalH = getHeight();
    int padW = totalW / GRID_COLS;
    int padH = totalH / GRID_ROWS;

    return { col * padW, row * padH, padW, padH };
}

int PadGrid::hitTestPad(juce::Point<int> pos) const
{
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
    {
        if (getPadBounds(i).contains(pos))
            return i;
    }
    return -1;
}

juce::Colour PadGrid::getCategoryColour(const juce::String& category) const
{
    auto cat = category.toLowerCase();
    if (cat == "kicks" || cat == "kick")     return VLF::accentRed();
    if (cat == "snares" || cat == "snare")   return VLF::accentCyan();
    if (cat == "hats" || cat == "hat")       return VLF::accentMagenta();
    return VLF::textDim();
}

// =========================================================================
// Mouse handling
// =========================================================================

void PadGrid::mouseDown(const juce::MouseEvent& event)
{
    int hit = hitTestPad(event.getPosition());
    if (hit >= 0)
    {
        selectedPad = hit;

        // Simple velocity based on y-position within pad (top=soft, bottom=hard)
        auto padBounds = getPadBounds(hit);
        float relY = static_cast<float>(event.getPosition().y - padBounds.getY())
                   / static_cast<float>(padBounds.getHeight());
        float velocity = juce::jlimit(0.3f, 1.0f, 0.3f + relY * 0.7f);

        triggerHit(hit, velocity);

        if (onPadTrigger)
            onPadTrigger(hit, velocity);
    }
}

// =========================================================================
// Drag and Drop
// =========================================================================

bool PadGrid::isInterestedInDragSource(const SourceDetails& /*details*/)
{
    return true; // Accept any drag (sample browser will provide sample IDs)
}

void PadGrid::itemDragEnter(const SourceDetails& details)
{
    dropTargetPad = hitTestPad(details.localPosition.toInt());
    repaint();
}

void PadGrid::itemDragExit(const SourceDetails& /*details*/)
{
    dropTargetPad = -1;
    repaint();
}

void PadGrid::itemDropped(const SourceDetails& details)
{
    int targetPad = hitTestPad(details.localPosition.toInt());
    dropTargetPad = -1;

    if (targetPad >= 0 && onSampleDrop)
    {
        auto sampleId = details.description.toString();
        onSampleDrop(targetPad, sampleId);
    }

    repaint();
}

// =========================================================================
// Paint
// =========================================================================

void PadGrid::paint(juce::Graphics& g)
{
    g.fillAll(VLF::voidBlack());

    for (int i = 0; i < void_drum::NUM_PADS; ++i)
        drawPad(g, i);
}

void PadGrid::resized()
{
    // All proportional -- nothing to do
}

// =========================================================================
// Draw individual pad
// =========================================================================

void PadGrid::drawPad(juce::Graphics& g, int padIndex)
{
    auto bounds = getPadBounds(padIndex).toFloat().reduced(2.0f);
    auto& pad = pads[static_cast<size_t>(padIndex)];

    // --- Fill ---
    auto fillColour = VLF::surfaceRaised();
    if (pad.hitBrightness > 0.0f)
        fillColour = fillColour.interpolatedWith(VLF::hitFlash(), pad.hitBrightness * 0.5f);
    if (dropTargetPad == padIndex)
        fillColour = fillColour.interpolatedWith(VLF::accentCyan(), 0.15f);
    g.setColour(fillColour);
    g.fillRect(bounds);

    // --- Border ---
    auto catColour = getCategoryColour(pad.category);

    if (padIndex == selectedPad)
    {
        // Pulsing glow border for selected pad
        float pulseAlpha = 0.4f + 0.4f * std::sin(pulsePhase);
        g.setColour(catColour.withAlpha(pulseAlpha));
        g.drawRect(bounds, 2.0f);

        // Outer glow
        g.setColour(catColour.withAlpha(pulseAlpha * 0.3f));
        g.drawRect(bounds.expanded(1.0f), 1.0f);
    }
    else
    {
        g.setColour(VLF::ghost());
        g.drawRect(bounds, 1.0f);
    }

    // --- Category colour indicator bar (top edge) ---
    g.setColour(catColour.withAlpha(0.7f));
    g.fillRect(bounds.getX(), bounds.getY(), bounds.getWidth(), 2.0f);

    // --- Hit flash glow ---
    if (pad.hitBrightness > 0.0f)
    {
        g.setColour(catColour.withAlpha(pad.hitBrightness * 0.2f));
        g.fillRect(bounds);
    }

    // --- Text content ---
    float padW = bounds.getWidth();
    float padH = bounds.getHeight();
    float margin = 4.0f;

    // Greek letter + pad number (top-left)
    g.setFont(VLF::getMonoFontBold(juce::jmin(16.0f, padH * 0.28f)));
    g.setColour(catColour);
    auto greekText = juce::String(juce::CharPointer_UTF8(kGreekLetters[padIndex]))
                   + juce::String(padIndex + 1);
    g.drawText(greekText,
               bounds.reduced(margin).removeFromTop(padH * 0.3f),
               juce::Justification::topLeft, false);

    // MIDI note (top-right)
    g.setFont(VLF::getMonoFont(juce::jmin(10.0f, padH * 0.18f)));
    g.setColour(VLF::textDim());
    g.drawText(juce::String(pad.midiNote),
               bounds.reduced(margin).removeFromTop(padH * 0.3f),
               juce::Justification::topRight, false);

    // Sample name (centre, truncated)
    if (pad.sampleName.isNotEmpty())
    {
        g.setFont(VLF::getMonoFont(juce::jmin(11.0f, padH * 0.2f)));
        g.setColour(VLF::textPrimary());
        auto nameArea = bounds.reduced(margin);
        nameArea = nameArea.withTrimmedTop(padH * 0.35f).withTrimmedBottom(padH * 0.15f);

        auto displayName = pad.sampleName;
        auto availableWidth = nameArea.getWidth();
        auto font = g.getCurrentFont();
        if (font.getStringWidthFloat(displayName) > availableWidth)
        {
            while (displayName.length() > 3 && font.getStringWidthFloat(displayName + "..") > availableWidth)
                displayName = displayName.dropLastCharacters(1);
            displayName += "..";
        }
        g.drawText(displayName, nameArea, juce::Justification::centred, false);
    }
    else
    {
        // Empty pad indicator
        g.setFont(VLF::getMonoFont(juce::jmin(9.0f, padH * 0.16f)));
        g.setColour(VLF::ghost());
        auto emptyArea = bounds.reduced(margin);
        emptyArea = emptyArea.withTrimmedTop(padH * 0.35f).withTrimmedBottom(padH * 0.15f);
        g.drawText("--empty--", emptyArea, juce::Justification::centred, false);
    }

    // Category label (bottom)
    if (pad.category.isNotEmpty())
    {
        g.setFont(VLF::getMonoFont(juce::jmin(9.0f, padH * 0.15f)));
        g.setColour(catColour.withAlpha(0.5f));
        g.drawText(pad.category.toUpperCase(),
                   bounds.reduced(margin).removeFromBottom(padH * 0.2f),
                   juce::Justification::centredBottom, false);
    }
}

#include "PadGrid.h"

using VLF = VoidLookAndFeel;

// =========================================================================
// Helper: parameter ID string
// =========================================================================
static juce::String padParamId(int padIndex, const juce::String& suffix)
{
    return "pad_" + juce::String(padIndex + 1).paddedLeft('0', 2) + "_" + suffix;
}

// =========================================================================
// PadCell
// =========================================================================

PadCell::PadCell(int index)
    : padIndex(index)
{
    // Volume fader — vertical, compact
    volumeFader.setSliderStyle(juce::Slider::LinearVertical);
    volumeFader.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volumeFader.setRange(0.0, 1.0, 0.001);
    volumeFader.setValue(0.8);
    volumeFader.setColour(juce::Slider::trackColourId, juce::Colour(VLF::kGhost));
    volumeFader.setColour(juce::Slider::thumbColourId, juce::Colour(VLF::kTextPrimary));
    volumeFader.setColour(juce::Slider::backgroundColourId, juce::Colour(VLF::kSurface));
    addAndMakeVisible(volumeFader);

    // Pan knob — small rotary
    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    panKnob.setRange(-1.0, 1.0, 0.01);
    panKnob.setValue(0.0);
    addAndMakeVisible(panKnob);

    // Mute button
    muteButton.setColour(juce::TextButton::buttonColourId, juce::Colour(VLF::kSurfaceRaised));
    muteButton.setColour(juce::TextButton::textColourOffId, juce::Colour(VLF::kTextDim));
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(VLF::kAccentRed));
    muteButton.setColour(juce::TextButton::textColourOnId, juce::Colour(VLF::kTextPrimary));
    muteButton.setClickingTogglesState(true);
    addAndMakeVisible(muteButton);

    // Solo button
    soloButton.setColour(juce::TextButton::buttonColourId, juce::Colour(VLF::kSurfaceRaised));
    soloButton.setColour(juce::TextButton::textColourOffId, juce::Colour(VLF::kTextDim));
    soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(VLF::kAccentCyan));
    soloButton.setColour(juce::TextButton::textColourOnId, juce::Colour(VLF::kVoidBlack));
    soloButton.setClickingTogglesState(true);
    addAndMakeVisible(soloButton);
}

void PadCell::connectToParameters(juce::AudioProcessorValueTreeState& apvts)
{
    volumeAttachment.reset();
    panAttachment.reset();

    volumeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, padParamId(padIndex, "volume"), volumeFader);
    panAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, padParamId(padIndex, "pan"), panKnob);
}

void PadCell::setSelected(bool selected)
{
    isSelected = selected;
    repaint();
}

void PadCell::setSampleName(const juce::String& name) { sampleName = name; repaint(); }
void PadCell::setCategory(const juce::String& cat)    { category = cat; repaint(); }
void PadCell::setMidiNote(int note)                    { midiNote = note; repaint(); }

void PadCell::triggerHit(float velocity)
{
    hitBrightness = juce::jlimit(0.3f, 1.0f, velocity);
    repaint();
}

void PadCell::decayHit()
{
    if (hitBrightness > 0.0f)
    {
        hitBrightness -= 0.07f;
        if (hitBrightness < 0.0f) hitBrightness = 0.0f;
        repaint();
    }
}

juce::Colour PadCell::getCategoryColour() const
{
    auto cat = category.toLowerCase();
    if (cat == "kicks" || cat == "kick")     return VLF::accentRed();
    if (cat == "snares" || cat == "snare")   return VLF::accentCyan();
    if (cat == "hats" || cat == "hat")       return VLF::accentMagenta();
    return VLF::textDim();
}

void PadCell::paint(juce::Graphics& g)
{
    auto fullBounds = getLocalBounds().toFloat();

    // The pad display area is the left portion (controls are child components on the right)
    float controlsWidth = fullBounds.getWidth() * 0.28f;
    auto padBounds = fullBounds.withTrimmedRight(controlsWidth).reduced(2.0f);

    auto catColour = getCategoryColour();

    // --- Pad fill ---
    auto fillColour = VLF::surfaceRaised();
    if (hitBrightness > 0.0f)
        fillColour = fillColour.interpolatedWith(VLF::hitFlash(), hitBrightness * 0.5f);
    g.setColour(fillColour);
    g.fillRect(padBounds);

    // --- Pad border ---
    if (isSelected)
    {
        g.setColour(catColour.withAlpha(0.8f));
        g.drawRect(padBounds, 2.0f);
    }
    else
    {
        g.setColour(VLF::ghost());
        g.drawRect(padBounds, 1.0f);
    }

    // --- Category colour bar (top edge) ---
    g.setColour(catColour.withAlpha(0.7f));
    g.fillRect(padBounds.getX(), padBounds.getY(), padBounds.getWidth(), 2.0f);

    // --- Hit flash glow ---
    if (hitBrightness > 0.0f)
    {
        g.setColour(catColour.withAlpha(hitBrightness * 0.2f));
        g.fillRect(padBounds);
    }

    // --- Text content ---
    float margin = 4.0f;
    auto textArea = padBounds.reduced(margin);
    float padH = padBounds.getHeight();

    // Greek letter + pad number (top-left)
    g.setFont(VLF::getMonoFontBold(juce::jmin(14.0f, padH * 0.26f)));
    g.setColour(catColour);
    auto greekText = juce::String(juce::CharPointer_UTF8(kGreekLetters[padIndex]))
                   + juce::String(padIndex + 1);
    g.drawText(greekText, textArea.removeFromTop(padH * 0.28f),
               juce::Justification::topLeft, false);

    // MIDI note (top-right of pad area)
    g.setFont(VLF::getMonoFont(juce::jmin(9.0f, padH * 0.16f)));
    g.setColour(VLF::textDim());
    auto noteArea = padBounds.reduced(margin);
    g.drawText(juce::String(midiNote), noteArea.removeFromTop(padH * 0.28f),
               juce::Justification::topRight, false);

    // Sample name (centre)
    auto nameArea = padBounds.reduced(margin);
    nameArea = nameArea.withTrimmedTop(padH * 0.32f).withTrimmedBottom(padH * 0.18f);

    if (sampleName.isNotEmpty())
    {
        g.setFont(VLF::getMonoFont(juce::jmin(10.0f, padH * 0.18f)));
        g.setColour(VLF::textPrimary());
        auto displayName = sampleName;
        auto font = g.getCurrentFont();
        auto availableWidth = nameArea.getWidth();
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
        g.setFont(VLF::getMonoFont(juce::jmin(8.0f, padH * 0.14f)));
        g.setColour(VLF::ghost());
        g.drawText("--empty--", nameArea, juce::Justification::centred, false);
    }

    // Category label (bottom)
    if (category.isNotEmpty())
    {
        g.setFont(VLF::getMonoFont(juce::jmin(8.0f, padH * 0.13f)));
        g.setColour(catColour.withAlpha(0.5f));
        auto catArea = padBounds.reduced(margin).removeFromBottom(padH * 0.18f);
        g.drawText(category.toUpperCase(), catArea, juce::Justification::centredBottom, false);
    }

    // --- Controls area background ---
    auto ctrlBounds = fullBounds.withTrimmedLeft(fullBounds.getWidth() - controlsWidth).reduced(1.0f, 2.0f);
    g.setColour(VLF::surface());
    g.fillRect(ctrlBounds);
    g.setColour(VLF::ghost().withAlpha(0.3f));
    g.drawRect(ctrlBounds, 0.5f);
}

void PadCell::mouseDown(const juce::MouseEvent& e)
{
    // Only trigger if click is in the pad area (left ~72%), not the controls area
    float controlsWidth = getWidth() * 0.28f;
    float padAreaRight = getWidth() - controlsWidth;

    if (e.getPosition().x < static_cast<int>(padAreaRight))
    {
        // Forward to parent PadGrid's mouseDown by converting coordinates
        if (auto* parent = dynamic_cast<PadGrid*>(getParentComponent()))
        {
            auto parentEvent = e.getEventRelativeTo(parent);
            parent->mouseDown(parentEvent);
        }
    }
    // If click is in controls area, let it fall through to child components naturally
}

void PadCell::resized()
{
    auto fullBounds = getLocalBounds();
    float controlsWidth = fullBounds.getWidth() * 0.28f;
    auto ctrlArea = fullBounds.withTrimmedLeft(fullBounds.getWidth() - static_cast<int>(controlsWidth)).reduced(1, 2);

    // Pan knob at top — compact, just wide enough
    int knobSize = juce::jmin(ctrlArea.getWidth(), juce::jmax(18, ctrlArea.getHeight() / 6));
    auto panArea = ctrlArea.removeFromTop(knobSize);
    panKnob.setBounds(panArea.withSizeKeepingCentre(knobSize, knobSize));

    // M/S buttons at bottom — thin row
    auto btnRow = ctrlArea.removeFromBottom(juce::jmin(14, ctrlArea.getHeight() / 5));
    muteButton.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2));
    soloButton.setBounds(btnRow);

    // Volume fader fills all remaining vertical space
    volumeFader.setBounds(ctrlArea.reduced(1, 0));
}

// =========================================================================
// PadGrid
// =========================================================================

PadGrid::PadGrid()
{
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
    {
        cells[static_cast<size_t>(i)] = std::make_unique<PadCell>(i);
        cells[static_cast<size_t>(i)]->setMidiNote(kDefaultMidiNotes[i]);
        addAndMakeVisible(*cells[static_cast<size_t>(i)]);

        // Set default categories
        auto& cell = *cells[static_cast<size_t>(i)];
        if (i == 0) cell.setCategory("kicks");
        else if (i == 1) cell.setCategory("snares");
        else if (i == 2 || i == 3) cell.setCategory("hats");
        else cell.setCategory("percs");
    }

    cells[0]->setSelected(true);
    startTimerHz(30);
}

void PadGrid::connectToParameters(juce::AudioProcessorValueTreeState& apvts)
{
    for (auto& cell : cells)
        cell->connectToParameters(apvts);
}

void PadGrid::setSelectedPad(int padIndex)
{
    if (padIndex >= 0 && padIndex < void_drum::NUM_PADS)
    {
        selectedPad = padIndex;
        for (int i = 0; i < void_drum::NUM_PADS; ++i)
            cells[static_cast<size_t>(i)]->setSelected(i == padIndex);
        repaint();
    }
}

void PadGrid::setPadInfo(int padIndex, const juce::String& sampleName,
                          const juce::String& category, int midiNote)
{
    if (padIndex >= 0 && padIndex < void_drum::NUM_PADS)
    {
        auto& cell = *cells[static_cast<size_t>(padIndex)];
        cell.setSampleName(sampleName);
        cell.setCategory(category);
        cell.setMidiNote(midiNote);
    }
}

void PadGrid::triggerHit(int padIndex, float velocity)
{
    if (padIndex >= 0 && padIndex < void_drum::NUM_PADS)
        cells[static_cast<size_t>(padIndex)]->triggerHit(velocity);
}

void PadGrid::timerCallback()
{
    pulsePhase += 0.08f;
    if (pulsePhase > juce::MathConstants<float>::twoPi)
        pulsePhase -= juce::MathConstants<float>::twoPi;

    for (auto& cell : cells)
        cell->decayHit();
}

// =========================================================================
// Layout
// =========================================================================

juce::Rectangle<int> PadGrid::getCellBounds(int padIndex) const
{
    int col = padIndex % GRID_COLS;
    int row = padIndex / GRID_COLS;
    int cellW = getWidth() / GRID_COLS;
    int cellH = getHeight() / GRID_ROWS;
    return { col * cellW, row * cellH, cellW, cellH };
}

int PadGrid::hitTestPad(juce::Point<int> pos) const
{
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
    {
        if (getCellBounds(i).contains(pos))
            return i;
    }
    return -1;
}

void PadGrid::paint(juce::Graphics& g)
{
    g.fillAll(VLF::voidBlack());
}

void PadGrid::resized()
{
    for (int i = 0; i < void_drum::NUM_PADS; ++i)
        cells[static_cast<size_t>(i)]->setBounds(getCellBounds(i));
}

// =========================================================================
// Mouse handling
// =========================================================================

void PadGrid::mouseDown(const juce::MouseEvent& event)
{
    int hit = hitTestPad(event.getPosition());
    if (hit >= 0)
    {
        setSelectedPad(hit);

        auto cellBounds = getCellBounds(hit);
        // Only trigger if click is in the pad area (left ~72%), not the controls
        float padAreaRight = cellBounds.getX() + cellBounds.getWidth() * 0.72f;
        if (event.getPosition().x < static_cast<int>(padAreaRight))
        {
            float relY = static_cast<float>(event.getPosition().y - cellBounds.getY())
                       / static_cast<float>(cellBounds.getHeight());
            float velocity = juce::jlimit(0.3f, 1.0f, 0.3f + relY * 0.7f);

            triggerHit(hit, velocity);

            if (onPadTrigger)
                onPadTrigger(hit, velocity);
        }
    }
}

// =========================================================================
// Drag and Drop
// =========================================================================

bool PadGrid::isInterestedInDragSource(const SourceDetails& /*details*/)
{
    return true;
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

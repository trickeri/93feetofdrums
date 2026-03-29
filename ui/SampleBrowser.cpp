#include "SampleBrowser.h"
#include <algorithm>
#include <cmath>

// =========================================================================
// Greek character map for category decoration
// =========================================================================

juce::String SampleBrowser::getGreekForCategory(const juce::String& cat)
{
    auto lower = cat.toLowerCase();
    if (lower == "kicks")  return juce::CharPointer_UTF8("\xce\xa9"); // Omega
    if (lower == "snares") return juce::CharPointer_UTF8("\xce\xa3"); // Sigma
    if (lower == "hats")   return juce::CharPointer_UTF8("\xce\x98"); // Theta
    if (lower == "percs")  return juce::CharPointer_UTF8("\xce\xa6"); // Phi
    if (lower == "fx")     return juce::CharPointer_UTF8("\xce\x94"); // Delta
    if (lower == "toms")   return juce::CharPointer_UTF8("\xce\x9b"); // Lambda
    if (lower == "user")   return juce::CharPointer_UTF8("\xce\xa8"); // Psi
    return juce::CharPointer_UTF8("\xce\xb1"); // alpha (default)
}

// =========================================================================
// SearchBar
// =========================================================================

SampleBrowser::SearchBar::SearchBar()
{
    setMultiLine(false);
    setReturnKeyStartsNewLine(false);
    setCaretVisible(true);
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(VoidLookAndFeel::kSurfaceRaised));
    setColour(juce::TextEditor::textColourId, juce::Colour(VoidLookAndFeel::kTextPrimary));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(VoidLookAndFeel::kGhost));
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(VoidLookAndFeel::kAccentCyan));
    setFont(VoidLookAndFeel::getMonoFont(13.0f));
    setTextToShowWhenEmpty("// search samples...", juce::Colour(VoidLookAndFeel::kTextDim));
}

// =========================================================================
// BrowserList
// =========================================================================

SampleBrowser::BrowserList::BrowserList(SampleBrowser& o) : browser(o)
{
    setInterceptsMouseClicks(true, false);
}

int SampleBrowser::BrowserList::getContentHeight() const
{
    int h = 0;
    for (auto& cat : browser.filteredCategories)
    {
        h += SampleBrowser::kHeaderHeight;
        if (!cat.collapsed)
            h += static_cast<int>(cat.samples.size()) * SampleBrowser::kItemHeight;
    }
    return h;
}

std::pair<int, int> SampleBrowser::BrowserList::getItemAt(int y) const
{
    int adjustedY = y + static_cast<int>(browser.scrollOffset);
    int runningY = 0;

    for (int ci = 0; ci < static_cast<int>(browser.filteredCategories.size()); ++ci)
    {
        auto& cat = browser.filteredCategories[static_cast<size_t>(ci)];

        // Category header
        if (adjustedY >= runningY && adjustedY < runningY + SampleBrowser::kHeaderHeight)
            return { ci, -1 }; // -1 means the header itself

        runningY += SampleBrowser::kHeaderHeight;

        if (!cat.collapsed)
        {
            for (int si = 0; si < static_cast<int>(cat.samples.size()); ++si)
            {
                if (adjustedY >= runningY && adjustedY < runningY + SampleBrowser::kItemHeight)
                    return { ci, si };
                runningY += SampleBrowser::kItemHeight;
            }
        }
    }
    return { -1, -1 };
}

void SampleBrowser::BrowserList::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(VoidLookAndFeel::surface());

    auto monoFont     = VoidLookAndFeel::getMonoFont(12.0f);
    auto monoBold     = VoidLookAndFeel::getMonoFontBold(12.0f);
    auto monoSmall    = VoidLookAndFeel::getMonoFont(10.0f);

    float yPos = -browser.scrollOffset;

    for (int ci = 0; ci < static_cast<int>(browser.filteredCategories.size()); ++ci)
    {
        auto& cat = browser.filteredCategories[static_cast<size_t>(ci)];

        // -- Category header --------------------------------------------------
        if (yPos + kHeaderHeight > 0 && yPos < bounds.getHeight())
        {
            auto headerBounds = juce::Rectangle<float>(0.0f, yPos,
                                                        bounds.getWidth(), static_cast<float>(kHeaderHeight));

            // Header background
            g.setColour(VoidLookAndFeel::surfaceRaised());
            g.fillRect(headerBounds);

            // Greek prefix + category name
            g.setFont(monoBold);
            g.setColour(VoidLookAndFeel::accentCyan());
            juce::String headerText = cat.greekPrefix + "  " + cat.name.toUpperCase();

            // Sample count
            juce::String countText = "[" + juce::String(static_cast<int>(cat.samples.size())) + "]";

            // Collapse arrow
            juce::String arrow = cat.collapsed ? "> " : "v ";

            g.drawText(arrow + headerText, headerBounds.reduced(4.0f, 0.0f),
                       juce::Justification::centredLeft, true);

            g.setFont(monoSmall);
            g.setColour(VoidLookAndFeel::textDim());
            g.drawText(countText, headerBounds.reduced(4.0f, 0.0f),
                       juce::Justification::centredRight, true);

            // Bottom border
            g.setColour(VoidLookAndFeel::ghost());
            g.drawHorizontalLine(static_cast<int>(yPos + kHeaderHeight - 1),
                                 0.0f, bounds.getWidth());
        }
        yPos += kHeaderHeight;

        // -- Sample entries ---------------------------------------------------
        if (!cat.collapsed)
        {
            for (int si = 0; si < static_cast<int>(cat.samples.size()); ++si)
            {
                if (yPos + kItemHeight > 0 && yPos < bounds.getHeight())
                {
                    auto& sample = cat.samples[static_cast<size_t>(si)];
                    auto itemBounds = juce::Rectangle<float>(0.0f, yPos,
                                                              bounds.getWidth(),
                                                              static_cast<float>(kItemHeight));

                    // Selection highlight
                    bool isSelected = (ci == browser.selectedCategoryIdx &&
                                       si == browser.selectedSampleIdx);
                    if (isSelected)
                    {
                        g.setColour(VoidLookAndFeel::accentCyan().withAlpha(0.15f));
                        g.fillRect(itemBounds);
                        g.setColour(VoidLookAndFeel::accentCyan());
                        g.drawRect(itemBounds, 1.0f);
                    }

                    // Inline waveform thumbnail
                    float waveX = itemBounds.getX() + 8.0f;
                    float waveY = itemBounds.getY() + 2.0f;
                    float waveW = static_cast<float>(kWaveformWidth);
                    float waveH = itemBounds.getHeight() - 4.0f;

                    // Draw mini waveform
                    g.setColour(VoidLookAndFeel::accentCyan().withAlpha(0.5f));
                    float waveCY = waveY + waveH * 0.5f;
                    float waveHalfH = waveH * 0.4f;
                    for (int pi = 0; pi < 128; ++pi)
                    {
                        float px = waveX + (static_cast<float>(pi) / 127.0f) * waveW;
                        float peak = sample.waveformCache[static_cast<size_t>(pi)];
                        float lineH = peak * waveHalfH;
                        if (lineH < 0.5f) lineH = 0.5f;
                        g.drawVerticalLine(static_cast<int>(px),
                                           waveCY - lineH, waveCY + lineH);
                    }

                    // File name
                    float textX = waveX + waveW + 6.0f;
                    float textW = itemBounds.getWidth() - textX - 60.0f;
                    g.setFont(monoFont);
                    g.setColour(VoidLookAndFeel::textPrimary());
                    g.drawText(sample.displayName,
                               juce::Rectangle<float>(textX, yPos, textW,
                                                       static_cast<float>(kItemHeight)),
                               juce::Justification::centredLeft, true);

                    // Duration in ms
                    g.setFont(monoSmall);
                    g.setColour(VoidLookAndFeel::textDim());
                    juce::String durText = juce::String(static_cast<int>(sample.durationMs)) + "ms";
                    g.drawText(durText,
                               juce::Rectangle<float>(itemBounds.getRight() - 54.0f, yPos,
                                                       50.0f, static_cast<float>(kItemHeight)),
                               juce::Justification::centredRight, true);
                }
                yPos += kItemHeight;
            }
        }
    }
}

void SampleBrowser::BrowserList::mouseDown(const juce::MouseEvent& e)
{
    auto [ci, si] = getItemAt(e.y);

    if (ci < 0) return;

    if (si == -1)
    {
        // Clicked on a category header — toggle collapse
        browser.filteredCategories[static_cast<size_t>(ci)].collapsed =
            !browser.filteredCategories[static_cast<size_t>(ci)].collapsed;
        browser.clampScroll();
        repaint();
        return;
    }

    // Select this sample
    browser.selectedCategoryIdx = ci;
    browser.selectedSampleIdx  = si;
    repaint();

    // Single-click preview
    auto& sample = browser.filteredCategories[static_cast<size_t>(ci)]
                       .samples[static_cast<size_t>(si)];
    browser.notifyPreview(sample.id);
}

void SampleBrowser::BrowserList::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto [ci, si] = getItemAt(e.y);
    if (ci < 0 || si < 0) return;

    auto& sample = browser.filteredCategories[static_cast<size_t>(ci)]
                       .samples[static_cast<size_t>(si)];
    browser.notifyAssign(sample.id);
}

void SampleBrowser::BrowserList::mouseDrag(const juce::MouseEvent& e)
{
    if (e.getDistanceFromDragStart() < 5) return;

    if (browser.selectedCategoryIdx >= 0 && browser.selectedSampleIdx >= 0)
    {
        auto& sample = browser.filteredCategories[static_cast<size_t>(browser.selectedCategoryIdx)]
                           .samples[static_cast<size_t>(browser.selectedSampleIdx)];

        // Use the sample ID as the drag description so the pad grid can identify it.
        auto desc = browser.getDragDescriptionForSample(sample.id);
        if (auto* dndContainer = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            dndContainer->startDragging(desc, this, juce::ScaledImage(), true);
        }
    }
}

// =========================================================================
// CustomScrollBar
// =========================================================================

SampleBrowser::CustomScrollBar::CustomScrollBar(SampleBrowser& o) : browser(o) {}

void SampleBrowser::CustomScrollBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.fillAll(VoidLookAndFeel::surface());

    int contentH = browser.browserList.getContentHeight();
    float visibleH = static_cast<float>(browser.browserList.getHeight());

    if (contentH <= static_cast<int>(visibleH))
        return; // No scrollbar needed

    float thumbRatio = visibleH / static_cast<float>(contentH);
    float thumbH = juce::jmax(20.0f, bounds.getHeight() * thumbRatio);
    float scrollRange = static_cast<float>(contentH) - visibleH;
    float thumbY = (browser.scrollOffset / scrollRange) * (bounds.getHeight() - thumbH);

    g.setColour(VoidLookAndFeel::accentCyan().withAlpha(0.6f));
    g.fillRoundedRectangle(bounds.getX() + 1.0f, thumbY, bounds.getWidth() - 2.0f, thumbH, 2.0f);
}

void SampleBrowser::CustomScrollBar::mouseDown(const juce::MouseEvent& e)
{
    dragging = true;
    dragStartY = static_cast<float>(e.y);
    dragStartOffset = browser.scrollOffset;
}

void SampleBrowser::CustomScrollBar::mouseDrag(const juce::MouseEvent& e)
{
    if (!dragging) return;

    int contentH = browser.browserList.getContentHeight();
    float visibleH = static_cast<float>(browser.browserList.getHeight());
    if (contentH <= static_cast<int>(visibleH)) return;

    float scrollRange = static_cast<float>(contentH) - visibleH;
    float barH = static_cast<float>(getHeight());
    float delta = static_cast<float>(e.y) - dragStartY;
    float scrollDelta = (delta / barH) * scrollRange;

    browser.scrollOffset = juce::jlimit(0.0f, scrollRange, dragStartOffset + scrollDelta);
    browser.browserList.repaint();
    repaint();
}

void SampleBrowser::CustomScrollBar::mouseWheelMove(const juce::MouseEvent& e,
                                                     const juce::MouseWheelDetails& wheel)
{
    browser.mouseWheelMove(e, wheel);
}

// =========================================================================
// SampleBrowser — construction
// =========================================================================

SampleBrowser::SampleBrowser()
    : browserList(*this),
      scrollBar(*this)
{
    addAndMakeVisible(searchBar);
    addAndMakeVisible(browserList);
    addAndMakeVisible(scrollBar);
    addAndMakeVisible(refreshButton);

    refreshButton.setButtonText("REFRESH");
    refreshButton.setColour(juce::TextButton::buttonColourId, juce::Colour(VoidLookAndFeel::kSurfaceRaised));
    refreshButton.setColour(juce::TextButton::textColourOffId, juce::Colour(VoidLookAndFeel::kAccentCyan));
    refreshButton.onClick = [this]() { onRefreshClicked(); };

    searchBar.onTextChange = [this]() { onSearchTextChanged(); };
}

SampleBrowser::~SampleBrowser()
{
    if (auto* reg = dynamic_cast<void_drum::SampleRegistry*>(registry))
        reg->removeListener(this);
    stopTimer();
}

// =========================================================================
// Registry connection
// =========================================================================

void SampleBrowser::setSampleRegistry(void_drum::ISampleRegistry* reg)
{
    // Unregister from old registry
    if (auto* oldReg = dynamic_cast<void_drum::SampleRegistry*>(registry))
        oldReg->removeListener(this);

    registry = reg;

    // Register with new registry for auto-refresh
    if (auto* newReg = dynamic_cast<void_drum::SampleRegistry*>(registry))
        newReg->addListener(this);

    refreshFromRegistry();
}

void SampleBrowser::sampleRegistryUpdated()
{
    // Called on message thread when the scanner rebuilds the registry
    refreshFromRegistry();
}

void SampleBrowser::refreshFromRegistry()
{
    categories.clear();

    if (registry != nullptr)
    {
        auto cats = registry->getCategories();
        for (auto& catName : cats)
        {
            CategorySection section;
            section.name = catName;
            section.greekPrefix = getGreekForCategory(catName);
            section.samples = registry->getSamplesByCategory(catName);
            section.collapsed = false;
            categories.push_back(std::move(section));
        }
    }

    applyFilter();
}

// =========================================================================
// Scanner activity
// =========================================================================

void SampleBrowser::setScannerActive(bool active)
{
    scannerActive = active;
    if (active && !isTimerRunning())
        startTimerHz(30);
    else if (!active)
        stopTimer();
    repaint();
}

void SampleBrowser::timerCallback()
{
    scannerPulsePhase += 0.1f;
    if (scannerPulsePhase > juce::MathConstants<float>::twoPi)
        scannerPulsePhase -= juce::MathConstants<float>::twoPi;
    repaint(); // Just the top bar area for the pulse dot
}

// =========================================================================
// Listener management
// =========================================================================

void SampleBrowser::addListener(SampleBrowserListener* l)    { listeners.add(l); }
void SampleBrowser::removeListener(SampleBrowserListener* l) { listeners.remove(l); }

void SampleBrowser::notifyPreview(const juce::String& sampleId)
{
    listeners.call([&](SampleBrowserListener& l) { l.samplePreviewRequested(sampleId); });
}

void SampleBrowser::notifyAssign(const juce::String& sampleId)
{
    listeners.call([&](SampleBrowserListener& l) { l.sampleAssignRequested(sampleId); });
}

juce::String SampleBrowser::getDragDescriptionForSample(const juce::String& sampleId)
{
    // Prefix with "voidsample:" so drop targets can identify the source
    return "voidsample:" + sampleId;
}

// =========================================================================
// Filtering
// =========================================================================

void SampleBrowser::onSearchTextChanged()
{
    searchFilter = searchBar.getText().trim();
    applyFilter();
}

void SampleBrowser::applyFilter()
{
    filteredCategories.clear();

    for (auto& cat : categories)
    {
        CategorySection filtered;
        filtered.name = cat.name;
        filtered.greekPrefix = cat.greekPrefix;
        filtered.collapsed = cat.collapsed;

        if (searchFilter.isEmpty())
        {
            filtered.samples = cat.samples;
        }
        else
        {
            for (auto& sample : cat.samples)
            {
                if (sample.displayName.containsIgnoreCase(searchFilter) ||
                    sample.id.containsIgnoreCase(searchFilter))
                {
                    filtered.samples.push_back(sample);
                }
            }
        }

        // Only include categories that have matching samples (or if no filter is active)
        if (!filtered.samples.empty() || searchFilter.isEmpty())
            filteredCategories.push_back(std::move(filtered));
    }

    scrollOffset = 0.0f;
    selectedCategoryIdx = -1;
    selectedSampleIdx = -1;
    browserList.repaint();
    scrollBar.repaint();
}

void SampleBrowser::onRefreshClicked()
{
    refreshFromRegistry();
}

// =========================================================================
// Scrolling
// =========================================================================

void SampleBrowser::mouseWheelMove(const juce::MouseEvent&,
                                    const juce::MouseWheelDetails& wheel)
{
    scrollOffset -= wheel.deltaY * 80.0f;
    clampScroll();
    browserList.repaint();
    scrollBar.repaint();
}

void SampleBrowser::clampScroll()
{
    int contentH = browserList.getContentHeight();
    float visibleH = static_cast<float>(browserList.getHeight());
    float maxScroll = juce::jmax(0.0f, static_cast<float>(contentH) - visibleH);
    scrollOffset = juce::jlimit(0.0f, maxScroll, scrollOffset);
}

// =========================================================================
// Paint
// =========================================================================

void SampleBrowser::paint(juce::Graphics& g)
{
    g.fillAll(VoidLookAndFeel::surface());

    // Top bar background
    auto topBar = getLocalBounds().removeFromTop(kTopBarHeight).toFloat();
    g.setColour(VoidLookAndFeel::surfaceRaised());
    g.fillRect(topBar);

    // Scanner activity indicator (pulsing dot)
    if (scannerActive)
    {
        float alpha = 0.5f + 0.5f * std::sin(scannerPulsePhase);
        g.setColour(VoidLookAndFeel::accentCyan().withAlpha(alpha));
        float dotX = topBar.getRight() - 14.0f;
        float dotY = topBar.getCentreY();
        g.fillEllipse(dotX - 3.0f, dotY - 3.0f, 6.0f, 6.0f);
    }

    // Border
    g.setColour(VoidLookAndFeel::ghost());
    g.drawRect(getLocalBounds(), 1);
}

// =========================================================================
// Layout
// =========================================================================

void SampleBrowser::resized()
{
    auto area = getLocalBounds();

    // Top bar: search + refresh button
    auto topBar = area.removeFromTop(kTopBarHeight);
    refreshButton.setBounds(topBar.removeFromRight(70).reduced(2));
    searchBar.setBounds(topBar.reduced(4, 4));

    // Scrollbar on the right
    scrollBar.setBounds(area.removeFromRight(kScrollBarWidth));

    // Remaining area is the browser list
    browserList.setBounds(area);
}

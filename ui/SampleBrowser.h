#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../engine/interfaces/ISampleRegistry.h"
#include "VoidLookAndFeel.h"
#include "WaveformDisplay.h"
#include <functional>
#include <map>
#include <vector>

// =========================================================================
// SampleBrowser
//
// Left-panel terminal-style sample browser. Reads from ISampleRegistry,
// displays samples grouped by category with collapsible headers, inline
// waveform thumbnails, search filtering, and drag-and-drop source.
//
// Visual style:
//   Background   -- surface (#0D0D0D)
//   Text         -- text-primary (#F0F0F0) for names
//   Metadata     -- text-dim (#666666) for duration, etc.
//   Categories   -- Greek accent + CAPS name
//   Scrollbar    -- accent-cyan (#00F0FF) thin custom
//   Accent       -- accent-cyan for selection highlight
// =========================================================================

// Forward declaration
class SampleBrowserEntry;

// =========================================================================
// SampleBrowser::Listener — callback interface for browser events
// =========================================================================
class SampleBrowserListener
{
public:
    virtual ~SampleBrowserListener() = default;

    /** Called on single-click to preview a sample. */
    virtual void samplePreviewRequested(const juce::String& sampleId) = 0;

    /** Called on double-click to assign to the currently selected pad. */
    virtual void sampleAssignRequested(const juce::String& sampleId) = 0;
};

// =========================================================================
// SampleBrowser component
// =========================================================================
class SampleBrowser : public juce::Component,
                      public juce::DragAndDropContainer,
                      public juce::Timer
{
public:
    SampleBrowser();
    ~SampleBrowser() override;

    // -- Registry connection -------------------------------------------------

    /** Set the sample registry to read from. Pass nullptr to disconnect. */
    void setSampleRegistry(void_drum::ISampleRegistry* registry);

    /** Force a rebuild of the displayed tree from the current registry data. */
    void refreshFromRegistry();

    // -- Listener ------------------------------------------------------------
    void addListener(SampleBrowserListener* listener);
    void removeListener(SampleBrowserListener* listener);

    // -- Scanner activity indicator ------------------------------------------
    void setScannerActive(bool active);

    // -- Component overrides -------------------------------------------------
    void paint(juce::Graphics& g) override;
    void resized() override;

    // -- Timer (for scanner pulse animation) ---------------------------------
    void timerCallback() override;

private:
    // -- Internal types ------------------------------------------------------

    struct CategorySection
    {
        juce::String name;
        juce::String greekPrefix; // Decorative Greek character
        std::vector<void_drum::SampleEntry> samples;
        bool collapsed = false;
    };

    // -- Sub-components ------------------------------------------------------
    class SearchBar : public juce::TextEditor
    {
    public:
        SearchBar();
    };

    class BrowserList : public juce::Component
    {
    public:
        BrowserList(SampleBrowser& owner);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDoubleClick(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        int getContentHeight() const;

    private:
        SampleBrowser& browser;
        int getItemIndexAt(int y) const;

        // Returns: {category index, sample index within category} or {-1,-1}
        std::pair<int, int> getItemAt(int y) const;
    };

    class CustomScrollBar : public juce::Component
    {
    public:
        CustomScrollBar(SampleBrowser& owner);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    private:
        SampleBrowser& browser;
        bool dragging = false;
        float dragStartY = 0.0f;
        float dragStartOffset = 0.0f;
    };

    // -- Data ----------------------------------------------------------------
    void_drum::ISampleRegistry* registry = nullptr;
    std::vector<CategorySection> categories;
    juce::String searchFilter;
    std::vector<CategorySection> filteredCategories;

    // -- UI state ------------------------------------------------------------
    float scrollOffset = 0.0f;
    int selectedCategoryIdx = -1;
    int selectedSampleIdx  = -1;
    bool scannerActive = false;
    float scannerPulsePhase = 0.0f;

    // -- Listeners -----------------------------------------------------------
    juce::ListenerList<SampleBrowserListener> listeners;

    // -- Sub-components ------------------------------------------------------
    SearchBar searchBar;
    BrowserList browserList;
    CustomScrollBar scrollBar;
    juce::TextButton refreshButton;

    // -- Layout constants ----------------------------------------------------
    static constexpr int kSearchBarHeight = 28;
    static constexpr int kHeaderHeight    = 24;
    static constexpr int kItemHeight      = 22;
    static constexpr int kScrollBarWidth  = 6;
    static constexpr int kTopBarHeight    = 32;
    static constexpr int kWaveformWidth   = 40;

    // -- Greek characters for category decoration ----------------------------
    static juce::String getGreekForCategory(const juce::String& cat);

    // -- Filtering -----------------------------------------------------------
    void applyFilter();

    // -- Helpers -------------------------------------------------------------
    void onSearchTextChanged();
    void onRefreshClicked();
    void notifyPreview(const juce::String& sampleId);
    void notifyAssign(const juce::String& sampleId);
    juce::String getDragDescriptionForSample(const juce::String& sampleId);

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void clampScroll();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleBrowser)
};

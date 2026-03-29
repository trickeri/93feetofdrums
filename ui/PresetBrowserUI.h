#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../engine/interfaces/IKitPreset.h"
#include "../engine/HostIntegration.h"
#include <vector>

// Forward declaration
class VOIDDrumEngineProcessor;

// =========================================================================
// PresetEntry: UI model for a single item in the preset list.
// =========================================================================
struct PresetEntry
{
    juce::String name;
    juce::String author;
    juce::File   file;
    bool         isFactory = false;
    int          padCount  = 0; // number of pads with assigned samples
};

// =========================================================================
// PresetBrowserUI
//
// Dark overlay panel for browsing, loading, and saving kit presets.
// Features:
//   - Factory presets (read-only) and User presets (writable) sections
//   - Click to load, Save / Save As / Init Kit buttons
//   - A/B comparison toggle
//   - Visual style: dark surface with monospace font, cyan accents
// =========================================================================
class PresetBrowserUI : public juce::Component,
                        public juce::Button::Listener,
                        public juce::ListBoxModel
{
public:
    /** Construct with references to processor and host integration. */
    PresetBrowserUI(VOIDDrumEngineProcessor& processor,
                    void_drum::HostIntegration& hostIntegration);

    ~PresetBrowserUI() override;

    // -- Component overrides --------------------------------------------------
    void paint(juce::Graphics& g) override;
    void resized() override;

    // -- Visibility control ---------------------------------------------------
    /** Show the browser as an overlay (call from parent component). */
    void showBrowser();

    /** Hide the browser overlay. */
    void hideBrowser();

    /** Toggle visibility. */
    void toggleBrowser();

    // -- Preset list management -----------------------------------------------
    /** Refresh the preset list from disk (factory + user directories). */
    void refreshPresetList();

    /** Set the factory presets directory. */
    void setFactoryPresetsDir(const juce::File& dir);

    /** Set the user presets directory. */
    void setUserPresetsDir(const juce::File& dir);

    // -- ListBoxModel overrides -----------------------------------------------
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g,
                          int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;

    // -- Button::Listener override --------------------------------------------
    void buttonClicked(juce::Button* button) override;

    // -- Callback for preset load (set by parent) -----------------------------
    std::function<void(const juce::File& presetFile)> onPresetLoad;
    std::function<void()> onInitKit;

private:
    /** Load preset entries from a directory. */
    void scanDirectory(const juce::File& dir, bool factory);

    /** Prompt user for a name and save the current state. */
    void savePresetAs();

    /** Save the current state to the currently selected preset file (user only). */
    void savePreset();

    // References
    VOIDDrumEngineProcessor& processorRef;
    void_drum::HostIntegration& hostIntegration;

    // Preset data
    std::vector<PresetEntry> presets;
    int selectedIndex = -1;

    // Directories
    juce::File factoryDir;
    juce::File userDir;

    // UI components
    juce::ListBox presetList { "PresetList", this };
    juce::TextButton closeButton   { "X" };
    juce::TextButton saveButton    { "SAVE" };
    juce::TextButton saveAsButton  { "SAVE AS" };
    juce::TextButton initKitButton { "INIT KIT" };
    juce::TextButton slotAButton   { "A" };
    juce::TextButton slotBButton   { "B" };
    juce::TextButton copyAToBButton { juce::CharPointer_UTF8("A\xe2\x86\x92\x42") }; // A->B
    juce::TextButton copyBToAButton { juce::CharPointer_UTF8("B\xe2\x86\x92\x41") }; // B->A

    // Colours (matching the VOID design palette)
    static constexpr juce::uint32 colSurface       = 0xFF0D0D0D;
    static constexpr juce::uint32 colSurfaceRaised = 0xFF1A1A1A;
    static constexpr juce::uint32 colSurfaceHot    = 0xFF252525;
    static constexpr juce::uint32 colTextPrimary   = 0xFFF0F0F0;
    static constexpr juce::uint32 colTextDim       = 0xFF666666;
    static constexpr juce::uint32 colAccentCyan    = 0xFF00F0FF;
    static constexpr juce::uint32 colAccentMagenta = 0xFFFF00AA;
    static constexpr juce::uint32 colGhost         = 0xFF333333;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserUI)
};

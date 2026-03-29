#include "PresetBrowserUI.h"
#include "../engine/PluginProcessor.h"

// =========================================================================
// Construction / destruction
// =========================================================================

PresetBrowserUI::PresetBrowserUI(VOIDDrumEngineProcessor& processor,
                                 void_drum::HostIntegration& host)
    : processorRef(processor),
      hostIntegration(host)
{
    setVisible(false);

    // Preset list
    addAndMakeVisible(presetList);
    presetList.setColour(juce::ListBox::backgroundColourId,
                         juce::Colour(colSurface));
    presetList.setColour(juce::ListBox::outlineColourId,
                         juce::Colour(colGhost));
    presetList.setRowHeight(32);

    // Buttons
    auto setupButton = [this](juce::TextButton& btn, juce::uint32 textCol)
    {
        addAndMakeVisible(btn);
        btn.addListener(this);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(colSurfaceRaised));
        btn.setColour(juce::TextButton::textColourOnId, juce::Colour(textCol));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(textCol));
    };

    setupButton(closeButton,    colTextPrimary);
    setupButton(saveButton,     colAccentCyan);
    setupButton(saveAsButton,   colAccentCyan);
    setupButton(initKitButton,  colAccentMagenta);
    setupButton(slotAButton,    colAccentCyan);
    setupButton(slotBButton,    colTextDim);
    setupButton(copyAToBButton, colTextDim);
    setupButton(copyBToAButton, colTextDim);

    // Default directories
    auto dataDir = juce::File::getSpecialLocation(
        juce::File::commonApplicationDataDirectory)
        .getChildFile("VOID_Drum_Engine");

    factoryDir = dataDir.getChildFile("Factory_Presets");
    userDir    = dataDir.getChildFile("User_Presets");
}

PresetBrowserUI::~PresetBrowserUI() = default;

// =========================================================================
// Visibility control
// =========================================================================

void PresetBrowserUI::showBrowser()
{
    refreshPresetList();
    setVisible(true);
    toFront(true);
}

void PresetBrowserUI::hideBrowser()
{
    setVisible(false);
}

void PresetBrowserUI::toggleBrowser()
{
    if (isVisible())
        hideBrowser();
    else
        showBrowser();
}

// =========================================================================
// Preset list management
// =========================================================================

void PresetBrowserUI::setFactoryPresetsDir(const juce::File& dir) { factoryDir = dir; }
void PresetBrowserUI::setUserPresetsDir(const juce::File& dir)    { userDir = dir; }

void PresetBrowserUI::refreshPresetList()
{
    presets.clear();
    selectedIndex = -1;

    scanDirectory(factoryDir, true);
    scanDirectory(userDir, false);

    presetList.updateContent();
    presetList.repaint();
}

void PresetBrowserUI::scanDirectory(const juce::File& dir, bool factory)
{
    if (!dir.isDirectory())
        return;

    for (const auto& entry :
         juce::RangedDirectoryIterator(dir, false, "*.voidkit",
                                       juce::File::findFiles))
    {
        auto file = entry.getFile();

        PresetEntry pe;
        pe.file      = file;
        pe.name      = file.getFileNameWithoutExtension();
        pe.isFactory = factory;
        pe.author    = "";
        pe.padCount  = 0;

        // Try to parse the JSON to get author and pad count
        auto jsonText = file.loadFileAsString();
        auto parsed = juce::JSON::parse(jsonText);
        if (auto* obj = parsed.getDynamicObject())
        {
            if (obj->hasProperty("author"))
                pe.author = obj->getProperty("author").toString();
            if (obj->hasProperty("name"))
                pe.name = obj->getProperty("name").toString();
            if (auto* padsArr = obj->getProperty("pads").getArray())
                pe.padCount = padsArr->size();
        }

        presets.push_back(std::move(pe));
    }
}

// =========================================================================
// ListBoxModel
// =========================================================================

int PresetBrowserUI::getNumRows()
{
    return static_cast<int>(presets.size());
}

void PresetBrowserUI::paintListBoxItem(int rowNumber, juce::Graphics& g,
                                       int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(presets.size()))
        return;

    const auto& entry = presets[static_cast<size_t>(rowNumber)];

    // Background
    if (rowIsSelected)
        g.fillAll(juce::Colour(colSurfaceHot));
    else if (rowNumber % 2 == 0)
        g.fillAll(juce::Colour(colSurface));
    else
        g.fillAll(juce::Colour(colSurfaceRaised));

    // Factory / User tag
    auto tagArea = juce::Rectangle<int>(4, 0, 60, height);
    g.setColour(entry.isFactory ? juce::Colour(colTextDim)
                                : juce::Colour(colAccentCyan));
    g.setFont(juce::Font(10.0f));
    g.drawText(entry.isFactory ? "FACTORY" : "USER",
               tagArea, juce::Justification::centredLeft, false);

    // Preset name
    auto nameArea = juce::Rectangle<int>(68, 0, width - 180, height);
    g.setColour(rowIsSelected ? juce::Colour(colAccentCyan)
                              : juce::Colour(colTextPrimary));
    g.setFont(juce::Font(14.0f));
    g.drawText(entry.name, nameArea, juce::Justification::centredLeft, true);

    // Author
    if (entry.author.isNotEmpty())
    {
        auto authorArea = juce::Rectangle<int>(width - 160, 0, 100, height);
        g.setColour(juce::Colour(colTextDim));
        g.setFont(juce::Font(11.0f));
        g.drawText(entry.author, authorArea, juce::Justification::centredLeft, true);
    }

    // Pad count indicator
    auto padArea = juce::Rectangle<int>(width - 50, 0, 44, height);
    g.setColour(juce::Colour(colTextDim));
    g.setFont(juce::Font(10.0f));
    g.drawText(juce::String(entry.padCount) + "p",
               padArea, juce::Justification::centredRight, false);

    // Bottom border line
    g.setColour(juce::Colour(colGhost));
    g.drawLine(0.0f, static_cast<float>(height - 1),
               static_cast<float>(width), static_cast<float>(height - 1), 1.0f);
}

void PresetBrowserUI::listBoxItemClicked(int row, const juce::MouseEvent& /*e*/)
{
    selectedIndex = row;
    presetList.repaint();
}

void PresetBrowserUI::listBoxItemDoubleClicked(int row, const juce::MouseEvent& /*e*/)
{
    if (row >= 0 && row < static_cast<int>(presets.size()))
    {
        selectedIndex = row;
        if (onPresetLoad)
            onPresetLoad(presets[static_cast<size_t>(row)].file);
    }
}

// =========================================================================
// Button handler
// =========================================================================

void PresetBrowserUI::buttonClicked(juce::Button* button)
{
    if (button == &closeButton)
    {
        hideBrowser();
    }
    else if (button == &saveButton)
    {
        savePreset();
    }
    else if (button == &saveAsButton)
    {
        savePresetAs();
    }
    else if (button == &initKitButton)
    {
        if (onInitKit)
            onInitKit();
    }
    else if (button == &slotAButton || button == &slotBButton)
    {
        hostIntegration.toggleAB();

        // Update button appearance
        bool isSlotA = (hostIntegration.getActiveSlot() ==
                        void_drum::HostIntegration::Slot::A);
        slotAButton.setColour(juce::TextButton::textColourOffId,
                              juce::Colour(isSlotA ? colAccentCyan : colTextDim));
        slotBButton.setColour(juce::TextButton::textColourOffId,
                              juce::Colour(isSlotA ? colTextDim : colAccentCyan));
        slotAButton.repaint();
        slotBButton.repaint();
    }
    else if (button == &copyAToBButton)
    {
        hostIntegration.copyAToB();
    }
    else if (button == &copyBToAButton)
    {
        hostIntegration.copyBToA();
    }
}

// =========================================================================
// Save helpers
// =========================================================================

void PresetBrowserUI::savePreset()
{
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(presets.size()))
    {
        const auto& entry = presets[static_cast<size_t>(selectedIndex)];
        if (entry.isFactory)
        {
            // Cannot overwrite factory presets -- redirect to Save As
            savePresetAs();
            return;
        }

        // Save current APVTS state as a .voidkit JSON file
        auto state = processorRef.getAPVTS().copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        if (xml != nullptr)
            entry.file.replaceWithText(xml->toString());

        refreshPresetList();
    }
    else
    {
        savePresetAs();
    }
}

void PresetBrowserUI::savePresetAs()
{
    // Ensure user directory exists
    if (!userDir.isDirectory())
        userDir.createDirectory();

    auto chooser = std::make_shared<juce::FileChooser>(
        "Save Preset As...",
        userDir,
        "*.voidkit");

    chooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, chooser](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File())
                return;

            // Ensure .voidkit extension
            if (!file.hasFileExtension(".voidkit"))
                file = file.withFileExtension(".voidkit");

            auto state = processorRef.getAPVTS().copyState();
            std::unique_ptr<juce::XmlElement> xml(state.createXml());
            if (xml != nullptr)
                file.replaceWithText(xml->toString());

            refreshPresetList();
        });
}

// =========================================================================
// Paint
// =========================================================================

void PresetBrowserUI::paint(juce::Graphics& g)
{
    // Semi-transparent dark overlay covering the entire parent
    g.fillAll(juce::Colour(0xDD0D0D0D));

    auto bounds = getLocalBounds().reduced(40);

    // Main panel background
    g.setColour(juce::Colour(colSurface));
    g.fillRect(bounds);

    // Border
    g.setColour(juce::Colour(colGhost));
    g.drawRect(bounds, 1);

    // Title bar
    auto titleBar = bounds.removeFromTop(40);
    g.setColour(juce::Colour(colSurfaceRaised));
    g.fillRect(titleBar);

    g.setColour(juce::Colour(colTextPrimary));
    g.setFont(juce::Font(16.0f));
    g.drawText(juce::CharPointer_UTF8("\xce\xa3  PRESET BROWSER"),
               titleBar.reduced(12, 0),
               juce::Justification::centredLeft, false);

    // Section labels (drawn above the list area)
    auto headerArea = bounds.removeFromTop(20);
    g.setFont(juce::Font(10.0f));
    g.setColour(juce::Colour(colTextDim));
    g.drawText("TAG", headerArea.withX(bounds.getX() + 4).withWidth(60),
               juce::Justification::centredLeft, false);
    g.drawText("NAME", headerArea.withX(bounds.getX() + 68).withWidth(200),
               juce::Justification::centredLeft, false);
    g.drawText("AUTHOR", headerArea.withX(bounds.getRight() - 160).withWidth(100),
               juce::Justification::centredLeft, false);
    g.drawText("PADS", headerArea.withX(bounds.getRight() - 50).withWidth(44),
               juce::Justification::centredRight, false);
}

// =========================================================================
// Layout
// =========================================================================

void PresetBrowserUI::resized()
{
    auto bounds = getLocalBounds().reduced(40);
    auto titleBar = bounds.removeFromTop(40);
    bounds.removeFromTop(20); // header row

    // Close button in title bar
    closeButton.setBounds(titleBar.removeFromRight(40));

    // A/B buttons in title bar
    auto abArea = titleBar.removeFromRight(220);
    slotAButton.setBounds(abArea.removeFromLeft(30).reduced(2));
    slotBButton.setBounds(abArea.removeFromLeft(30).reduced(2));
    abArea.removeFromLeft(8);
    copyAToBButton.setBounds(abArea.removeFromLeft(50).reduced(2));
    copyBToAButton.setBounds(abArea.removeFromLeft(50).reduced(2));

    // Bottom button bar
    auto bottomBar = bounds.removeFromBottom(44);
    bottomBar = bottomBar.reduced(8, 4);
    initKitButton.setBounds(bottomBar.removeFromLeft(80));
    bottomBar.removeFromLeft(8);
    saveButton.setBounds(bottomBar.removeFromRight(80));
    bottomBar.removeFromRight(8);
    saveAsButton.setBounds(bottomBar.removeFromRight(80));

    // Preset list fills the remaining space
    presetList.setBounds(bounds.reduced(4, 2));
}

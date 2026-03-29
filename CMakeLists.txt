cmake_minimum_required(VERSION 3.22)
project(VOID_DRUM_ENGINE VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Fetch JUCE 7 via FetchContent
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.9
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(VOIDDrumEngine
    PRODUCT_NAME               "VOID Drum Engine"
    COMPANY_NAME               "93feetofdrums"
    PLUGIN_MANUFACTURER_CODE   93Ft
    PLUGIN_CODE                Void
    FORMATS                    VST3          # Primary target: Windows VST3
    IS_SYNTH                   TRUE
    NEEDS_MIDI_INPUT           TRUE
    NEEDS_MIDI_OUTPUT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD    FALSE
)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
target_sources(VOIDDrumEngine
    PRIVATE
        engine/PluginProcessor.cpp
        engine/PluginProcessor.h
        engine/SampleLoader.cpp
        engine/SampleLoader.h
        engine/VoiceAllocator.cpp
        engine/VoiceAllocator.h
        engine/PadDSP.cpp
        engine/PadDSP.h
        engine/MixBus.cpp
        engine/MixBus.h
        engine/MIDIMapper.cpp
        engine/MIDIMapper.h
        engine/HostIntegration.cpp
        engine/HostIntegration.h
        engine/FolderScanner.cpp
        engine/FolderScanner.h
        engine/SampleRegistry.cpp
        engine/SampleRegistry.h
        engine/PresetManager.cpp
        engine/PresetManager.h
        engine/interfaces/ISampleRegistry.h
        engine/interfaces/IPadState.h
        engine/interfaces/IKitPreset.h
        engine/interfaces/IMIDIMapping.h
        ui/PluginEditor.cpp
        ui/PluginEditor.h
        ui/VoidLookAndFeel.h
        ui/VoidLookAndFeel.cpp
        ui/DrumKitView.h
        ui/DrumKitView.cpp
        ui/PadGrid.h
        ui/PadGrid.cpp
        ui/WaveformDisplay.h
        ui/WaveformDisplay.cpp
        ui/SampleBrowser.h
        ui/SampleBrowser.cpp
        ui/MixerStrip.h
        ui/MixerStrip.cpp
        ui/FXPanel.h
        ui/FXPanel.cpp
        ui/PresetBrowserUI.h
        ui/PresetBrowserUI.cpp
)

target_include_directories(VOIDDrumEngine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---------------------------------------------------------------------------
# JUCE module dependencies
# ---------------------------------------------------------------------------
target_link_libraries(VOIDDrumEngine
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---------------------------------------------------------------------------
# Compile definitions
# ---------------------------------------------------------------------------
target_compile_definitions(VOIDDrumEngine
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

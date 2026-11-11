cmake_minimum_required(VERSION 3.22)
project(Decu6000 VERSION 0.9.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows: link the C++ runtime statically so Decu6000.exe / .vst3 run without
# the Visual C++ Redistributable installed.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Decu6000
    COMPANY_NAME "Decu"
    PLUGIN_MANUFACTURER_CODE Decu
    PLUGIN_CODE Dcu6
    PRODUCT_NAME "Decu6000"
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Decu6000 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PatchManager.cpp
    Source/PatchManager.h
    Source/FactoryPatches.h
    Source/GatedReverb.h
    Source/BioEngine.h
    Source/Parameters.h
    Source/NoteMap.h
    Source/PadMap.h
    Source/DspUtils.h
    Source/DrumVoice.h
    Source/KickVoice.h
    Source/SnareVoice.h
    Source/HatVoice.h
    Source/TomVoice.h
    Source/CymbalVoice.h
    Source/PercVoice.h
    Source/CrystalVoice.h
    Source/LiquidVoice.h
    Source/BreathVoice.h
    Source/DigitalVoice.h
    Source/BurstVoice.h
    Source/UI/Theme.cpp
    Source/UI/Theme.h
    Source/UI/Widgets.cpp
    Source/UI/Widgets.h
    Source/UI/Views.cpp
    Source/UI/Views.h)

# Embedded fonts (SIL OFL 1.1 - see Resources/Fonts)
juce_add_binary_data(DecuFonts SOURCES
    Resources/Fonts/DecuDisplay-Bold.ttf
    Resources/Fonts/DecuDisplay-Black.ttf
    Resources/Fonts/ShareTechMono-Regular.ttf)

target_compile_definitions(Decu6000 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

# Linux: ALSA (default) plus JACK for the Standalone's audio device list.
# JACK is loaded at runtime if installed, so it's not a hard dependency.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(Decu6000 PUBLIC JUCE_ALSA=1 JUCE_JACK=1)
endif()

target_link_libraries(Decu6000
    PRIVATE juce::juce_audio_utils
            DecuFonts
    PUBLIC  juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)

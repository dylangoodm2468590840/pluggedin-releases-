cmake_minimum_required(VERSION 3.22)
project(UNDERGROUND VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use static linking for C++ runtime libraries to eliminate missing DLL errors on user PCs
if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -pthread")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static -static-libgcc -static-libstdc++ -pthread")
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

set(JUCEAIDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dummy_juceaide.exe" CACHE PATH "" FORCE)
set(JUCE_TOOL_juceaide "${CMAKE_CURRENT_SOURCE_DIR}/dummy_juceaide.exe" CACHE PATH "" FORCE)
set(JUCE_BUILD_HELPER_TOOLS OFF CACHE BOOL "" FORCE)

# Use local JUCE in libs/JUCE if available
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/JUCE/CMakeLists.txt")
    add_subdirectory(libs/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 7.0.12
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(UNDERGROUND
    COMPANY_NAME "UndergroundDSP"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Undg
    PLUGIN_CODE Undg
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "UNDERGROUND"
    VERSION "2.5.0"
)

target_sources(UNDERGROUND
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/ShadowProcessor.cpp
        Source/DSP/CrushProcessor.cpp
        Source/DSP/WidthProcessor.cpp
        Source/DSP/SpaceProcessor.cpp
        Source/DSP/DeviceProcessor.cpp
        Source/DSP/EQEngine.cpp
        Source/DSP/VocalCompressor.cpp
        Source/DSP/DeEsserProcessor.cpp
        Source/DSP/AirExciterProcessor.cpp
        Source/DSP/SignalChain.cpp


        Source/Utils/PluggedINAutoUpdater.cpp
        Source/Presets/PresetManager.cpp
        Source/UI/UndergroundLookAndFeel.cpp
        Source/UI/RackUnitLookAndFeel.cpp
        Source/UI/DegenerateKnob.cpp
        Source/UI/VisualEQDisplay.cpp
)

target_compile_definitions(UNDERGROUND
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(UNDERGROUND
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
)

# PluggedIN Central Standalone Desktop Application Executable Target
juce_add_gui_app(PluggedIN_Central
    PRODUCT_NAME "PluggedIN Central"
    COMPANY_NAME "PluggedIN Audio Systems"
    BUNDLE_ID "com.pluggedin.central"
    VERSION "1.0.7"
)

target_sources(PluggedIN_Central
    PRIVATE
        Source/Central/PluggedINCentralMain.cpp
        Source/Utils/PluggedINAutoUpdater.cpp
        Source/UI/RackUnitLookAndFeel.cpp
        Source/UI/DegenerateKnob.cpp
)

target_link_libraries(PluggedIN_Central
    PRIVATE
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_core
        juce::juce_events
)

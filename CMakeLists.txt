cmake_minimum_required(VERSION 3.22)
project(UNDERGROUND VERSION 4.0.0 LANGUAGES C CXX)

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
    VERSION "${CMAKE_PROJECT_VERSION}"   # Always in sync with project() version above
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
        Source/DSP/VocalResonanceProcessor.cpp
        Source/DSP/StudioMicroDetuner.cpp
        Source/DSP/AnalogTransformerCore.cpp
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
    VERSION "2.1.0"
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
        juce::juce_audio_basics
)

# Plugged 1 Flagship Hybrid Instrument Plugin Target
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/Plugged1/PluginProcessor.cpp")
    juce_add_plugin(Plugged_1
        COMPANY_NAME "Plugged In Central"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        PLUGIN_MANUFACTURER_CODE Plug
        PLUGIN_CODE P101
        FORMATS VST3 AU Standalone
        PRODUCT_NAME "Plugged 1"
        DESCRIPTION "Plugged 1 - Flagship Hybrid Instrument Plugin"
        VERSION "1.0.0"
    )

    target_sources(Plugged_1 PRIVATE
        Source/Plugged1/PluginProcessor.cpp
        Source/Plugged1/PluginProcessor.h
        Source/Plugged1/PluginEditor.cpp
        Source/Plugged1/PluginEditor.h
        Source/Plugged1/Engine/PluggedSound.h
        Source/Plugged1/Engine/PluggedVoice.cpp
        Source/Plugged1/Engine/PluggedVoice.h
        Source/Plugged1/Engine/VoiceManager.cpp
        Source/Plugged1/Engine/VoiceManager.h
        Source/Plugged1/Engine/PolyBLEP.h
        Source/Plugged1/DSP/StereoSaturator.h
        Source/Plugged1/DSP/LadderFilter.h
        Source/Plugged1/DSP/StudioDelay.h
        Source/Plugged1/DSP/StudioReverb.h
        Source/Plugged1/Presets/PresetManager.h
        Source/Plugged1/Presets/PresetManager.cpp
    )

    target_include_directories(Plugged_1 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/Plugged1"
    )

    target_compile_definitions(Plugged_1
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
    )

    target_link_libraries(Plugged_1 PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    )
endif()

# Automated Test Harness Executable
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Source/TestAdversarial.cpp")
    add_executable(TestAdversarial
        Source/TestAdversarial.cpp
        Source/DSP/ShadowProcessor.cpp
        Source/DSP/CrushProcessor.cpp
        Source/DSP/WidthProcessor.cpp
        Source/DSP/SpaceProcessor.cpp
        Source/DSP/DeviceProcessor.cpp
        Source/DSP/EQEngine.cpp
        Source/DSP/VocalCompressor.cpp
        Source/DSP/DeEsserProcessor.cpp
        Source/DSP/AirExciterProcessor.cpp
        Source/DSP/VocalResonanceProcessor.cpp
        Source/DSP/StudioMicroDetuner.cpp
        Source/DSP/AnalogTransformerCore.cpp
        Source/DSP/SignalChain.cpp
        Source/Presets/PresetManager.cpp
    )

    target_compile_definitions(TestAdversarial
        PUBLIC
            JUCE_STANDALONE_APPLICATION=0
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
    )

    target_link_libraries(TestAdversarial
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_core
    )
endif()

# Diagnostic Audit Runner
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/diagnostic_runner.cpp")
    add_executable(DiagnosticRunner
        tools/diagnostic_runner.cpp
        Source/DSP/ShadowProcessor.cpp
        Source/DSP/CrushProcessor.cpp
        Source/DSP/WidthProcessor.cpp
        Source/DSP/SpaceProcessor.cpp
        Source/DSP/DeviceProcessor.cpp
        Source/DSP/EQEngine.cpp
        Source/DSP/VocalCompressor.cpp
        Source/DSP/DeEsserProcessor.cpp
        Source/DSP/AirExciterProcessor.cpp
        Source/DSP/VocalResonanceProcessor.cpp
        Source/DSP/StudioMicroDetuner.cpp
        Source/DSP/AnalogTransformerCore.cpp
        Source/DSP/SignalChain.cpp
        Source/Presets/PresetManager.cpp
    )

    target_compile_definitions(DiagnosticRunner
        PUBLIC
            JUCE_STANDALONE_APPLICATION=0
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
    )

    target_link_libraries(DiagnosticRunner
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_gui_extra
            juce::juce_gui_basics
            juce::juce_core
    )
endif()



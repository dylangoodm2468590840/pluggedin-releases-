#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include <vector>

namespace Plugged1
{

// Custom Dark-Mode Look & Feel with modern cyan/neon accents
class PluggedLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PluggedLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;
};

// Real-Time Animated Oscilloscope Component
class OscilloscopeComponent : public juce::Component, public juce::Timer
{
public:
    OscilloscopeComponent(Plugged1AudioProcessor& processor);
    ~OscilloscopeComponent() override;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    Plugged1AudioProcessor& audioProcessor;
    std::array<float, Plugged1AudioProcessor::visualizerBufferSize> waveData {};
};

class Plugged1AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit Plugged1AudioProcessorEditor(Plugged1AudioProcessor&);
    ~Plugged1AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text,
                     const juce::String& paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment);
    void setupComboBox(juce::ComboBox& box, juce::Label& label, const juce::String& text,
                       const juce::String& paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& attachment);
    void updatePresetDropdown();

    Plugged1AudioProcessor& audioProcessor;
    PluggedLookAndFeel lookAndFeel;

    // Header / Preset Controls
    juce::ComboBox categoryBox;
    juce::ComboBox presetBox;
    juce::TextButton prevPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::ComboBox voiceModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> voiceModeAttachment;

    // UI Scale / Resize Quick Buttons
    juce::TextButton scale75Button { "75%" };
    juce::TextButton scale100Button { "100%" };
    juce::TextButton scale125Button { "125%" };

    // Real-time Visualizer
    OscilloscopeComponent oscilloscope;

    // 4 Macro Knobs
    juce::Slider macroPunchSlider, macroDirtSlider, macroSpaceSlider, macroAirSlider;
    juce::Label macroPunchLabel, macroDirtLabel, macroSpaceLabel, macroAirLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> macroPunchAttach, macroDirtAttach, macroSpaceAttach, macroAirAttach;

    // Layer 1: Sub / 808 Machine
    juce::ComboBox subWaveBox;
    juce::Slider subPunchSlider, subDriveSlider, subGainSlider, glideSlider;
    juce::Label subWaveLabel, subPunchLabel, subDriveLabel, subGainLabel, glideLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> subWaveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subPunchAttach, subDriveAttach, subGainAttach, glideAttach;

    // Layer 2: Synth Engine
    juce::ComboBox synthShapeBox;
    juce::Slider synthUnisonSlider, synthDetuneSlider, synthSpreadSlider, synthGainSlider;
    juce::Label synthShapeLabel, synthUnisonLabel, synthDetuneLabel, synthSpreadLabel, synthGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> synthShapeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> synthUnisonAttach, synthDetuneAttach, synthSpreadAttach, synthGainAttach;

    // Amp ADSR
    juce::Slider ampAttackSlider, ampDecaySlider, ampSustainSlider, ampReleaseSlider;
    juce::Label ampAttackLabel, ampDecayLabel, ampSustainLabel, ampReleaseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampAttackAttach, ampDecayAttach, ampSustainAttach, ampReleaseAttach;

    // Filter & FX
    juce::Slider cutoffSlider, resSlider, fxDriveSlider, delayMixSlider, reverbMixSlider, masterGainSlider;
    juce::Label cutoffLabel, resLabel, fxDriveLabel, delayMixLabel, reverbMixLabel, masterGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttach, resAttach, fxDriveAttach, delayMixAttach, reverbMixAttach, masterGainAttach;

    // Interactive Virtual MIDI Piano Keyboard
    juce::MidiKeyboardComponent keyboardComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plugged1AudioProcessorEditor)
};

} // namespace Plugged1

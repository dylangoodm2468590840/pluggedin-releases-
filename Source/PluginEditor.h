#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/RackUnitLookAndFeel.h"
#include "UI/DegenerateKnob.h"
#include "UI/VisualEQDisplay.h"
#include "UI/HardwareMaterials.h"

class UndergroundAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit UndergroundAudioProcessorEditor (UndergroundAudioProcessor&);
    ~UndergroundAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    UndergroundAudioProcessor& audioProcessor;
    RackUnitLookAndFeel customLookAndFeel;

    // Cached Brushed Gunmetal Texture Image
    juce::Image chassisTexture;

    // Top Preset Browser Controls
    juce::ComboBox presetModeBox;
    juce::Label presetModeLabel;
    juce::TextButton prevPresetButton { "<" };
    juce::TextButton nextPresetButton { ">" };
    juce::TextButton abButton         { "A/B" };
    juce::TextButton bypassButton     { "BYPASS" };
    juce::TextButton setupButton      { "SETUP" };
    juce::TextButton oversamplingButton { "OVERSAMPLING" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> presetModeAttach;

    // Signature Macro Component (Top Crown Hero)
    DegenerateKnob degenerateKnob;
    juce::Label degenerateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> degenerateAttach;

    // Dynamics & Taming Controls (Flanking DEGENERATE Crown)
    juce::Slider compSqueezeSlider;
    juce::Label compSqueezeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> compSqueezeAttach;

    juce::Slider deEssAmountSlider;
    juce::Label deEssAmountLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> deEssAmountAttach;

    // Psychoacoustic Fresh Air Controls
    juce::Slider airMidSlider;
    juce::Label airMidLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> airMidAttach;

    juce::Slider airTopSlider;
    juce::Label airTopLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> airTopAttach;

    // Real-Time Visual EQ Curve Display Window

    VisualEQDisplay visualEQDisplay;

    // Global Trim Sliders
    juce::Slider inputGainSlider;
    juce::Label inputGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttach;

    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttach;

    juce::Slider mixGlobalSlider;
    juce::Label mixGlobalLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixGlobalAttach;


    // 15 Module Knobs & Controls (3 Per Column matching 1:1 Sketch Photo)
    // Column 1: SUB BASS (Shadow Engine)
    juce::Slider subDriveSlider;  juce::Label subDriveLabel;
    juce::Slider subWidthSlider;  juce::Label subWidthLabel;
    juce::Slider subCompSlider;   juce::Label subCompLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subDriveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subWidthAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subCompAttach;

    // Column 2: GRIT (Crush Engine & 5-Circuit Saturation)
    juce::Slider gritFuzzSlider;  juce::Label gritFuzzLabel;
    juce::Slider gritDustSlider;  juce::Label gritDustLabel;
    juce::Slider gritBitSlider;   juce::Label gritBitLabel;
    juce::TextButton punishButton { "PUNISH" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gritFuzzAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gritDustAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gritBitAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> punishAttach;


    // Column 3: MODULATION (Width Engine)
    juce::Slider modChorusSlider; juce::Label modChorusLabel;
    juce::Slider modPhaseSlider;  juce::Label modPhaseLabel;
    juce::Slider modVibeSlider;   juce::Label modVibeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modChorusAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modPhaseAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modVibeAttach;

    // Column 4: DELAY (Space Delay Engine)
    juce::Slider delayTimeSlider; juce::Label delayTimeLabel;
    juce::Slider delayFbSlider;   juce::Label delayFbLabel;
    juce::Slider delayMixSlider;  juce::Label delayMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayFbAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayMixAttach;

    // Column 5: REVERB (Space Reverb Engine & Device)
    juce::Slider verbSizeSlider;  juce::Label verbSizeLabel;
    juce::Slider verbDecaySlider; juce::Label verbDecayLabel;
    juce::Slider verbSpaceSlider; juce::Label verbSpaceLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> verbSizeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> verbDecayAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> verbSpaceAttach;

    // Device ComboBox
    juce::ComboBox deviceTypeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> deviceTypeAttach;

    // 5 Module Bypass Toggle Switches & Ruby Red LED Indicators
    juce::ToggleButton subEnableToggle;
    juce::ToggleButton gritEnableToggle;
    juce::ToggleButton modEnableToggle;
    juce::ToggleButton delayEnableToggle;
    juce::ToggleButton reverbEnableToggle;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> subEnableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gritEnableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modEnableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> delayEnableAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> reverbEnableAttach;

    // Metering Levels
    float currentInLevel  { 0.0f };
    float currentOutLevel { 0.0f };
    float currentCompGr   { 0.0f };
    float currentDeEssGr  { 0.0f };
    float currentResReduction[4] { 0.0f, 0.0f, 0.0f, 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UndergroundAudioProcessorEditor)
};



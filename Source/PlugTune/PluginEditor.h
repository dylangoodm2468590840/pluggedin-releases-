#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/PlugTuneLookFeel.h"
#include "UI/HeatMapKeyboard.h"

class PlugTuneAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     public juce::Timer,
                                     public juce::FileDragAndDropTarget
{
public:
    explicit PlugTuneAudioProcessorEditor(PlugTuneAudioProcessor&);
    ~PlugTuneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // File Drag & Drop for Beat Key Detection
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

private:
    PlugTuneAudioProcessor& mProcessor;
    PlugTuneUI::PlugTuneLookFeel mLookAndFeel;

    // Top Bar Controls
    juce::ComboBox mGroupCombo;
    juce::ToggleButton mLiveModeToggle;
    juce::ComboBox mVocalRangeCombo;

    // Drag & Drop Banner Component
    juce::Label mDropZoneLabel;
    juce::TextButton mApplyKeyButton;
    bool mIsFileDragOver = false;
    juce::String mDetectedKeyText;
    int mDetectedRootKey = 0;
    int mDetectedScaleType = 0; // 0 = Minor (m)

    // Key & Scale Strip
    juce::ComboBox mRootKeyCombo;
    juce::ComboBox mScaleTypeCombo;
    juce::ToggleButton mToneToggle;

    // Interactive HeatMap Keyboard
    PlugTuneUI::HeatMapKeyboard mHeatMapKeyboard;

    // The 3 Clean Main Dials
    juce::Slider mTuneAmountSlider;
    juce::Slider mFormantSlider;
    juce::Slider mDoublerSlider;

    // Labels for Knobs
    juce::Label mTuneAmountLabel;
    juce::Label mFormantLabel;
    juce::Label mDoublerLabel;

    // APVTS Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   mTuneAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   mFormantAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   mDoublerAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mRootKeyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mScaleTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mVocalRangeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>   mLiveModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> mGroupAttachment;

    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::String& suffix = "");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugTuneAudioProcessorEditor)
};

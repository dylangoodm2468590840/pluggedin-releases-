#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/ShadowProcessor.h"
#include "DSP/CrushProcessor.h"
#include "DSP/WidthProcessor.h"
#include "DSP/SpaceProcessor.h"
#include "DSP/DeviceProcessor.h"
#include "DSP/SignalChain.h"
#include "Presets/PresetManager.h"
#include "Utils/ParameterIDs.h"
#include "Utils/PluggedINAutoUpdater.h"

class UndergroundAudioProcessor  : public juce::AudioProcessor
{
public:
    UndergroundAudioProcessor();
    ~UndergroundAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    SignalChain& getSignalChain() noexcept { return signalChain; }
    PluggedINAutoUpdater& getAutoUpdater() noexcept { return autoUpdater; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    SignalChain signalChain;
    PluggedINAutoUpdater autoUpdater;

    ShadowProcessor shadowProcessor;
    CrushProcessor crushProcessor;
    WidthProcessor widthProcessor;
    SpaceProcessor spaceProcessor;
    DeviceProcessor deviceProcessor;

    // Fast atomic parameter listeners
    std::atomic<float>* degenerateParam   { nullptr };
    std::atomic<float>* presetModeParam   { nullptr };
    std::atomic<float>* inputGainParam    { nullptr };
    std::atomic<float>* outputGainParam   { nullptr };
    std::atomic<float>* mixGlobalParam    { nullptr };

    // Macro Matrix params
    std::atomic<float>* macroDepthParam   { nullptr };
    std::atomic<float>* macroDarkParam    { nullptr };
    std::atomic<float>* macroMotionParam  { nullptr };
    std::atomic<float>* macroChaosParam   { nullptr };
    std::atomic<float>* macroAgeParam     { nullptr };
    std::atomic<float>* macroGhostParam   { nullptr };
    std::atomic<float>* macroToneParam    { nullptr };

    // Shadow Engine params
    std::atomic<float>* shadowEnableParam { nullptr };
    std::atomic<float>* shadowMixParam    { nullptr };
    std::atomic<float>* shadowPitchParam  { nullptr };
    std::atomic<float>* shadowFormantParam{ nullptr };
    std::atomic<float>* shadowDarkParam   { nullptr };
    std::atomic<float>* shadowDriveParam  { nullptr };

    // Module params
    std::atomic<float>* crushAmountParam  { nullptr };
    std::atomic<float>* crushCharParam    { nullptr };
    std::atomic<float>* crushToneParam    { nullptr };
    std::atomic<float>* crushMixParam     { nullptr };

    std::atomic<float>* widthAmountParam  { nullptr };
    std::atomic<float>* spaceReverbParam  { nullptr };
    std::atomic<float>* spaceDelayParam   { nullptr };
    std::atomic<float>* deviceTypeParam   { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndergroundAudioProcessor)
};

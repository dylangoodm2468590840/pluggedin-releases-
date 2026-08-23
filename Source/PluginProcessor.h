#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/ShadowProcessor.h"
#include "DSP/CrushProcessor.h"
#include "DSP/WidthProcessor.h"
#include "DSP/SpaceProcessor.h"
#include "DSP/DeviceProcessor.h"
#include "DSP/EQEngine.h"
#include "DSP/VocalCompressor.h"
#include "DSP/DeEsserProcessor.h"
#include "DSP/AirExciterProcessor.h"
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
    VocalCompressor& getVocalCompressor() noexcept { return vocalCompressor; }
    DeEsserProcessor& getDeEsser() noexcept { return deEsserProcessor; }
    AirExciterProcessor& getAirExciter() noexcept { return airExciterProcessor; }

    void toggleABState();
    int getActiveStateSlot() const noexcept { return activeStateSlot; }

private:
    int activeStateSlot { 0 }; // 0 = State A, 1 = State B
    std::unique_ptr<juce::XmlElement> stateSlotA;
    std::unique_ptr<juce::XmlElement> stateSlotB;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    SignalChain signalChain;
    PluggedINAutoUpdater autoUpdater;

    ShadowProcessor shadowProcessor;
    CrushProcessor crushProcessor;
    WidthProcessor widthProcessor;
    SpaceProcessor spaceProcessor;
    DeviceProcessor deviceProcessor;
    VocalCompressor vocalCompressor;
    DeEsserProcessor deEsserProcessor;
    AirExciterProcessor airExciterProcessor;

    // Fast atomic parameter listeners
    std::atomic<float>* degenerateParam   { nullptr };
    std::atomic<float>* presetModeParam   { nullptr };
    std::atomic<float>* inputGainParam    { nullptr };
    std::atomic<float>* outputGainParam   { nullptr };
    std::atomic<float>* mixGlobalParam    { nullptr };

    // Dynamics & Taming params
    std::atomic<float>* compSqueezeParam  { nullptr };
    std::atomic<float>* compCharParam     { nullptr };
    std::atomic<float>* deEssAmountParam  { nullptr };
    std::atomic<float>* deEssFreqParam    { nullptr };

    // Psychoacoustic Fresh Air params
    std::atomic<float>* airMidParam       { nullptr };
    std::atomic<float>* airTopParam       { nullptr };

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
    std::atomic<float>* crushPunishParam  { nullptr };

    std::atomic<float>* widthAmountParam  { nullptr };
    std::atomic<float>* modRateParam      { nullptr };
    std::atomic<float>* modDepthParam     { nullptr };

    std::atomic<float>* spaceReverbParam  { nullptr };
    std::atomic<float>* reverbDecayParam  { nullptr };
    std::atomic<float>* reverbMixParam    { nullptr };
    std::atomic<float>* spaceDelayParam   { nullptr };
    std::atomic<float>* delayFbParam      { nullptr };
    std::atomic<float>* delayMixParam     { nullptr };
    std::atomic<float>* spaceDuckingParam { nullptr };
    std::atomic<float>* deviceTypeParam   { nullptr };

    std::atomic<float>* masterBypassParam { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndergroundAudioProcessor)
};



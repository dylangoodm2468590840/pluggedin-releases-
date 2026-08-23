#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Engine/VoiceManager.h"
#include "DSP/StereoSaturator.h"
#include "DSP/LadderFilter.h"
#include "DSP/StudioDelay.h"
#include "DSP/StudioReverb.h"
#include "Presets/PresetManager.h"

namespace Plugged1
{

class Plugged1AudioProcessor : public juce::AudioProcessor
{
public:
    Plugged1AudioProcessor();
    ~Plugged1AudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    PresetManager& getPresetManager() { return *presetManager; }
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    // Visualizer Audio Buffer Access (FIFO for UI)
    static constexpr int visualizerBufferSize = 512;
    void getVisualizerData(float* dest, int numSamples) const;

    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateDSPFromParameters();

    juce::AudioProcessorValueTreeState apvts;
    std::unique_ptr<PresetManager> presetManager;

    VoiceManager voiceManager;
    StereoSaturator saturator;
    LadderFilter masterFilter;
    StudioDelay studioDelay;
    StudioReverb studioReverb;

    juce::dsp::Gain<float> masterGain;
    juce::dsp::Limiter<float> masterLimiter;

    // Visualizer ring buffer
    std::array<float, visualizerBufferSize> visualizerBuffer {};
    std::atomic<int> visualizerWritePos { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plugged1AudioProcessor)
};

} // namespace Plugged1

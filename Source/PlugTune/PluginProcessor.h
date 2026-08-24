#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/PitchDetector.h"
#include "DSP/PitchShifter.h"
#include "DSP/FormantFilter.h"
#include "DSP/ScaleQuantizer.h"
#include "DSP/KeyDetector.h"
#include "DSP/StereoDoubler.h"
#include "UI/ToneGenerator.h"
#include <atomic>

class PlugTuneAudioProcessor : public juce::AudioProcessor
{
public:
    PlugTuneAudioProcessor();
    ~PlugTuneAudioProcessor() override;

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

    juce::AudioProcessorValueTreeState& getAPVTS() { return mValueTreeState; }

    // Live Telemetry for UI Visuals
    float getLiveInputMidi() const   { return mLiveInputMidi.load(); }
    float getLiveTargetMidi() const  { return mLiveTargetMidi.load(); }
    float getLiveClarity() const     { return mLiveClarity.load(); }
    bool getLiveIsVoiced() const     { return mLiveIsVoiced.load(); }
    int getLiveDetectedNote() const  { return mLiveDetectedNote.load(); }

    PlugTuneDSP::ToneGenerator& getToneGenerator() { return mToneGenerator; }
    PlugTuneDSP::KeyDetector& getKeyDetector()     { return mKeyDetector; }
    PlugTuneDSP::ScaleQuantizer& getQuantizer()    { return mScaleQuantizer; }

private:
    juce::AudioProcessorValueTreeState mValueTreeState;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Sub-modules
    PlugTuneDSP::PitchDetector  mPitchDetector;
    PlugTuneDSP::PitchShifter   mPitchShifter;
    PlugTuneDSP::ScaleQuantizer mScaleQuantizer;
    PlugTuneDSP::StereoDoubler  mStereoDoubler;
    PlugTuneDSP::ToneGenerator  mToneGenerator;
    PlugTuneDSP::KeyDetector    mKeyDetector;

    // Live Telemetry Atomics
    std::atomic<float> mLiveInputMidi { 0.0f };
    std::atomic<float> mLiveTargetMidi { 0.0f };
    std::atomic<float> mLiveClarity { 0.0f };
    std::atomic<bool>  mLiveIsVoiced { false };
    std::atomic<int>   mLiveDetectedNote { 0 };
    std::atomic<bool>  mCurrentLiveMode { true };

    juce::AudioBuffer<float> mDryBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlugTuneAudioProcessor)
};

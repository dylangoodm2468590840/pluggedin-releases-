#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "signalsmith-stretch.h"
#include <vector>

namespace PlugTuneDSP
{

class PitchShifter
{
public:
    PitchShifter();
    ~PitchShifter() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void setPitchAndFormant(float pitchShiftSemitones, float formantSemitones, float detectedF0Hz, bool isVoiced, float tuneAmount);

    void process(juce::AudioBuffer<float>& buffer);

    void setLiveMode(bool isLive);
    bool isLiveMode() const { return mIsLiveMode; }

    int getLatencySamples() const { return mLatencySamples; }

private:
    void configureEngine();

    double mSampleRate = 44100.0;
    int mNumChannels = 2;
    int mLatencySamples = 0;
    int mInputLatency = 0;
    bool mIsLiveMode = true;

    signalsmith::stretch::SignalsmithStretch<float> mStretchEngine;
    juce::AudioBuffer<float> mScratchBuffer;

    std::vector<float*> mInputPointers;
    std::vector<float*> mOutputPointers;

    // Latency Alignment Ring Buffers
    std::vector<float> mPitchDelayRing;
    std::vector<float> mF0DelayRing;
    int mRingWritePos = 0;
    int mRingSize = 8192;

    float mTargetPitch = 0.0f;
    float mCurrentPitch = 0.0f;
    float mTargetF0 = 0.0f;
    float mTargetFormant = 0.0f;
    float mCurrentFormant = 0.0f;
    float mTuneAmount = 100.0f;
};

} // namespace PlugTuneDSP


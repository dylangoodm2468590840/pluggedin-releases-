#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

namespace PlugTuneDSP
{

class ToneGenerator
{
public:
    ToneGenerator();
    ~ToneGenerator() = default;

    void prepare(double sampleRate);
    void reset();

    void playNote(int note0to11, int octave = 4);
    void stop();

    void setEnabled(bool enabled);
    bool isEnabled() const { return mEnabled.load(); }

    void processAdding(juce::AudioBuffer<float>& buffer);

private:
    double mSampleRate = 44100.0;
    std::atomic<bool> mEnabled { false };
    std::atomic<float> mTargetFrequency { 440.0f };
    std::atomic<bool> mIsPlaying { false };

    float mPhase = 0.0f;
    float mPhaseIncrement = 0.0f;
    float mCurrentLevel = 0.0f;
    float mTargetLevel = 0.0f;
};

} // namespace PlugTuneDSP

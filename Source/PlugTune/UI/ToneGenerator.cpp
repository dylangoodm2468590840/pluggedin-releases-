#include "ToneGenerator.h"
#include <cmath>
#include <algorithm>

namespace PlugTuneDSP
{

ToneGenerator::ToneGenerator()
{
}

void ToneGenerator::prepare(double sampleRate)
{
    mSampleRate = (sampleRate > 8000.0) ? sampleRate : 44100.0;
    reset();
}

void ToneGenerator::reset()
{
    mPhase = 0.0f;
    mPhaseIncrement = 0.0f;
    mCurrentLevel = 0.0f;
    mTargetLevel = 0.0f;
    mIsPlaying = false;
}

void ToneGenerator::setEnabled(bool enabled)
{
    mEnabled = enabled;
    if (!enabled)
    {
        stop();
    }
}

void ToneGenerator::playNote(int note0to11, int octave)
{
    if (!mEnabled) return;

    int midiNote = 12 * (octave + 1) + (note0to11 % 12);
    float freq = 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
    mTargetFrequency = freq;
    mPhaseIncrement = (2.0f * 3.14159265358979323846f * freq) / static_cast<float>(mSampleRate);
    mTargetLevel = 0.20f; // Audition level
    mIsPlaying = true;
}

void ToneGenerator::stop()
{
    mTargetLevel = 0.0f;
}

void ToneGenerator::processAdding(juce::AudioBuffer<float>& buffer)
{
    if (!mEnabled.load() || (!mIsPlaying.load() && mCurrentLevel < 0.001f))
    {
        return;
    }

    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        mCurrentLevel += 0.005f * (mTargetLevel - mCurrentLevel);
        if (mCurrentLevel < 0.0001f && mTargetLevel <= 0.0f)
        {
            mIsPlaying = false;
            mCurrentLevel = 0.0f;
            break;
        }

        float sample = std::sin(mPhase) * mCurrentLevel;
        mPhase += mPhaseIncrement;
        if (mPhase >= 2.0f * 3.14159265358979323846f)
        {
            mPhase -= 2.0f * 3.14159265358979323846f;
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.addSample(ch, i, sample);
        }
    }
}

} // namespace PlugTuneDSP

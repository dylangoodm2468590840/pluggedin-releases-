#include "EQEngine.h"

EQEngine::EQEngine()
{
    for (int i = 0; i < maxBands; ++i)
    {
        bandConfigs[i].active = false;
    }
}

void EQEngine::prepare(double sampleRate, int maxBlockSize) noexcept
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    juce::ignoreUnused(maxBlockSize);

    for (int i = 0; i < maxBands; ++i)
    {
        biquadArray[i].prepare(currentSampleRate);
    }
}

void EQEngine::reset() noexcept
{
    for (int i = 0; i < maxBands; ++i)
    {
        biquadArray[i].reset();
    }
}

void EQEngine::updateBand(int bandIndex, TransposedDirectFormIIBiquad::FilterType type, double freqHz, double gainDb, double qFactor, bool active) noexcept
{
    if (bandIndex < 0 || bandIndex >= maxBands)
        return;

    bandConfigs[bandIndex].type = type;
    bandConfigs[bandIndex].freqHz = freqHz;
    bandConfigs[bandIndex].gainDb = gainDb;
    bandConfigs[bandIndex].qFactor = qFactor;
    bandConfigs[bandIndex].active = active;

    if (active)
    {
        biquadArray[bandIndex].updateCoefficients(type, freqHz, gainDb, qFactor);
    }
}

void EQEngine::processBlock(juce::AudioBuffer<float>& buffer) noexcept
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;

    float* leftData = buffer.getWritePointer(0);
    float* rightData = (numChannels > 1) ? buffer.getWritePointer(1) : leftData;

    for (int s = 0; s < numSamples; ++s)
    {
        float sampleL = leftData[s];
        float sampleR = rightData[s];

        for (int b = 0; b < maxBands; ++b)
        {
            if (bandConfigs[b].active)
            {
                biquadArray[b].processSample(sampleL, sampleR);
            }
        }

        leftData[s] = sampleL;
        if (numChannels > 1)
        {
            rightData[s] = sampleR;
        }
    }
}

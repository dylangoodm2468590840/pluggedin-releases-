#include "StereoDoubler.h"
#include <cmath>
#include <algorithm>

namespace PlugTuneDSP
{

StereoDoubler::StereoDoubler()
{
    mBufSize = 8192;
    mDelayBufferL.assign(mBufSize, 0.0f);
    mDelayBufferR.assign(mBufSize, 0.0f);
}

void StereoDoubler::prepare(double sampleRate)
{
    mSampleRate = (sampleRate > 8000.0) ? sampleRate : 44100.0;
    mBufSize = 8192;
    mDelayBufferL.assign(mBufSize, 0.0f);
    mDelayBufferR.assign(mBufSize, 0.0f);

    mDelaySamplesL = static_cast<int>(mSampleRate * 0.011); // 11ms
    mDelaySamplesR = static_cast<int>(mSampleRate * 0.017); // 17ms
    reset();
}

void StereoDoubler::reset()
{
    std::fill(mDelayBufferL.begin(), mDelayBufferL.end(), 0.0f);
    std::fill(mDelayBufferR.begin(), mDelayBufferR.end(), 0.0f);
    mWritePos = 0;
}

void StereoDoubler::setAmount(float amount0to1)
{
    mAmount = std::clamp(amount0to1, 0.0f, 1.0f);
}

void StereoDoubler::setWidth(float width0to2)
{
    mWidth = std::clamp(width0to2, 0.0f, 2.0f);
}

void StereoDoubler::process(juce::AudioBuffer<float>& buffer)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    if (numChannels < 2 || mAmount < 0.01f)
    {
        return; // Bypass
    }

    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        float inL = left[i];
        float inR = right[i];
        float mono = 0.5f * (inL + inR);

        mDelayBufferL[mWritePos] = mono;
        mDelayBufferR[mWritePos] = mono;

        int readL = (mWritePos - mDelaySamplesL + mBufSize) % mBufSize;
        int readR = (mWritePos - mDelaySamplesR + mBufSize) % mBufSize;

        float wetL = mDelayBufferL[readL];
        float wetR = mDelayBufferR[readR];

        // M/S Width processing
        float mid = mono;
        float side = (wetL - wetR) * mWidth;

        float finalL = inL * (1.0f - 0.4f * mAmount) + (mid + side) * (0.4f * mAmount);
        float finalR = inR * (1.0f - 0.4f * mAmount) + (mid - side) * (0.4f * mAmount);

        left[i] = finalL;
        right[i] = finalR;

        mWritePos = (mWritePos + 1) % mBufSize;
    }
}

} // namespace PlugTuneDSP

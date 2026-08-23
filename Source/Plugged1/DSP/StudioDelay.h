#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include <algorithm>

namespace Plugged1
{

class StudioDelay
{
public:
    StudioDelay() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
        int maxDelaySamples = static_cast<int>(sampleRate * 2.0); // 2 seconds max
        delayBufferL.resize(maxDelaySamples, 0.0f);
        delayBufferR.resize(maxDelaySamples, 0.0f);
        reset();
    }

    void reset()
    {
        std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
        std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
        writeIndex = 0;
        filteredFeedbackL = 0.0f;
        filteredFeedbackR = 0.0f;
    }

    void setParameters(float delayTimeMs, float newFeedback, float newMix, bool pingPong = true)
    {
        targetDelaySamples = std::clamp(static_cast<int>((delayTimeMs / 1000.0f) * sampleRate), 1, static_cast<int>(delayBufferL.size()) - 1);
        feedback = std::clamp(newFeedback, 0.0f, 0.95f);
        mix = std::clamp(newMix, 0.0f, 1.0f);
        isPingPong = pingPong;
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        if (mix < 0.001f || delayBufferL.empty())
            return;

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        const int bufSize = static_cast<int>(delayBufferL.size());

        float* left = buffer.getWritePointer(0);
        float* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

        const float dampCoeff = 0.35f; // High-cut damping in feedback path

        for (int i = 0; i < numSamples; ++i)
        {
            int readIndexL = (writeIndex - targetDelaySamples + bufSize) % bufSize;
            int readIndexR = isPingPong ? (writeIndex - (targetDelaySamples / 2) + bufSize) % bufSize : readIndexL;

            float delayedL = delayBufferL[readIndexL];
            float delayedR = delayBufferR[readIndexR];

            // 6dB/oct Low-pass filter for tape warmth
            filteredFeedbackL += (delayedL - filteredFeedbackL) * dampCoeff;
            filteredFeedbackR += (delayedR - filteredFeedbackR) * dampCoeff;

            float inL = left[i];
            float inR = right != nullptr ? right[i] : inL;

            if (isPingPong)
            {
                delayBufferL[writeIndex] = inL + filteredFeedbackR * feedback;
                delayBufferR[writeIndex] = inR + filteredFeedbackL * feedback;
            }
            else
            {
                delayBufferL[writeIndex] = inL + filteredFeedbackL * feedback;
                delayBufferR[writeIndex] = inR + filteredFeedbackR * feedback;
            }

            left[i] = inL * (1.0f - mix) + delayedL * mix;
            if (right != nullptr)
                right[i] = inR * (1.0f - mix) + delayedR * mix;

            writeIndex = (writeIndex + 1) % bufSize;
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int writeIndex = 0;
    int targetDelaySamples = 44100 / 2;
    float feedback = 0.4f;
    float mix = 0.0f;
    bool isPingPong = true;
    float filteredFeedbackL = 0.0f;
    float filteredFeedbackR = 0.0f;
};

} // namespace Plugged1

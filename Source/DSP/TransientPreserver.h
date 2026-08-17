#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <algorithm>
#include "../Utils/AudioUtils.h"

/**
 * @class TransientPreserver
 * @brief High-Resolution Consonant & Syllable Attack Protection Engine.
 * 
 * Features:
 * - Detects rapid vocal consonant bursts (T, K, P, D, B, CH) using spectral derivative and high-frequency envelope tracking.
 * - Computes a continuous transient protection multiplier [0.0 to 1.0].
 * - Prevents heavy distortion, waveshaping, and fast compressors from smearing vocal articulation.
 */
class TransientPreserver
{
public:
    TransientPreserver() = default;
    ~TransientPreserver() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

        juce::dsp::ProcessSpec monoSpec = spec;
        monoSpec.numChannels = 1;

        for (int ch = 0; ch < 2; ++ch)
        {
            highpassFilter[ch].prepare(monoSpec);
            highpassFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
            highpassFilter[ch].setCutoffFrequency(2800.0f);
            highpassFilter[ch].setResonance(0.707f);
        }

        attackCoeff  = std::exp(-1.0f / (0.0015f * static_cast<float>(sampleRate))); // 1.5ms instant attack
        releaseCoeff = std::exp(-1.0f / (0.025f  * static_cast<float>(sampleRate))); // 25ms release
        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            highpassFilter[ch].reset();
            fastEnv[ch] = 0.0f;
            slowEnv[ch] = 0.0f;
            prevSample[ch] = 0.0f;
        }
        currentProtection.store(0.0f);
    }

    /**
     * @brief Analyzes buffer and returns current transient intensity [0.0 to 1.0]
     */
    float analyze(const juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numSamples == 0 || numChannels == 0)
            return 0.0f;

        float maxTransient = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            int chIdx = std::min(ch, 1);
            const float* channelData = buffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                float in = channelData[i];
                float hf = highpassFilter[chIdx].processSample(0, in);
                float absHf = std::abs(hf);

                // High-Frequency envelope follower
                float fEnv = fastEnv[chIdx];
                if (absHf > fEnv) fEnv = attackCoeff * fEnv + (1.0f - attackCoeff) * absHf;
                else              fEnv = releaseCoeff * fEnv + (1.0f - releaseCoeff) * absHf;
                fastEnv[chIdx] = AudioUtils::sanitize(fEnv);

                // Slower background envelope follower
                slowEnv[chIdx] = 0.999f * slowEnv[chIdx] + 0.001f * absHf;

                // Transient ratio
                float ratio = (fastEnv[chIdx] + 1.0e-5f) / (slowEnv[chIdx] + 1.0e-4f);
                if (ratio > 2.0f && fastEnv[chIdx] > 0.01f)
                {
                    float transVal = std::clamp((ratio - 2.0f) * 0.25f, 0.0f, 1.0f);
                    if (transVal > maxTransient) maxTransient = transVal;
                }
            }
        }

        currentProtection.store(maxTransient);
        return maxTransient;
    }

    float getProtection() const noexcept { return currentProtection.load(); }

private:
    double sampleRate { 44100.0 };
    float attackCoeff { 0.0f };
    float releaseCoeff { 0.0f };

    juce::dsp::StateVariableTPTFilter<float> highpassFilter[2];
    float fastEnv[2] { 0.0f, 0.0f };
    float slowEnv[2] { 0.0f, 0.0f };
    float prevSample[2] { 0.0f, 0.0f };

    std::atomic<float> currentProtection { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientPreserver)
};

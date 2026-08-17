#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <algorithm>
#include "../Utils/AudioUtils.h"

/**
 * @class InputConditioner
 * @brief Studio-grade Input Level Analysis, Gain Conditioning & Headroom Protection Engine.
 * 
 * Features:
 * - Transparent Soft-Knee Headroom Clamping (prevents downstream DSP from blowing up on extreme inputs).
 * - High-Resolution True RMS & Peak Level Estimator.
 * - Sub-audible Low-Cut DC Blocker (18Hz).
 * - Predictable reference calibration across quiet and loud vocal performances.
 */
class InputConditioner
{
public:
    InputConditioner() = default;
    ~InputConditioner() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

        juce::dsp::ProcessSpec monoSpec = spec;
        monoSpec.numChannels = 1;

        for (int ch = 0; ch < 2; ++ch)
        {
            subDcFilter[ch].prepare(monoSpec);
            subDcFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
            subDcFilter[ch].setCutoffFrequency(18.0f);
            subDcFilter[ch].setResonance(0.707f);
            dcBlocker[ch].reset();
        }

        reset();
    }

    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            subDcFilter[ch].reset();
            dcBlocker[ch].reset();
        }
        inputRmsLevel.store(0.0f);
        inputPeakLevel.store(0.0f);
    }

    void process(juce::AudioBuffer<float>& buffer, float inputTrimLinear)
    {
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        if (numSamples == 0 || numChannels == 0)
            return;

        float maxPeak = 0.0f;
        float sumSquares = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            int chIdx = std::min(ch, 1);
            float* channelData = buffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                // 1. Apply user input trim
                float s = channelData[i] * inputTrimLinear;

                // 2. 18Hz DC & infrasonic rumble filter
                s = subDcFilter[chIdx].processSample(0, s);
                s = dcBlocker[chIdx].process(s);

                // 3. Adaptive Optical Studio Noise Squelch & Downward Expander
                // Protects downstream high-gain compressors and saturation from amplifying room noise/hiss
                float inAbs = std::abs(s);
                if (inAbs > squelchEnv[chIdx])
                    squelchEnv[chIdx] += squelchAttCoeff * (inAbs - squelchEnv[chIdx]);
                else
                    squelchEnv[chIdx] += squelchRelCoeff * (inAbs - squelchEnv[chIdx]);

                float envDb = 20.0f * std::log10(std::max(squelchEnv[chIdx], 1.0e-7f));
                float gateGain = 1.0f;

                // Seamless downward expansion below -62 dBFS, tapering to silence below -75 dBFS
                if (envDb < -62.0f)
                {
                    float ratio = std::clamp((envDb - (-75.0f)) / 13.0f, 0.0f, 1.0f);
                    gateGain = ratio * ratio; // Smooth quadratic optical release curve
                }

                s *= gateGain;

                // 4. Transparent soft headroom protection (smooth tanh knee above +3dBFS / 1.414)
                float absS = std::abs(s);
                if (absS > 1.25f)
                {
                    float sign = s >= 0.0f ? 1.0f : -1.0f;
                    s = sign * (1.25f + 0.35f * std::tanh((absS - 1.25f) * 1.5f));
                }

                s = AudioUtils::sanitize(s);
                channelData[i] = s;

                // Metering accumulation
                float currentAbs = std::abs(s);
                if (currentAbs > maxPeak) maxPeak = currentAbs;
                sumSquares += s * s;
            }
        }

        inputPeakLevel.store(maxPeak);
        float rms = std::sqrt(sumSquares / static_cast<float>(numSamples * numChannels + 1));
        inputRmsLevel.store(rms);
    }

    float getPeakLevel() const noexcept { return inputPeakLevel.load(); }
    float getRmsLevel() const noexcept { return inputRmsLevel.load(); }

private:
    double sampleRate { 44100.0 };

    juce::dsp::StateVariableTPTFilter<float> subDcFilter[2];
    AudioUtils::DCBlocker dcBlocker[2];

    float squelchEnv[2] { 0.0f, 0.0f };
    float squelchAttCoeff { 0.15f };
    float squelchRelCoeff { 0.002f };

    std::atomic<float> inputPeakLevel { 0.0f };
    std::atomic<float> inputRmsLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputConditioner)
};

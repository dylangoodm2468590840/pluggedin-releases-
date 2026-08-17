#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <algorithm>
#include "../Utils/AudioUtils.h"

/**
 * @class AutoLevelCompensator
 * @brief Real-Time Perceived Loudness Matching & Level Compensation Engine.
 * 
 * Features:
 * - Compares pre-FX dry RMS/loudness with post-FX wet loudness.
 * - Dynamically computes a transparent compensation gain trim to prevent loudness bias ("louder sounds better").
 * - Smooth, musical time constants (300ms integration window) preventing audible pumping.
 * - Maximum compensation bounds (±12 dB) with anti-runaway safety.
 */
class AutoLevelCompensator
{
public:
    AutoLevelCompensator() = default;
    ~AutoLevelCompensator() = default;

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
        timeCoeff = std::exp(-1.0f / (0.30f * static_cast<float>(sampleRate))); // 300ms window
        reset();
    }

    void reset()
    {
        dryRms = 0.0f;
        wetRms = 0.0f;
        smoothedGainTrim.reset(sampleRate, 0.05);
        smoothedGainTrim.setCurrentAndTargetValue(1.0f);
        currentCompensationDb.store(0.0f);
    }

    void setEnabled(bool shouldBeEnabled) noexcept { enabled.store(shouldBeEnabled); }
    bool isEnabled() const noexcept { return enabled.load(); }

    void process(const juce::AudioBuffer<float>& dryBuf, juce::AudioBuffer<float>& wetBuf)
    {
        const int numSamples = wetBuf.getNumSamples();
        const int numChannels = wetBuf.getNumChannels();

        if (numSamples == 0 || numChannels == 0)
            return;

        if (!enabled.load())
        {
            currentCompensationDb.store(0.0f);
            return;
        }

        // Calculate dry and wet RMS accumulation
        float drySum = 0.0f;
        float wetSum = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* dryData = dryBuf.getReadPointer(ch);
            const float* wetData = wetBuf.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                drySum += dryData[i] * dryData[i];
                wetSum += wetData[i] * wetData[i];
            }
        }

        float blockDryRms = std::sqrt(drySum / static_cast<float>(numSamples * numChannels + 1));
        float blockWetRms = std::sqrt(wetSum / static_cast<float>(numSamples * numChannels + 1));

        // Smooth energy followers
        dryRms = timeCoeff * dryRms + (1.0f - timeCoeff) * blockDryRms;
        wetRms = timeCoeff * wetRms + (1.0f - timeCoeff) * blockWetRms;

        // Compute compensation factor only during active audio
        float targetGain = 1.0f;
        if (dryRms > 0.005f && wetRms > 0.005f)
        {
            float ratio = (dryRms + 1.0e-4f) / (wetRms + 1.0e-4f);
            // Clamp compensation between -12dB (0.25x) and +6dB (2.0x)
            targetGain = std::clamp(ratio, 0.25f, 2.0f);
        }

        smoothedGainTrim.setTargetValue(targetGain);

        for (int i = 0; i < numSamples; ++i)
        {
            float g = smoothedGainTrim.getNextValue();
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* wetData = wetBuf.getWritePointer(ch);
                wetData[i] = AudioUtils::sanitize(wetData[i] * g);
            }
        }

        float compDb = juce::Decibels::gainToDecibels(smoothedGainTrim.getCurrentValue());
        currentCompensationDb.store(compDb);
    }

    float getCompensationDb() const noexcept { return currentCompensationDb.load(); }

private:
    double sampleRate { 44100.0 };
    float timeCoeff { 0.0f };

    std::atomic<bool> enabled { true };

    float dryRms { 0.0f };
    float wetRms { 0.0f };

    juce::SmoothedValue<float> smoothedGainTrim;
    std::atomic<float> currentCompensationDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoLevelCompensator)
};

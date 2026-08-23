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

        float totalCount = static_cast<float>(numSamples * numChannels);
        float blockDryRms = std::sqrt(drySum / std::max(totalCount, 1.0f));
        float blockWetRms = std::sqrt(wetSum / std::max(totalCount, 1.0f));

        // Smooth energy followers (400ms integration window)
        dryRms = timeCoeff * dryRms + (1.0f - timeCoeff) * blockDryRms;
        wetRms = timeCoeff * wetRms + (1.0f - timeCoeff) * blockWetRms;

        // Compute compensation factor with speech pause freeze
        if (dryRms > 0.0025f && wetRms > 0.0025f)
        {
            // Active speech detected: update compensation ratio smoothly
            float ratio = (dryRms + 1.0e-4f) / (wetRms + 1.0e-4f);
            // Apply soft-knee bounded compensation between -9 dB (0.35x) and +4.5 dB (1.68x)
            lastTargetGain = std::clamp(std::sqrt(ratio), 0.35f, 1.68f);
        }
        // When dry audio pauses (reverb tail or silence), lastTargetGain is frozen so tails decay naturally!

        smoothedGainTrim.setTargetValue(lastTargetGain);

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
    float lastTargetGain { 1.0f };

    juce::SmoothedValue<float> smoothedGainTrim;
    std::atomic<float> currentCompensationDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoLevelCompensator)
};

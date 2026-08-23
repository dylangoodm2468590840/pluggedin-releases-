#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include "../Utils/AudioUtils.h"

/**
 * @class VocalResonanceProcessor
 * @brief Studio-Grade Dynamic Spectral Resonance & Harshness Suppressor (Soothe2 Caliber).
 * 
 * Features:
 * - 4 Critical Dynamic Vocal Zones:
 *     Band 0: 320 Hz (Boxy Mud & Low-Mid Buildup)
 *     Band 1: 1.2 kHz (Nasal Honk / Box Resonances)
 *     Band 2: 3.4 kHz (Ear-Fatiguing Harshness / Consonant Spikes)
 *     Band 3: 7.2 kHz (Sibilant Whistle & Crystalline Edge)
 * - True Minimum-Phase Dynamic Bell Filters (Zero comb-filtering or phase smearing)
 * - Narrow-to-Broadband Energy Tracking with Fast 2ms Attack / Program Release
 * - Real-time thread-safe reduction metrics for visual meters
 */
class VocalResonanceProcessor
{
public:
    static constexpr int NUM_BANDS = 4;

    VocalResonanceProcessor();
    ~VocalResonanceProcessor() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setAmount(float newAmount) noexcept { amount.store(std::clamp(newAmount, 0.0f, 1.0f)); }
    float getAmount() const noexcept { return amount.load(); }

    void setFocus(float newFocus) noexcept { focus.store(std::clamp(newFocus, 0.0f, 1.0f)); }

    void setBandActive(int bandIndex, bool active) noexcept
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
            bandsActive[bandIndex].store(active);
    }

    void process(juce::AudioBuffer<float>& buffer);

    float getTotalReductionDb() const noexcept { return maxReductionDb.load(); }
    
    float getBandReductionDb(int bandIndex) const noexcept
    {
        if (bandIndex >= 0 && bandIndex < NUM_BANDS)
            return bandReductionDb[bandIndex].load();
        return 0.0f;
    }

private:
    double sampleRate { 44100.0 };

    std::atomic<float> amount { 0.5f };
    std::atomic<float> focus { 0.5f };
    std::array<std::atomic<bool>, NUM_BANDS> bandsActive { true, true, true, true };
    std::array<std::atomic<float>, NUM_BANDS> bandReductionDb { 0.0f, 0.0f, 0.0f, 0.0f };

    juce::SmoothedValue<float> smoothedAmount;

    // Minimum-Phase Transposed Direct-Form II Biquad Bell Filter for each channel and band
    struct DynamicFilterNode
    {
        float b0 { 1.0f }, b1 { 0.0f }, b2 { 0.0f };
        float a1 { 0.0f }, a2 { 0.0f };
        float s1 { 0.0f }, s2 { 0.0f };

        inline void reset() noexcept { s1 = 0.0f; s2 = 0.0f; }

        inline float process(float x) noexcept
        {
            float y = b0 * x + s1;
            s1 = b1 * x - a1 * y + s2;
            s2 = b2 * x - a2 * y;
            return y;
        }

        void updateBellCoeffs(double sr, double freqHz, double gainDb, double q) noexcept
        {
            double A = std::pow(10.0, gainDb / 40.0);
            double w0 = 2.0 * 3.14159265358979323846 * std::clamp(freqHz, 20.0, sr * 0.48) / sr;
            double alpha = std::sin(w0) / (2.0 * std::max(0.1, q));
            double cosw0 = std::cos(w0);

            double a0 = 1.0 + alpha / A;
            b0 = static_cast<float>((1.0 + alpha * A) / a0);
            b1 = static_cast<float>((-2.0 * cosw0) / a0);
            b2 = static_cast<float>((1.0 - alpha * A) / a0);
            a1 = static_cast<float>((-2.0 * cosw0) / a0);
            a2 = static_cast<float>((1.0 - alpha / A) / a0);
        }
    };

    struct DynamicBandState
    {
        float centerFreq { 1000.0f };
        float qFactor { 2.2f };
        float attackCoeff { 0.0f };
        float releaseCoeff { 0.0f };

        juce::dsp::StateVariableTPTFilter<float> detectorBp[2];
        DynamicFilterNode dynamicCutBiquad[2];

        float detectorEnv[2] { 0.0f, 0.0f };
        float broadEnv[2] { 0.0f, 0.0f };
        float currentGainDb[2] { 0.0f, 0.0f };
    };

    std::array<DynamicBandState, NUM_BANDS> bands;
    std::atomic<float> maxReductionDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalResonanceProcessor)
};

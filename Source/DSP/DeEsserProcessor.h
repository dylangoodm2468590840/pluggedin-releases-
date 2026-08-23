#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "../Utils/AudioUtils.h"

/**
 * @class DeEsserProcessor
 * @brief Dynamic Split-Band Vocal De-Esser (FabFilter Pro-DS / Waves Silk style).
 * 
 * Features:
 * - Linkwitz-Riley crossover splitting vocal into Body (Low) and Sibilance (High).
 * - Ultra-fast sidechain peak detector (1ms attack, 35ms release) detecting harsh 'S', 'T', 'CH' frequencies.
 * - Dynamic split-band gain reduction: reduces only harsh sibilance without dulling body or air.
 * - Real-time atomic gain-reduction metering.
 * - Zero memory allocations in audio thread, denormal & NaN safe.
 */
class DeEsserProcessor
{
public:
    DeEsserProcessor();
    ~DeEsserProcessor() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    /**
     * @brief Set de-essing intensity [0.0 to 1.0].
     */
    void setAmount(float newAmount) noexcept;

    /**
     * @brief Set crossover target frequency in Hz [4000.0 to 10000.0].
     */
    void setFrequency(float newFreqHz) noexcept;

    void process(juce::AudioBuffer<float>& buffer);

    float getGainReductionDb() const noexcept { return currentGainReductionDb.load(); }

private:
    double sampleRate { 44100.0 };

    std::atomic<float> amount { 0.0f };
    std::atomic<float> frequency { 6500.0f };

    juce::SmoothedValue<float> smoothedThresholdDb;
    juce::SmoothedValue<float> smoothedFreq;

    // Zero-Delay Feedback Dynamic Bell Filters & Sidechain detectors
    juce::dsp::StateVariableTPTFilter<float> dynamicBellFilter[2];
    juce::dsp::StateVariableTPTFilter<float> sidechainDetectorFilter[2];

    // Linked envelope followers
    float sibilanceEnv[2] { 0.0f, 0.0f };

    std::atomic<float> currentGainReductionDb { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeEsserProcessor)
};

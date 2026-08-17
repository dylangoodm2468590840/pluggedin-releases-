#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "../Utils/AudioUtils.h"

/**
 * @class VocalCompressor
 * @brief Commercial-grade Dual-Stage Vocal Dynamics & Leveling Engine (Waves R-Vox / CLA-76 / LA-2A Hybrid).
 * 
 * Features:
 * - Stage 1: Ultra-fast FET Peak Tamer (catches spiky plosives and sharp consonants).
 * - Stage 2: Program-Dependent Optical RMS Leveler (smooth optical knee & auto-release).
 * - Intelligent Auto-Makeup Gain (maintains constant perceived loudness across squeeze levels).
 * - Real-Time Atomic Gain-Reduction Metering.
 * - Zero-allocation, real-time audio thread safety with denormal protection.
 */
class VocalCompressor
{
public:
    VocalCompressor();
    ~VocalCompressor() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    /**
     * @brief Sets the Squeeze amount [0.0 to 1.0].
     * 0.0 = completely bypass / 0dB compression.
     * 1.0 = heavy commercial in-your-face vocal clamping.
     */
    void setSqueeze(float newSqueeze) noexcept;

    /**
     * @brief Sets the compression character:
     * 0 = Modern Fast (FET 1176 style)
     * 1 = Vintage Optical (Smooth LA-2A style)
     * 2 = Punchy Vocal (Dual-Stage Blend)
     */
    void setCharacter(int newChar) noexcept { compCharacter.store(newChar); }

    void process(juce::AudioBuffer<float>& buffer);

    /**
     * @brief Returns current gain reduction in dB for UI meters (always positive or zero).
     */
    float getGainReductionDb() const noexcept { return currentGainReductionDb.load(); }

private:
    double sampleRate { 44100.0 };

    std::atomic<float> squeezeAmount { 0.0f };
    std::atomic<int> compCharacter { 2 }; // Default: Punchy Vocal Blend

    juce::SmoothedValue<float> smoothedThresholdDb;
    juce::SmoothedValue<float> smoothedMakeupGain;

    // Envelope state per channel for dual-stage detection
    float envFast[2] { 0.0f, 0.0f };
    float envSlow[2] { 0.0f, 0.0f };

    std::atomic<float> currentGainReductionDb { 0.0f };

    // DC Blocker per channel
    AudioUtils::DCBlocker dcBlocker[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressor)
};

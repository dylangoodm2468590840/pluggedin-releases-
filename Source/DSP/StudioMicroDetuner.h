#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>
#include <cmath>
#include <algorithm>
#include "../Utils/AudioUtils.h"

/**
 * @class StudioMicroDetuner
 * @brief Studio-Grade Dual Micro-Pitch Detune & 3D Spatializer (Soundtoys MicroShift / Eventide H910 caliber).
 * 
 * Features:
 * - Dual-Head Asymmetric Micro-Pitch Shifting (Left +9 cents @ 18ms, Right -9 cents @ 26ms).
 * - 3rd-Order Cubic Hermite Fractional Interpolation.
 * - Sub-150Hz Mono Anchor (keeps vocal bass punch 100% focused in mono with zero phase cancellation).
 * - Creates wide, expensive 3D vocal aura while keeping center vocal forward in the mix.
 */
class StudioMicroDetuner
{
public:
    StudioMicroDetuner();
    ~StudioMicroDetuner() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setAmount(float newAmount) noexcept { amountSmoother.setTargetValue(std::clamp(newAmount, 0.0f, 1.0f)); }
    float getAmount() const noexcept { return amountSmoother.getTargetValue(); }

    void setDetuneCents(float cents) noexcept { detuneCents.store(std::clamp(cents, 1.0f, 25.0f)); }
    void setMix(float newMix) noexcept { mixSmoother.setTargetValue(std::clamp(newMix, 0.0f, 1.0f)); }

    void process(juce::AudioBuffer<float>& buffer);

private:
    inline float readHermite(const std::vector<float>& buf, float readPos, int mask) noexcept
    {
        while (readPos < 0.0f) readPos += static_cast<float>(mask + 1);

        int i1 = static_cast<int>(readPos) & mask;
        int i0 = (i1 - 1) & mask;
        int i2 = (i1 + 1) & mask;
        int i3 = (i1 + 2) & mask;

        float frac = readPos - std::floor(readPos);

        float y0 = buf[i0];
        float y1 = buf[i1];
        float y2 = buf[i2];
        float y3 = buf[i3];

        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    double sampleRate { 44100.0 };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> amountSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoother;
    std::atomic<float> detuneCents { 9.0f };

    static constexpr int BUFFER_SIZE = 8192;
    static constexpr int BUFFER_MASK = BUFFER_SIZE - 1;

    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int writePos { 0 };

    // Dual grain phases per channel for seamless pitch-shifting crossfade
    float grainPhaseL[2] { 0.0f, 0.5f };
    float grainPhaseR[2] { 0.0f, 0.5f };

    // Sub-150Hz Mono Anchor crossover filter
    juce::dsp::StateVariableTPTFilter<float> monoAnchorFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StudioMicroDetuner)
};

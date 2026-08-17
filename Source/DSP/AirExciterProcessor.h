#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "../Utils/AudioUtils.h"

/**
 * @class AirExciterProcessor
 * @brief Commercial Psychoacoustic Dual-Band Vocal Exciter (Slate Fresh Air / Dolby 361-A Trick).
 * 
 * Features:
 * - Mid-Air Band (3.5 kHz - 8.0 kHz): Injects vocal clarity, presence, and articulation.
 * - Top-Air Band (8.5 kHz - 22.0 kHz): Injects silky breath texture and "expensive studio mic" sheen.
 * - Dynamic Upward Expansion & Harmonic Generation (lifts subtle high-frequency details without static EQ harshness).
 * - Zero memory allocations in audio thread, denormal & NaN safe.
 */
class AirExciterProcessor
{
public:
    AirExciterProcessor();
    ~AirExciterProcessor() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setMidAir(float newMidAir) noexcept { midAirSmoother.setTargetValue(std::clamp(newMidAir, 0.0f, 1.0f)); }
    float getMidAir() const noexcept { return midAirSmoother.getTargetValue(); }

    void setTopAir(float newTopAir) noexcept { topAirSmoother.setTargetValue(std::clamp(newTopAir, 0.0f, 1.0f)); }
    float getTopAir() const noexcept { return topAirSmoother.getTargetValue(); }

    void process(juce::AudioBuffer<float>& buffer);

private:
    float processBandExciter(float input, float amount, float& envState, float attCoeff, float relCoeff);

    double sampleRate { 44100.0 };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> midAirSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> topAirSmoother;

    // Crossover / Bandpass filters per channel
    juce::dsp::StateVariableTPTFilter<float> midAirFilter[2];
    juce::dsp::StateVariableTPTFilter<float> topAirFilter[2];

    // Envelope followers per channel for dynamic upward expansion
    float midEnv[2] { 0.0f, 0.0f };
    float topEnv[2] { 0.0f, 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AirExciterProcessor)
};

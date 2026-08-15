#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../Utils/AudioUtils.h"
#include <vector>
#include <cmath>

/**
 * @class WidthProcessor
 * @brief Studio-Grade 4-Tap Analog BBD Multi-Voice Chorus & Flanger DSP Engine.
 * Modeled after FL Studio's Fruity Flangus, Vintage Chorus & Hyper Chorus.
 */
class WidthProcessor
{
public:
    enum class ModulationMode
    {
        DimensionalChorus = 0,  // Roland Dimension D / Vintage Chorus (Smooth, 4 taps, 0.5Hz)
        AnalogFlangus     = 1,  // Fruity Flangus / Resonant Flanger (Short delays, harmonic feedback)
        StereoDoubler     = 2,  // 20ms Haas Doubler + subtle micro-pitch
        HyperEnsemble     = 3   // 8-phase wide vocal wash
    };

    WidthProcessor();
    ~WidthProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setAmount(float newAmount) noexcept { amount = std::clamp(newAmount, 0.0f, 1.0f); }
    float getAmount() const noexcept { return amount; }

    void setDetune(float newDetune) noexcept { detune = std::clamp(newDetune, 0.0f, 1.0f); }
    void setMix(float newMix) noexcept { mix = std::clamp(newMix, 0.0f, 1.0f); }
    void setMode(ModulationMode newMode) noexcept { mode = newMode; }

    void process(const juce::dsp::ProcessContextReplacing<float>& context);

private:
    inline float readCubicHermite(const std::vector<float>& buffer, float delaySamples, int writePos, int mask) noexcept
    {
        float readPos = static_cast<float>(writePos) - delaySamples;
        while (readPos < 0.0f) readPos += static_cast<float>(mask + 1);

        int i1 = static_cast<int>(readPos) & mask;
        int i0 = (i1 - 1) & mask;
        int i2 = (i1 + 1) & mask;
        int i3 = (i1 + 2) & mask;

        float frac = readPos - std::floor(readPos);

        float y0 = buffer[i0];
        float y1 = buffer[i1];
        float y2 = buffer[i2];
        float y3 = buffer[i3];

        // 4-point, 3rd-order Hermite interpolation spline
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    float amount { 0.5f };
    float detune { 0.3f };
    float mix { 1.0f };
    ModulationMode mode { ModulationMode::DimensionalChorus };

    double sampleRate { 44100.0 };

    // 4096-sample power-of-two circular delay buffers for L & R
    static constexpr int BUFFER_SIZE = 4096;
    static constexpr int BUFFER_MASK = BUFFER_SIZE - 1;

    std::vector<float> delayBufferL;
    std::vector<float> delayBufferR;
    int writeIndex { 0 };

    // 4 Poly-Phase LFO oscillators
    float lfoPhase1 { 0.0f };  // 0 deg
    float lfoPhase2 { 0.25f }; // 90 deg
    float lfoPhase3 { 0.50f }; // 180 deg
    float lfoPhase4 { 0.75f }; // 270 deg

    // Feedback memories for flanger resonant circulation
    float feedbackL { 0.0f };
    float feedbackR { 0.0f };

    // 2-Pole Analog BBD Warmth Low-Pass Filter (7.5kHz)
    juce::dsp::StateVariableTPTFilter<float> bbdWarmthFilterL;
    juce::dsp::StateVariableTPTFilter<float> bbdWarmthFilterR;

    // Sub-120Hz Bass Mono-Maker Crossover Filter
    juce::dsp::StateVariableTPTFilter<float> bassMonoFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WidthProcessor)
};

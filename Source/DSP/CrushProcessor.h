#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../Utils/AudioUtils.h"

/**
 * @class CrushProcessor
 * @brief Commercial-grade Analog Vocal Saturation & Dynamic Exciter Engine.
 * Features:
 * - 12AX7 Triode Tube with Asymmetric 2nd & 3rd Order Harmonics
 * - Magnetic Tape Saturation with 60Hz Head-Bump Resonance & Hysteresis
 * - Germanium Diode Fuzz with Parallel Dry Body Preservation
 * - Anti-Aliased Vintage Decimator & Downsampler
 * - Dynamic High-Frequency Vocal Air Exciter (Slate Fresh Air style)
 * - 2x Oversampling with Polyphase IIR Reconstruction
 */
class CrushProcessor
{
public:
    enum class Character
    {
        SoftClip = 0,
        Bitcrusher = 1,
        Overdrive = 2,
        ParallelFuzz = 3
    };

    CrushProcessor();
    ~CrushProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setAmount(float newAmount) noexcept { amountSmoother.setTargetValue(std::clamp(newAmount, 0.0f, 1.0f)); }
    float getAmount() const noexcept { return amountSmoother.getTargetValue(); }
    void setCharacter(Character newChar) noexcept { character.store(newChar); }
    void setTone(float newTone) noexcept { toneSmoother.setTargetValue(std::clamp(newTone, 0.0f, 1.0f)); }
    void setMix(float newMix) noexcept { mixSmoother.setTargetValue(std::clamp(newMix, 0.0f, 1.0f)); }
    void setOversamplingEnabled(bool enabled) noexcept { oversamplingEnabled.store(enabled); }

    void process(juce::AudioBuffer<float>& buffer);

private:
    float processSample(float inputSample, int channel, float currentAmount, float currentTone);

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> amountSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> toneSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoother;

    std::atomic<Character> character { Character::SoftClip };
    std::atomic<bool> oversamplingEnabled { true };

    double sampleRate { 44100.0 };

    // Bitcrusher downsampler state per channel
    float holdSample[2] { 0.0f, 0.0f };
    float sampleCounter[2] { 0.0f, 0.0f };

    // Pre-emphasis & De-emphasis analog filters
    juce::dsp::StateVariableTPTFilter<float> preEmphasisFilter[2];
    juce::dsp::StateVariableTPTFilter<float> deEmphasisFilter[2];

    // Tape Head-Bump Filter (60Hz resonant bell)
    juce::dsp::StateVariableTPTFilter<float> tapeHeadBumpFilter[2];

    // Air Exciter Highpass
    juce::dsp::StateVariableTPTFilter<float> airExciterFilter[2];

    // Post Tone filters per channel
    juce::dsp::StateVariableTPTFilter<float> lowpassFilter[2];
    juce::dsp::StateVariableTPTFilter<float> highpassFilter[2];

    // Oversampling (2x)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrushProcessor)
};

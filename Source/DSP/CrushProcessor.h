#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "../Utils/AudioUtils.h"

/**
 * @class CrushProcessor
 * @brief Commercial-grade 5-Circuit Analog Saturation Suite with PUNISH Mode & 4x Polyphase Oversampling.
 * 
 * Features:
 * - Frequency-Aware Clean Low-End Split (<150Hz) eliminates intermodulation mud.
 * - Dynamic Consonant & Transient Attack Protection preserves vocal articulation.
 * - 5 Topologies: 12AX7 Tube, EL34 Pentode, Ampex Tape, Germanium, Cyber Fuzz.
 * - PUNISH Mode (+20dB analog input blast with automatic compensation).
 * - 4x Polyphase Minimum-Phase Oversampling.
 */
class CrushProcessor
{
public:
    enum class Character
    {
        Tube12AX7   = 0,  // Class-A Triode Tube (Warm 2nd harmonics & bias sag)
        PentodeEL34 = 1,  // Push-Pull Pentode Tube (3rd/5th harmonics, biting edge)
        TapeAmpex   = 2,  // Magnetic Tape Saturation (60Hz head bump + hysteresis)
        Germanium   = 3,  // Vintage Console Germanium Transistor
        CyberFuzz   = 4   // Rectified Parallel Octave Fuzz
    };

    CrushProcessor();
    ~CrushProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setAmount(float newAmount) noexcept { amountSmoother.setTargetValue(std::clamp(newAmount, 0.0f, 1.0f)); }
    float getAmount() const noexcept { return amountSmoother.getTargetValue(); }

    void setCharacter(Character newChar) noexcept { character.store(newChar); }
    Character getCharacter() const noexcept { return character.load(); }

    void setTone(float newTone) noexcept { toneSmoother.setTargetValue(std::clamp(newTone, 0.0f, 1.0f)); }
    void setMix(float newMix) noexcept { mixSmoother.setTargetValue(std::clamp(newMix, 0.0f, 1.0f)); }

    void setPunish(bool enabled) noexcept { punishEnabled.store(enabled); }
    bool isPunishEnabled() const noexcept { return punishEnabled.load(); }

    void setTransientProtection(float protection) noexcept { transientProtection.store(std::clamp(protection, 0.0f, 1.0f)); }

    void setOversamplingEnabled(bool enabled) noexcept { oversamplingEnabled.store(enabled); }

    void process(juce::AudioBuffer<float>& buffer);

private:
    float processSample(float inputSample, int channel, float currentAmount, float currentTone, bool isPunished, float transProt);

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> amountSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> toneSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> punishGainSmoother;

    std::atomic<Character> character { Character::Tube12AX7 };
    std::atomic<bool> punishEnabled { false };
    std::atomic<bool> oversamplingEnabled { true };
    std::atomic<float> transientProtection { 0.0f };

    double sampleRate { 44100.0 };

    // Dynamic physical model state registers
    float tubeBiasSag[2] { 0.0f, 0.0f };
    float tapeHysteresis[2] { 0.0f, 0.0f };

    // Low-end clean crossover filters
    juce::dsp::StateVariableTPTFilter<float> lowCrossoverFilter[2];
    juce::dsp::StateVariableTPTFilter<float> highCrossoverFilter[2];

    // Pre-emphasis & De-emphasis analog filters
    juce::dsp::StateVariableTPTFilter<float> preEmphasisFilter[2];
    juce::dsp::StateVariableTPTFilter<float> deEmphasisFilter[2];

    // Tape Head-Bump Filter (60Hz resonant bell)
    juce::dsp::StateVariableTPTFilter<float> tapeHeadBumpFilter[2];

    // Post Tone filters per channel
    juce::dsp::StateVariableTPTFilter<float> lowpassFilter[2];
    juce::dsp::StateVariableTPTFilter<float> highpassFilter[2];

    // DC Blocker per channel
    AudioUtils::DCBlocker dcBlocker[2];

    // 4x Minimum Phase Polyphase Oversampling Engine (2 factor stages = 4x)
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrushProcessor)
};

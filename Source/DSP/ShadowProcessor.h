#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../Utils/AudioUtils.h"

/**
 * @class ShadowProcessor
 * @brief Commercial-grade Pitch & Formant Shifting Engine.
 * Features:
 * - 4-Phase Grain Engine with 3rd-Order Cubic Hermite Fractional Interpolation
 * - Anatomical 3-Resonance Vocal Tract Formant Filter Bank (F1, F2, F3)
 * - Harmonic Sub-Octave Generator for Chest Resonance (Vocal Bender / Murda Melodies caliber)
 * - Frequency-Dependent Smooth Saturation & DC Protection
 */
class ShadowProcessor
{
public:
    enum class PitchInterval
    {
        OctaveDown = 0,  // -12 semitones
        FifthDown  = 1,  // -7 semitones
        FourthDown = 2,  // -5 semitones
        TwoOctaves = 3   // -24 semitones
    };

    ShadowProcessor();
    ~ShadowProcessor();

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setEnabled(bool shouldBeEnabled) noexcept { enabled = shouldBeEnabled; }
    bool isEnabled() const noexcept { return enabled; }

    void setMix(float newMix) noexcept { mix = std::clamp(newMix, 0.0f, 1.0f); }
    void setPitchInterval(PitchInterval interval) noexcept { pitchInterval = interval; }
    void setFormantShift(float newFormant) noexcept { formantShift = std::clamp(newFormant, 0.0f, 1.0f); }
    void setDarkness(float newDarkness) noexcept { darkness = std::clamp(newDarkness, 0.0f, 1.0f); }
    void setDrive(float newDrive) noexcept { drive = std::clamp(newDrive, 0.0f, 1.0f); }

    void process(juce::AudioBuffer<float>& buffer);

private:
    float processSample(float inputSample, int channel);
    inline float readHermite(const float* buffer, float delaySamples, int writePos, int mask) noexcept;

    bool enabled { true };
    float mix { 0.0f };
    PitchInterval pitchInterval { PitchInterval::OctaveDown };
    float formantShift { 0.5f };
    float darkness { 0.5f };
    float drive { 0.2f };

    double sampleRate { 44100.0 };

    // 4-Phase Granular Pitch Delay Line (Power-of-two size for fast bitwise masking)
    static constexpr int BUFFER_SIZE = 16384;
    static constexpr int BUFFER_MASK = BUFFER_SIZE - 1;

    std::vector<float> grainBufferL;
    std::vector<float> grainBufferR;
    int writeIndex { 0 };

    // 4 Grain Read Heads per channel
    float grainPhase[2][4] { { 0.0f, 0.25f, 0.50f, 0.75f }, { 0.0f, 0.25f, 0.50f, 0.75f } };

    // Anatomical 3-Resonance Vocal Tract Formant Filter Bank (F1, F2, F3)
    juce::dsp::StateVariableTPTFilter<float> formantF1[2]; // Pharynx (300 - 800 Hz)
    juce::dsp::StateVariableTPTFilter<float> formantF2[2]; // Oral Cavity (900 - 2300 Hz)
    juce::dsp::StateVariableTPTFilter<float> formantF3[2]; // Singing Formant / Throat (2400 - 3600 Hz)

    // Tone & Darkness low-pass / shelf filters
    juce::dsp::StateVariableTPTFilter<float> darknessFilter[2];
    juce::dsp::StateVariableTPTFilter<float> subWarmthFilter[2];

    // DC Blocker per channel
    AudioUtils::DCBlocker dcBlocker[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShadowProcessor)
};

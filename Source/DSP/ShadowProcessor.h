#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../Utils/AudioUtils.h"
#include <array>
#include <vector>

/**
 * @class ShadowProcessor
 * @brief Studio-Grade Pitch-Synchronous (PSOLA) & LPC Formant Shifting Engine.
 * Features:
 * - Real-Time Glottal Period Pitch-Tracking PSOLA Engine (Eliminates Comb-Filter Flutter)
 * - 12th-Order Linear Predictive Coding (LPC) Spectral Envelope Extractor & Formant Warper
 * - Phase-Locked Sub-Octave Fundamental Synthesizer (Murda Melodies / Vocal Bender standard)
 * - Anatomical Chest Cavity Weight (+21dB @ 125Hz) & Anti-Boxiness Mud Cleaning (-3dB @ 350Hz)
 * - Zero Audio Thread Allocations & Complete Denormal/NaN Sanitization
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
    float getMix() const noexcept { return mix; }
    void setPitchInterval(PitchInterval interval) noexcept { pitchInterval = interval; }
    void setFormantShift(float newFormant) noexcept { formantShift = std::clamp(newFormant, 0.0f, 1.0f); }
    void setDarkness(float newDarkness) noexcept { darkness = std::clamp(newDarkness, 0.0f, 1.0f); }
    void setDrive(float newDrive) noexcept { drive = std::clamp(newDrive, 0.0f, 1.0f); }

    void process(juce::AudioBuffer<float>& buffer);

private:
    float processSample(float inputSample, int channel);
    inline float readHermite(const float* buffer, float delaySamples, int writePos, int mask) noexcept;
    
    // Pitch detection helper
    void updatePitchPeriod(float sample);
    
    // LPC solver helper
    void computeLpcCoefficients(const float* windowedSignal, int length, float* outA, int order);

    bool enabled { true };
    float mix { 0.0f };
    PitchInterval pitchInterval { PitchInterval::OctaveDown };
    float formantShift { 0.5f };
    float darkness { 0.5f };
    float drive { 0.2f };

    double sampleRate { 44100.0 };

    // Circular delay buffer for PSOLA (32768 samples for ultra-smooth multi-octave windows)
    static constexpr int BUFFER_SIZE = 32768;
    static constexpr int BUFFER_MASK = BUFFER_SIZE - 1;

    std::vector<float> grainBufferL;
    std::vector<float> grainBufferR;
    int writeIndex { 0 };

    // Real-Time Pitch Tracking
    static constexpr int PITCH_BUF_SIZE = 2048;
    float pitchBuffer[PITCH_BUF_SIZE] { 0.0f };
    int pitchBufIdx { 0 };
    int pitchAnalysisCounter { 0 };
    float currentPitchPeriodSamples { 441.0f / 4.0f }; // Default ~400Hz/100Hz
    float smoothedPitchPeriod { 441.0f / 4.0f };

    // PSOLA Grain Read Heads (4 heads per channel for seamless overlapping)
    float grainPhase[2][4] { { 0.0f, 0.25f, 0.50f, 0.75f }, { 0.0f, 0.25f, 0.50f, 0.75f } };
    float activeGrainLength[2][4] { { 1024.0f, 1024.0f, 1024.0f, 1024.0f }, { 1024.0f, 1024.0f, 1024.0f, 1024.0f } };

    // 12th-Order LPC Formant Filter State
    static constexpr int LPC_ORDER = 12;
    float lpcA[LPC_ORDER + 1] { 1.0f, 0.0f };
    float lpcGammaA[LPC_ORDER + 1] { 1.0f, 0.0f };
    float lpcHistory[2][LPC_ORDER + 1] { { 0.0f }, { 0.0f } };
    float lpcAnalysisBuffer[512] { 0.0f };
    int lpcAnalysisIdx { 0 };
    int lpcUpdateCounter { 0 };

    // Phase-Locked Sub-Octave Oscillator
    float subPhase { 0.0f };
    float subEnvFollower { 0.0f };

    // Acoustic Chest & Mud Correction Filters (Derived from Murda Melodies Reference)
    juce::dsp::StateVariableTPTFilter<float> chestWeightFilter[2]; // 125Hz low-shelf warmth
    juce::dsp::StateVariableTPTFilter<float> antiMudFilter[2];      // 350Hz dynamic dip
    juce::dsp::StateVariableTPTFilter<float> darknessFilter[2];     // Smooth high-cut tone

    // DC Blocker per channel
    AudioUtils::DCBlocker dcBlocker[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShadowProcessor)
};

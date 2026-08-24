#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../../libs/signalsmith-stretch.h"
#include "../Utils/AudioUtils.h"
#include <vector>

/**
 * @class ShadowProcessor
 * @brief Studio-Grade Vocal Pitch & Formant Shifter (Little AlterBoy / Murda Melodies caliber).
 * Powered by vocal-optimized Signalsmith Stretch Phase Vocoder:
 * - Low-latency 22ms analysis window (eliminates 120ms spectral phase smearing and white noise)
 * - Decoupled pitch (-24 to +24 ST) and vocal-tract throat formant morphing (-12 to +12 ST)
 * - High-frequency tonality preservation (protects sibilants and breath transients)
 * - Delay-compensated dry/wet crossfading for 100% phase-coherent mixing
 * - Transpose, Monotone Robot, and Hard-Tune vocal modes
 * - 12AX7 tube saturation & dynamic vocal tone control
 * - Zero audio-thread allocations, real-time safe
 */
class ShadowProcessor
{
public:
    enum class DemonMode
    {
        Transpose = 0,  // Musical pitch transposition
        Robot     = 1,  // Locked robotic drone pitch
        HardTune  = 2   // Semitone-quantized hard pitch
    };

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

    void setMix(float newMix) noexcept { mix = std::clamp(newMix, 0.0f, 1.0f); mixSmoother.setTargetValue(mix); }
    float getMix() const noexcept { return mix; }

    // Direct Semitone Pitch Transposition (-24.0 to +24.0 ST)
    void setPitchSemitones(float semis) noexcept { pitchSemitones = std::clamp(semis, -24.0f, 24.0f); pitchSmoother.setTargetValue(pitchSemitones); }
    float getPitchSemitones() const noexcept { return pitchSemitones; }

    // Direct Semitone Formant / Throat Shifter (-12.0 to +12.0 ST)
    void setFormantSemitones(float semis) noexcept { formantSemitones = std::clamp(semis, -12.0f, 12.0f); formantSmoother.setTargetValue(formantSemitones); }
    float getFormantSemitones() const noexcept { return formantSemitones; }

    // Pitch & Formant Varispeed Link
    void setLink(bool shouldLink) noexcept { linkEnabled = shouldLink; }
    bool isLinked() const noexcept { return linkEnabled; }

    // Demonic Voicing Mode
    void setMode(DemonMode newMode) noexcept { mode = newMode; }
    DemonMode getMode() const noexcept { return mode; }

    // Drive & Tone
    void setDrive(float newDrive) noexcept { drive = std::clamp(newDrive, 0.0f, 1.0f); driveSmoother.setTargetValue(drive); }
    float getDrive() const noexcept { return drive; }

    void setDarkness(float newDarkness) noexcept { darkness = std::clamp(newDarkness, 0.0f, 1.0f); }
    float getDarkness() const noexcept { return darkness; }

    void setClarity(float newClarity) noexcept { clarity = std::clamp(newClarity, 0.0f, 1.0f); claritySmoother.setTargetValue(clarity); }
    float getClarity() const noexcept { return clarity; }

    // Legacy compatibility helpers
    void setPitchInterval(PitchInterval interval) noexcept;
    PitchInterval getPitchInterval() const noexcept;
    void setFormantShift(float normalizedShift) noexcept;
    float getFormantShift() const noexcept;

    int getLatencySamples() const noexcept { return latencySamples; }

    void process(juce::AudioBuffer<float>& buffer);

private:
    bool enabled { true };
    float mix { 0.0f };
    float pitchSemitones { -12.0f };
    float formantSemitones { 0.0f };
    bool linkEnabled { false };
    DemonMode mode { DemonMode::Transpose };
    float darkness { 0.5f };
    float drive { 0.25f };
    float clarity { 0.65f };

    double sampleRate { 44100.0 };
    int numChannels { 2 };
    int latencySamples { 0 };

    // Vocal-Optimized Signalsmith Stretch Phase Vocoder
    signalsmith::stretch::SignalsmithStretch<float> stretchEngine;

    // Pre-allocated non-allocating scratch buffers
    juce::AudioBuffer<float> stretchScratchBuffer;
    std::vector<const float*> inputChannelPointers;
    std::vector<float*> outputChannelPointers;

    // Delay compensation for dry alignment
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> dryDelayLine[2];

    // Parameter Smoothers (Zipper-free transitions)
    juce::SmoothedValue<float> mixSmoother;
    juce::SmoothedValue<float> pitchSmoother;
    juce::SmoothedValue<float> formantSmoother;
    juce::SmoothedValue<float> driveSmoother;
    juce::SmoothedValue<float> claritySmoother;

    // Post-processing Analog Filters
    juce::dsp::StateVariableTPTFilter<float> toneDarknessFilter[2];
    juce::dsp::StateVariableTPTFilter<float> subRumbleHighpass[2];
    AudioUtils::DCBlocker dcBlocker[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ShadowProcessor)
};

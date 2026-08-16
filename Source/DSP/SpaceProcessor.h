#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../Utils/AudioUtils.h"
#include <vector>

/**
 * @class SpaceProcessor
 * @brief Commercial High-Density Dattorro Diffusion Reverb & Polyrhythmic Ping-Pong Tape Delay.
 * Features:
 * - High-Density 8-Node Allpass Diffusion Network + Cross-Coupled Recirculating Tank
 * - Lexicon / Plate-Style Chorused Decay Modulation (zero metallic resonance)
 * - Polyrhythmic Stereo Ping-Pong Tape Echo with Wow/Flutter & Soft Feedback Saturation
 * - Vocal Sidechain Auto-Ducking (preserves vocal clarity in dense mixes)
 */
class SpaceProcessor
{
public:
    SpaceProcessor() = default;
    ~SpaceProcessor() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setReverbMix(float newReverb) noexcept { reverbMix = std::clamp(newReverb, 0.0f, 1.0f); }
    void setDelayTime(float newTime) noexcept   { delayTime = std::clamp(newTime, 0.01f, 1.5f); }
    void setFeedback(float newFb) noexcept      { feedback = std::clamp(newFb, 0.0f, 0.95f); }
    void setDucking(float newDucking) noexcept  { ducking = std::clamp(newDucking, 0.0f, 1.0f); }
    void setShimmer(float newShimmer) noexcept  { shimmerMix = std::clamp(newShimmer, 0.0f, 1.0f); }

    void process(const juce::dsp::ProcessContextReplacing<float>& context);

private:
    float reverbMix  { 0.3f };
    float delayTime  { 0.25f }; // Seconds
    float feedback   { 0.35f };
    float ducking    { 0.5f };
    float shimmerMix { 0.25f };

    double sampleRate { 44100.0 };

    // --- High-Density Dattorro Diffusion Reverb Tank ---
    struct AllpassDelay
    {
        std::vector<float> buffer;
        int writePos { 0 };
        float feedbackCoef { 0.5f };

        void init(int maxSamples, float fb)
        {
            buffer.assign(std::max(16, maxSamples), 0.0f);
            writePos = 0;
            feedbackCoef = fb;
        }

        void reset()
        {
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writePos = 0;
        }

        inline float process(float in) noexcept
        {
            if (buffer.empty()) return in;
            float out = buffer[writePos];
            float writeVal = in + out * feedbackCoef;
            buffer[writePos] = writeVal;
            writePos = (writePos + 1) % static_cast<int>(buffer.size());
            return out - writeVal * feedbackCoef;
        }
    };

    // 4 Pre-Diffusers
    AllpassDelay diff1, diff2, diff3, diff4;

    // 2 Tank Recirculating Allpasses
    AllpassDelay tankDiff1, tankDiff2;

    // 4 Tank Delay Lines
    std::vector<float> tankDelayL1, tankDelayL2;
    std::vector<float> tankDelayR1, tankDelayR2;
    int tankIdxL1 { 0 }, tankIdxL2 { 0 };
    int tankIdxR1 { 0 }, tankIdxR2 { 0 };

    float tankLFOPhase { 0.0f };
    float dampingFilterL { 0.0f };
    float dampingFilterR { 0.0f };

    // --- Polyrhythmic Ping-Pong Tape Delay ---
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineL{ 192000 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineR{ 192000 };

    // Analog 3.5kHz feedback damping filters
    juce::dsp::StateVariableTPTFilter<float> delayDampFilterL;
    juce::dsp::StateVariableTPTFilter<float> delayDampFilterR;
    juce::dsp::StateVariableTPTFilter<float> reverbLowCutL;
    juce::dsp::StateVariableTPTFilter<float> reverbLowCutR;

    float envFollower { 0.0f };
    float wowPhase { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpaceProcessor)
};

#include "VocalCompressor.h"
#include <cmath>
#include <algorithm>

VocalCompressor::VocalCompressor() = default;

void VocalCompressor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    smoothedThresholdDb.reset(sampleRate, 0.03);
    smoothedMakeupGain.reset(sampleRate, 0.03);

    for (int ch = 0; ch < 2; ++ch)
        dcBlocker[ch].prepare(sampleRate, 15.0f);

    reset();
}

void VocalCompressor::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        envFast[ch] = 0.0f;
        envSlow[ch] = 0.0f;
        dcBlocker[ch].reset();
    }
    smoothedThresholdDb.setCurrentAndTargetValue(0.0f);
    smoothedMakeupGain.setCurrentAndTargetValue(1.0f);
    currentGainReductionDb.store(0.0f);
}

void VocalCompressor::setSqueeze(float newSqueeze) noexcept
{
    float sq = std::clamp(newSqueeze, 0.0f, 1.0f);
    squeezeAmount.store(sq);

    // Map 0.0 -> 1.0 to Threshold 0 dBFS down to -34 dBFS
    float targetThresh = -34.0f * sq;
    smoothedThresholdDb.setTargetValue(targetThresh);

    // Musical Auto-makeup: max +4.8 dB at 100% squeeze (prevents blowing out downstream limiters)
    float expectedGrDb = sq * 12.0f;
    float autoGainLinear = std::pow(10.0f, (expectedGrDb * 0.40f) / 20.0f);
    smoothedMakeupGain.setTargetValue(autoGainLinear);
}

void VocalCompressor::process(juce::AudioBuffer<float>& buffer)
{
    float sq = squeezeAmount.load();
    if (sq <= 0.001f)
    {
        currentGainReductionDb.store(0.0f);
        return;
    }

    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    int charMode = compCharacter.load();

    // Stage 1 (FET): 0.15ms attack, 35ms release
    const float attFastCoeff = std::exp(-1.0f / (0.00015f * (float)sampleRate));
    const float relFastCoeff = std::exp(-1.0f / (0.035f * (float)sampleRate));

    // Stage 2 (Opto): 10ms attack, 240ms release
    const float attSlowCoeff = std::exp(-1.0f / (0.010f * (float)sampleRate));
    const float relSlowCoeff = std::exp(-1.0f / (0.240f * (float)sampleRate));

    const float kneeWidthDb = 6.0f;
    const float halfKnee = kneeWidthDb * 0.5f;
    const float ratio = (charMode == 0) ? 8.0f : (charMode == 1 ? 4.0f : 5.0f);
    const float slope = 1.0f - (1.0f / ratio);

    float maxGrThisBlockDb = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float currentThresh = smoothedThresholdDb.getNextValue();
        float currentMakeup = smoothedMakeupGain.getNextValue();

        // Linked stereo sidechain detection
        float maxAbs = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float s = std::abs(buffer.getSample(ch, i));
            if (s > maxAbs) maxAbs = s;
        }

        // Update Dual-Stage Envelope Followers
        if (maxAbs > envFast[0])
            envFast[0] = maxAbs + attFastCoeff * (envFast[0] - maxAbs);
        else
            envFast[0] = maxAbs + relFastCoeff * (envFast[0] - maxAbs);

        if (maxAbs > envSlow[0])
            envSlow[0] = maxAbs + attSlowCoeff * (envSlow[0] - maxAbs);
        else
            envSlow[0] = maxAbs + relSlowCoeff * (envSlow[0] - maxAbs);

        // Blend detector based on character mode
        float detectorLevel;
        if (charMode == 0)      detectorLevel = envFast[0];                     // Pure Fast FET
        else if (charMode == 1) detectorLevel = envSlow[0];                     // Pure Smooth Opto
        else                    detectorLevel = 0.35f * envFast[0] + 0.65f * envSlow[0]; // Dual-Stage Blend

        detectorLevel = std::max(detectorLevel, 1.0e-5f);
        float detectorDb = 20.0f * std::log10(detectorLevel);
        float grDb = 0.0f;

        // Soft-Knee Gain Computation
        float deltaDb = detectorDb - currentThresh;
        if (deltaDb > halfKnee)
        {
            grDb = slope * deltaDb;
        }
        else if (deltaDb > -halfKnee)
        {
            float kneeVal = deltaDb + halfKnee;
            grDb = slope * (kneeVal * kneeVal) / (2.0f * kneeWidthDb);
        }

        grDb = std::max(0.0f, grDb);
        if (grDb > maxGrThisBlockDb)
            maxGrThisBlockDb = grDb;

        float gainReductionLinear = std::pow(10.0f, -grDb / 20.0f);

        // Dynamic noise floor makeup suppression:
        // Smoothly taper makeup gain down when signal is near silence/whisper below -50 dBFS
        float effectiveMakeup = currentMakeup;
        if (detectorDb < -48.0f)
        {
            float silenceRatio = std::clamp((detectorDb - (-60.0f)) / 12.0f, 0.0f, 1.0f);
            effectiveMakeup = 1.0f + (currentMakeup - 1.0f) * (silenceRatio * silenceRatio);
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample(ch, i);
            input = AudioUtils::sanitize(input);
            input = dcBlocker[ch].process(input);

            float output = input * gainReductionLinear * effectiveMakeup;
            buffer.setSample(ch, i, AudioUtils::sanitize(output));
        }
    }

    // Smoothly decay UI meter reduction
    float prevGr = currentGainReductionDb.load();
    float newGrMeter = std::max(maxGrThisBlockDb, prevGr * 0.85f);
    currentGainReductionDb.store(newGrMeter);
}

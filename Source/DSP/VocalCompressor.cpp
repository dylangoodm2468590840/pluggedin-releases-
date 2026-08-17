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
        dcBlocker[ch].reset();

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

    // Map 0.0 -> 1.0 to Threshold 0 dBFS down to -38 dBFS
    float targetThresh = -38.0f * sq;
    smoothedThresholdDb.setTargetValue(targetThresh);

    // Auto-makeup calculation based on average reduction curve
    // Provides studio-grade constant perceived loudness
    float expectedGrDb = sq * 18.0f;
    float autoGainLinear = std::pow(10.0f, (expectedGrDb * 0.55f) / 20.0f);
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

    // Attack / Release time constants (in seconds)
    // Stage 1 (FET): 0.2ms attack, 35ms release
    const float attFastCoeff = std::exp(-1.0f / (0.0002f * (float)sampleRate));
    const float relFastCoeff = std::exp(-1.0f / (0.035f * (float)sampleRate));

    // Stage 2 (Opto): 8ms attack, 220ms release
    const float attSlowCoeff = std::exp(-1.0f / (0.008f * (float)sampleRate));
    const float relSlowCoeff = std::exp(-1.0f / (0.220f * (float)sampleRate));

    const float kneeWidthDb = 5.0f;
    const float halfKnee = kneeWidthDb * 0.5f;
    const float ratio = 5.0f; // Standard commercial vocal compression ratio (5:1)
    const float slope = 1.0f - (1.0f / ratio);

    float maxGrThisBlockDb = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float currentThresh = smoothedThresholdDb.getNextValue();
        float currentMakeup = smoothedMakeupGain.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample(ch, i);
            input = AudioUtils::sanitize(input);
            input = dcBlocker[ch].process(input);


            float absVal = std::abs(input);

            // Update Dual-Stage Envelope Followers
            if (absVal > envFast[ch])
                envFast[ch] = absVal + attFastCoeff * (envFast[ch] - absVal);
            else
                envFast[ch] = absVal + relFastCoeff * (envFast[ch] - absVal);

            if (absVal > envSlow[ch])
                envSlow[ch] = absVal + attSlowCoeff * (envSlow[ch] - absVal);
            else
                envSlow[ch] = absVal + relSlowCoeff * (envSlow[ch] - absVal);

            // Blend FET fast peak tracking with Opto body tracking (60% Opto / 40% FET)
            float detectorLevel = 0.40f * envFast[ch] + 0.60f * envSlow[ch];
            detectorLevel = std::max(detectorLevel, 1.0e-5f);

            float detectorDb = 20.0f * std::log10(detectorLevel);
            float grDb = 0.0f;

            // Soft-Knee Gain Computation
            float deltaDb = detectorDb - currentThresh;
            if (deltaDb > halfKnee)
            {
                // Full compression region
                grDb = slope * deltaDb;
            }
            else if (deltaDb > -halfKnee)
            {
                // Soft-knee transition region
                float kneeVal = deltaDb + halfKnee;
                grDb = slope * (kneeVal * kneeVal) / (2.0f * kneeWidthDb);
            }
            else
            {
                grDb = 0.0f;
            }

            grDb = std::max(0.0f, grDb);
            if (grDb > maxGrThisBlockDb)
                maxGrThisBlockDb = grDb;

            // Compute linear gain reduction multiplier
            float gainReductionLinear = std::pow(10.0f, -grDb / 20.0f);

            // Dynamic noise floor makeup suppression:
            // If detector is below -48 dBFS, taper makeup gain down to unity (1.0) so silence/whisper stays dead quiet!
            float effectiveMakeup = currentMakeup;
            if (detectorDb < -48.0f)
            {
                float silenceRatio = std::clamp((detectorDb - (-65.0f)) / 17.0f, 0.0f, 1.0f);
                effectiveMakeup = 1.0f + (currentMakeup - 1.0f) * (silenceRatio * silenceRatio);
            }

            // Apply compression + studio-safe auto-makeup gain
            float output = input * gainReductionLinear * effectiveMakeup;
            buffer.setSample(ch, i, AudioUtils::sanitize(output));
        }
    }

    // Smoothly decay UI meter reduction
    float prevGr = currentGainReductionDb.load();
    float newGrMeter = std::max(maxGrThisBlockDb, prevGr * 0.85f);
    currentGainReductionDb.store(newGrMeter);
}

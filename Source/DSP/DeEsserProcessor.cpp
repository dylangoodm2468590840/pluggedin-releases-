#include "DeEsserProcessor.h"
#include <cmath>
#include <algorithm>

DeEsserProcessor::DeEsserProcessor() = default;

void DeEsserProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    smoothedThresholdDb.reset(sampleRate, 0.02);
    smoothedFreq.reset(sampleRate, 0.02);

    for (int ch = 0; ch < 2; ++ch)
    {
        sibilanceFilter[ch].prepare(spec);
        sibilanceFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        sibilanceFilter[ch].setResonance(1.4f);

        sidechainDetectorFilter[ch].prepare(spec);
        sidechainDetectorFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        sidechainDetectorFilter[ch].setResonance(2.2f); // Focused detection on sibilance center
    }

    reset();
}

void DeEsserProcessor::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        sibilanceFilter[ch].reset();
        sidechainDetectorFilter[ch].reset();
        sibilanceEnv[ch] = 0.0f;
    }
    smoothedThresholdDb.setCurrentAndTargetValue(0.0f);
    smoothedFreq.setCurrentAndTargetValue(6500.0f);
    currentGainReductionDb.store(0.0f);
}

void DeEsserProcessor::setAmount(float newAmount) noexcept
{
    float amt = std::clamp(newAmount, 0.0f, 1.0f);
    amount.store(amt);

    // Map 0.0 -> 1.0 to Detection Threshold -6 dB down to -46 dB
    float targetThresh = -6.0f - (40.0f * amt);
    smoothedThresholdDb.setTargetValue(targetThresh);
}

void DeEsserProcessor::setFrequency(float newFreqHz) noexcept
{
    float f = std::clamp(newFreqHz, 4000.0f, 10000.0f);
    frequency.store(f);
    smoothedFreq.setTargetValue(f);
}

void DeEsserProcessor::process(juce::AudioBuffer<float>& buffer)
{
    float amt = amount.load();
    if (amt <= 0.001f)
    {
        currentGainReductionDb.store(0.0f);
        return;
    }

    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    // Fast attack (1ms) to clamp harsh sibilant spikes instantly, 35ms release
    const float attCoeff = std::exp(-1.0f / (0.001f * (float)sampleRate));
    const float relCoeff = std::exp(-1.0f / (0.035f * (float)sampleRate));

    const float ratio = 6.0f; // Aggressive sibilance clamping
    const float slope = 1.0f - (1.0f / ratio);

    float maxGrThisBlockDb = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float currentThresh = smoothedThresholdDb.getNextValue();
        float currentCutoff = smoothedFreq.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            sibilanceFilter[ch].setCutoffFrequency(currentCutoff);
            sidechainDetectorFilter[ch].setCutoffFrequency(currentCutoff);

            float input = buffer.getSample(ch, i);
            input = AudioUtils::sanitize(input);

            // Isolate sibilance band
            float sibilantBand = sibilanceFilter[ch].processSample(0, input);

            // Sidechain bandpass for focused sibilance detection
            float scSignal = sidechainDetectorFilter[ch].processSample(0, input);
            float scAbs = std::abs(scSignal);

            // Fast envelope follower
            if (scAbs > sibilanceEnv[ch])
                sibilanceEnv[ch] = scAbs + attCoeff * (sibilanceEnv[ch] - scAbs);
            else
                sibilanceEnv[ch] = scAbs + relCoeff * (sibilanceEnv[ch] - scAbs);

            float detLevel = std::max(sibilanceEnv[ch], 1.0e-5f);
            float detDb = 20.0f * std::log10(detLevel);

            float grDb = 0.0f;
            if (detDb > currentThresh)
            {
                grDb = slope * (detDb - currentThresh);
            }

            grDb = std::clamp(grDb, 0.0f, 24.0f);
            if (grDb > maxGrThisBlockDb)
                maxGrThisBlockDb = grDb;

            float attenuationFactor = 1.0f - std::pow(10.0f, -grDb / 20.0f);

            // Zero-phase subtractive dynamic de-essing:
            // When grDb == 0, attenuationFactor == 0, output is 100% bit-exact identical to input (0 notch).
            float processedSample = input - sibilantBand * attenuationFactor;

            buffer.setSample(ch, i, AudioUtils::sanitize(processedSample));
        }
    }

    float prevGr = currentGainReductionDb.load();
    float newGrMeter = std::max(maxGrThisBlockDb, prevGr * 0.85f);
    currentGainReductionDb.store(newGrMeter);
}

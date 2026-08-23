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
        dynamicBellFilter[ch].prepare(spec);
        dynamicBellFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        dynamicBellFilter[ch].setResonance(1.4f);

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
        dynamicBellFilter[ch].reset();
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

    if (numChannels == 0 || numSamples == 0)
        return;

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

        // Linked stereo sidechain detection
        float scMaxAbs = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            sidechainDetectorFilter[ch].setCutoffFrequency(currentCutoff);
            dynamicBellFilter[ch].setCutoffFrequency(currentCutoff);

            float input = buffer.getSample(ch, i);
            float scSignal = sidechainDetectorFilter[ch].processSample(0, input);
            float scAbs = std::abs(scSignal);
            if (scAbs > scMaxAbs) scMaxAbs = scAbs;
        }

        // Update linked envelope follower
        float detEnv = sibilanceEnv[0];
        if (scMaxAbs > detEnv)
            detEnv = scMaxAbs + attCoeff * (detEnv - scMaxAbs);
        else
            detEnv = scMaxAbs + relCoeff * (detEnv - scMaxAbs);
        sibilanceEnv[0] = detEnv;
        sibilanceEnv[1] = detEnv;

        float detLevel = std::max(detEnv, 1.0e-5f);
        float detDb = 20.0f * std::log10(detLevel);

        float grDb = 0.0f;
        if (detDb > currentThresh)
        {
            grDb = slope * (detDb - currentThresh);
        }

        grDb = std::clamp(grDb, 0.0f, 20.0f);
        if (grDb > maxGrThisBlockDb)
            maxGrThisBlockDb = grDb;

        // Dynamic notch cut gain: 0 dB = 1.0 (untouched), >0 dB = attenuates only the sibilant band
        float bandGain = std::pow(10.0f, -grDb / 20.0f);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample(ch, i);
            input = AudioUtils::sanitize(input);

            // Dynamic bell filter: input - sibilantBand * (1.0 - bandGain)
            // When grDb == 0, bandGain == 1.0, attenuation is 0.0 (bit-exact unity input)
            float sibilantBand = dynamicBellFilter[ch].processSample(0, input);
            float processedSample = input - sibilantBand * (1.0f - bandGain);

            buffer.setSample(ch, i, AudioUtils::sanitize(processedSample));
        }
    }

    float prevGr = currentGainReductionDb.load();
    float newGrMeter = std::max(maxGrThisBlockDb, prevGr * 0.85f);
    currentGainReductionDb.store(newGrMeter);
}

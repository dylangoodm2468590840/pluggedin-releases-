#include "AirExciterProcessor.h"
#include <cmath>
#include <algorithm>

AirExciterProcessor::AirExciterProcessor()
{
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
}

void AirExciterProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    midAirSmoother.reset(sampleRate, 0.02);
    topAirSmoother.reset(sampleRate, 0.02);

    if (oversampler)
    {
        oversampler->initProcessing(spec.maximumBlockSize);
        oversamplingReady = true;
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        // Mid-Air Bandpass (3.5 kHz - 7.5 kHz)
        midAirFilter[ch].prepare(spec);
        midAirFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        midAirFilter[ch].setCutoffFrequency(4800.0f);
        midAirFilter[ch].setResonance(0.85f);

        // Top-Air Highpass (8.5 kHz+)
        topAirFilter[ch].prepare(spec);
        topAirFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        topAirFilter[ch].setCutoffFrequency(9200.0f);
        topAirFilter[ch].setResonance(0.707f);

        dcBlocker[ch].prepare(sampleRate, 20.0f);
    }

    reset();
}

void AirExciterProcessor::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        midAirFilter[ch].reset();
        topAirFilter[ch].reset();
        dcBlocker[ch].reset();
        midEnv[ch] = 0.0f;
        topEnv[ch] = 0.0f;
    }
    if (oversampler)
        oversampler->reset();

    midAirSmoother.setCurrentAndTargetValue(0.0f);
    topAirSmoother.setCurrentAndTargetValue(0.0f);
}

float AirExciterProcessor::processBandExciter(float inputBand, float amount, float& envState, float attCoeff, float relCoeff)
{
    if (amount <= 0.0001f || std::abs(inputBand) < 1.0e-5f)
        return 0.0f;

    float absVal = std::abs(inputBand);

    // Fast envelope follower
    if (absVal > envState)
        envState = absVal + attCoeff * (envState - absVal);
    else
        envState = absVal + relCoeff * (envState - absVal);

    if (envState < 1.0e-5f)
        return 0.0f;

    float envDb = 20.0f * std::log10(std::max(envState, 1.0e-5f));

    // Smooth soft-knee gate: transparent fade-in from -54 dBFS without clicks
    float airGate = std::clamp((envDb - (-54.0f)) / 14.0f, 0.0f, 1.0f);
    airGate = airGate * airGate;

    // Silky asymmetric even/odd harmonic sheen generator (Maag EQ4 / Dolby 361 style)
    float x = std::clamp(inputBand * 1.25f, -1.0f, 1.0f);
    float signX = (x >= 0.0f) ? 1.0f : -1.0f;
    float evenHarm = (2.0f * x * x - 1.0f) * signX;
    float harmonic = std::tanh(0.70f * x + 0.30f * evenHarm);

    return harmonic * (amount * 0.45f) * airGate;
}

void AirExciterProcessor::process(juce::AudioBuffer<float>& buffer)
{
    float currentMid = midAirSmoother.getTargetValue();
    float currentTop = topAirSmoother.getTargetValue();

    if (currentMid <= 0.001f && currentTop <= 0.001f)
        return;

    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    // 2ms attack for rapid transient response, 40ms release for natural decay
    const float attCoeff = std::exp(-1.0f / (0.002f * (float)sampleRate));
    const float relCoeff = std::exp(-1.0f / (0.040f * (float)sampleRate));

    for (int i = 0; i < numSamples; ++i)
    {
        float midAmt = midAirSmoother.getNextValue();
        float topAmt = topAirSmoother.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float input = buffer.getSample(ch, i);
            input = AudioUtils::sanitize(input);

            // Filter bands
            float midBand = midAirFilter[ch].processSample(0, input);
            float topBand = topAirFilter[ch].processSample(0, input);

            // Generate excited air
            float midExcited = processBandExciter(midBand, midAmt, midEnv[ch], attCoeff, relCoeff);
            float topExcited = processBandExciter(topBand, topAmt, topEnv[ch], attCoeff, relCoeff);

            float excitedTotal = dcBlocker[ch].process(midExcited + topExcited);

            // Sum clean signal with excited air at controlled parallel level
            float output = input + excitedTotal * 0.30f;
            buffer.setSample(ch, i, AudioUtils::sanitize(output));
        }
    }
}

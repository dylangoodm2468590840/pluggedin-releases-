#include "StudioMicroDetuner.h"

StudioMicroDetuner::StudioMicroDetuner()
{
    delayBufferL.resize(BUFFER_SIZE, 0.0f);
    delayBufferR.resize(BUFFER_SIZE, 0.0f);
}

void StudioMicroDetuner::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    amountSmoother.reset(sampleRate, 0.02);
    mixSmoother.reset(sampleRate, 0.02);

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    monoAnchorFilter.prepare(monoSpec);
    monoAnchorFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    monoAnchorFilter.setCutoffFrequency(160.0f);
    monoAnchorFilter.setResonance(0.707f);

    reset();
}

void StudioMicroDetuner::reset()
{
    std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
    std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
    writePos = 0;

    grainPhaseL[0] = 0.0f; grainPhaseL[1] = 0.5f;
    grainPhaseR[0] = 0.0f; grainPhaseR[1] = 0.5f;

    monoAnchorFilter.reset();

    amountSmoother.setCurrentAndTargetValue(amountSmoother.getTargetValue());
    mixSmoother.setCurrentAndTargetValue(mixSmoother.getTargetValue());
}

void StudioMicroDetuner::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;

    float currentAmt = amountSmoother.getTargetValue();
    if (currentAmt <= 0.001f)
        return;

    const float cents = detuneCents.load();
    const float grainSizeSamples = static_cast<float>(sampleRate) * 0.042f; // 42ms grain window for micro-pitch

    // Murda Melodies Effect 4 Asymmetric Detune ratios (+cents Left, -cents Right)
    const float shiftRatioL = std::pow(2.0f, (cents / 1200.0f));   // + cents
    const float shiftRatioR = std::pow(2.0f, (-cents / 1200.0f));  // - cents

    // Micro-pitch shift direction: advance read head for pitch up, retard for pitch down
    const float phaseIncL = (1.0f - shiftRatioL) / grainSizeSamples;
    const float phaseIncR = (1.0f - shiftRatioR) / grainSizeSamples;

    const float baseDelayL = static_cast<float>(sampleRate) * 0.014f; // 14ms offset Left
    const float baseDelayR = static_cast<float>(sampleRate) * 0.022f; // 22ms offset Right

    for (int i = 0; i < numSamples; ++i)
    {
        float amt = amountSmoother.getNextValue();
        float inL = buffer.getSample(0, i);
        float inR = numChannels > 1 ? buffer.getSample(1, i) : inL;

        // Sub-160Hz mono anchor
        float monoLow = monoAnchorFilter.processSample(0, (inL + inR) * 0.5f);

        // Write input to circular delay lines
        delayBufferL[writePos] = inL;
        delayBufferR[writePos] = inR;

        // Left Channel Detuning (+cents)
        float detunedL = 0.0f;
        for (int g = 0; g < 2; ++g)
        {
            float readOffset = baseDelayL + grainPhaseL[g] * grainSizeSamples;
            float readPos = static_cast<float>(writePos) - readOffset;
            float sample = readHermite(delayBufferL, readPos, BUFFER_MASK);

            // Hann window envelope for seamless crossfade
            float win = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * grainPhaseL[g]));
            detunedL += sample * win;

            grainPhaseL[g] += phaseIncL;
            if (grainPhaseL[g] >= 1.0f) grainPhaseL[g] -= 1.0f;
            else if (grainPhaseL[g] < 0.0f) grainPhaseL[g] += 1.0f;
        }

        // Right Channel Detuning (-cents)
        float detunedR = 0.0f;
        for (int g = 0; g < 2; ++g)
        {
            float readOffset = baseDelayR + grainPhaseR[g] * grainSizeSamples;
            float readPos = static_cast<float>(writePos) - readOffset;
            float sample = readHermite(delayBufferR, readPos, BUFFER_MASK);

            float win = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * grainPhaseR[g]));
            detunedR += sample * win;

            grainPhaseR[g] += phaseIncR;
            if (grainPhaseR[g] >= 1.0f) grainPhaseR[g] -= 1.0f;
            else if (grainPhaseR[g] < 0.0f) grainPhaseR[g] += 1.0f;
        }

        writePos = (writePos + 1) & BUFFER_MASK;

        // True stereo microshift balance: pure center anchor + spacious detuned halo
        float dryGain = 1.0f - (amt * 0.25f);
        float wetGain = amt * 0.55f;

        float wetL = inL * dryGain + detunedL * wetGain;
        float wetR = inR * dryGain + detunedR * wetGain;

        buffer.setSample(0, i, AudioUtils::sanitize(wetL));
        if (numChannels > 1)
            buffer.setSample(1, i, AudioUtils::sanitize(wetR));
    }
}

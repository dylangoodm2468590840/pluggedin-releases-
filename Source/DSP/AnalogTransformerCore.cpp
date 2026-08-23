#include "AnalogTransformerCore.h"

AnalogTransformerCore::AnalogTransformerCore() = default;

void AnalogTransformerCore::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    warmthSmoother.reset(sampleRate, 0.02);

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    for (int ch = 0; ch < 2; ++ch)
    {
        chestFilter[ch].prepare(monoSpec);
        chestFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        chestFilter[ch].setCutoffFrequency(240.0f);
        chestFilter[ch].setResonance(1.1f);

        slewFilter[ch].prepare(monoSpec);
        slewFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        slewFilter[ch].setCutoffFrequency(std::min(19500.0f, static_cast<float>(sampleRate * 0.45)));
        slewFilter[ch].setResonance(0.707f);

        dcBlocker[ch].prepare(sampleRate, 15.0f);
    }

    reset();
}

void AnalogTransformerCore::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        chestFilter[ch].reset();
        slewFilter[ch].reset();
        dcBlocker[ch].reset();
        prevSample[ch] = 0.0f;
    }
    warmthSmoother.setCurrentAndTargetValue(0.0f);
}

void AnalogTransformerCore::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;

    float currentWarmth = warmthSmoother.getTargetValue();
    if (currentWarmth <= 0.001f)
        return;

    float drive = ironDrive.load();

    for (int i = 0; i < numSamples; ++i)
    {
        float warmth = warmthSmoother.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            int chIdx = std::min(ch, 1);
            float in = AudioUtils::sanitize(buffer.getSample(ch, i));

            // 1. Isolate chest fundamental
            float chest = chestFilter[chIdx].processSample(0, in);

            // 2. Transformer Core Saturation: Symmetrical magnetic hysteresis & odd harmonics
            float chestDriven = chest * (1.0f + drive * 1.5f);
            float chestHarmonics = std::tanh(chestDriven * 1.15f) - 0.15f * std::tanh(chestDriven * 0.50f);

            // 3. Smooth blend
            float rich = in + (chestHarmonics - chest) * (warmth * 0.18f);
            rich = dcBlocker[chIdx].process(rich);
            rich = slewFilter[chIdx].processSample(0, rich);

            buffer.setSample(ch, i, AudioUtils::sanitize(rich));
        }
    }
}

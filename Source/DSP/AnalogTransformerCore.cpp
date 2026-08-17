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
        chestFilter[ch].setCutoffFrequency(280.0f);
        chestFilter[ch].setResonance(1.4f);

        slewFilter[ch].prepare(monoSpec);
        slewFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        slewFilter[ch].setCutoffFrequency(18500.0f);
        slewFilter[ch].setResonance(0.707f);

        dcBlocker[ch].reset();
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
            float in = buffer.getSample(ch, i);
            if (std::abs(in) < 1.0e-7f)
                continue;

            // 1. Isolate chest resonance fundamental (180-450Hz)
            float chest = chestFilter[chIdx].processSample(0, in);

            // 2. Transformer Core Saturation: 2nd-order even harmonics + magnetic flux compression
            float chestDriven = chest * (1.0f + drive * 2.5f);
            float chestHarmonics = chestDriven + 0.20f * (chestDriven * chestDriven) - 0.05f * std::tanh(chestDriven * 1.5f);

            // 3. Sum original signal + transformer chest weight
            float rich = in + chestHarmonics * (warmth * 0.30f);
            rich = dcBlocker[chIdx].process(rich);
            rich = slewFilter[chIdx].processSample(0, rich);

            buffer.setSample(ch, i, AudioUtils::sanitize(rich));
        }
    }
}

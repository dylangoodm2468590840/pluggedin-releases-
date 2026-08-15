#include "DeviceProcessor.h"
#include <cmath>

void DeviceProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    reset();
}

void DeviceProcessor::reset()
{
    hpFilterL.reset();
    hpFilterR.reset();
    lpFilterL.reset();
    lpFilterR.reset();
}

void DeviceProcessor::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    if (currentDevice == Off)
        return;

    auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();

    const size_t numChannels = inputBlock.getNumChannels();
    const size_t numSamples = inputBlock.getNumSamples();

    float hpCutoff = 20.0f;
    float lpCutoff = 20000.0f;

    switch (currentDevice)
    {
        case CellPhone:
            hpCutoff = 350.0f;
            lpCutoff = 3400.0f;
            break;
        case Webcam:
            hpCutoff = 500.0f;
            lpCutoff = 4500.0f;
            break;
        case Earbuds:
            hpCutoff = 250.0f;
            lpCutoff = 8000.0f;
            break;
        case Laptop:
            hpCutoff = 700.0f;
            lpCutoff = 6000.0f;
            break;
        case VoiceMemo:
            hpCutoff = 400.0f;
            lpCutoff = 3200.0f;
            break;
        default:
            break;
    }

    hpFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpCutoff);
    hpFilterR.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpCutoff);
    lpFilterL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lpCutoff);
    lpFilterR.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lpCutoff);

    for (size_t i = 0; i < numSamples; ++i)
    {
        float inL = inputBlock.getSample(0, i);
        float inR = numChannels > 1 ? inputBlock.getSample(1, i) : inL;

        // Apply lo-fi bandpass filter
        float filteredL = lpFilterL.processSample(hpFilterL.processSample(inL));
        float filteredR = lpFilterR.processSample(hpFilterR.processSample(inR));

        // Light lo-fi saturation
        float satL = std::tanh(filteredL * (1.0f + drive * 3.0f));
        float satR = std::tanh(filteredR * (1.0f + drive * 3.0f));

        outputBlock.setSample(0, i, AudioUtils::sanitize(satL));
        if (numChannels > 1)
            outputBlock.setSample(1, i, AudioUtils::sanitize(satR));
    }
}

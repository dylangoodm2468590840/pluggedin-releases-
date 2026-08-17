#include "DeviceProcessor.h"
#include <cmath>
#include <algorithm>

void DeviceProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    for (int ch = 0; ch < 2; ++ch)
    {
        hpFilter[ch].prepare(monoSpec);
        lpFilter[ch].prepare(monoSpec);
        resonancePeak[ch].prepare(monoSpec);
        bodyNotch[ch].prepare(monoSpec);
        dcBlocker[ch].reset();
    }

    reset();
}

void DeviceProcessor::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        hpFilter[ch].reset();
        lpFilter[ch].reset();
        resonancePeak[ch].reset();
        bodyNotch[ch].reset();
        dcBlocker[ch].reset();
    }
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
    float resFreq = 1800.0f;
    float resQ = 1.8f;
    float resGain = 1.0f;
    float notchFreq = 850.0f;
    float driveMult = 1.0f + drive * 4.0f;

    switch (currentDevice)
    {
        case CellPhone:
            // Narrow band + speech formant peak at 1.8kHz + miniature speaker distortion
            hpCutoff = 420.0f;
            lpCutoff = 3300.0f;
            resFreq = 1750.0f;
            resQ = 2.4f;
            resGain = 2.2f;
            driveMult = 1.5f + drive * 5.0f;
            break;

        case Webcam:
            // Muddy low cut + room boundary reflection ring at 2.4kHz
            hpCutoff = 550.0f;
            lpCutoff = 4800.0f;
            resFreq = 2400.0f;
            resQ = 3.0f;
            resGain = 1.8f;
            driveMult = 1.2f + drive * 3.5f;
            break;

        case Earbuds:
            // Low-end seal loss + 4.2kHz plastic acoustic ear canal resonance
            hpCutoff = 280.0f;
            lpCutoff = 8500.0f;
            resFreq = 4200.0f;
            resQ = 2.0f;
            resGain = 1.6f;
            driveMult = 1.1f + drive * 3.0f;
            break;

        case Laptop:
            // Thin body with notch at 850Hz and sharp 5.8kHz microphone resonance
            hpCutoff = 750.0f;
            lpCutoff = 6200.0f;
            resFreq = 3100.0f;
            resQ = 3.5f;
            resGain = 2.5f;
            notchFreq = 850.0f;
            driveMult = 1.8f + drive * 6.0f;
            break;

        case VoiceMemo:
            // Vintage portable cassette recorder: 320Hz to 3.8kHz + magnetic core saturation
            hpCutoff = 340.0f;
            lpCutoff = 3900.0f;
            resFreq = 1400.0f;
            resQ = 1.5f;
            resGain = 1.4f;
            driveMult = 1.4f + drive * 4.5f;
            break;

        default:
            break;
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        hpFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        hpFilter[ch].setCutoffFrequency(hpCutoff);
        hpFilter[ch].setResonance(0.707f);

        lpFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        lpFilter[ch].setCutoffFrequency(lpCutoff);
        lpFilter[ch].setResonance(0.707f);

        resonancePeak[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        resonancePeak[ch].setCutoffFrequency(resFreq);
        resonancePeak[ch].setResonance(resQ);
    }

    for (size_t i = 0; i < numSamples; ++i)
    {
        for (size_t ch = 0; ch < numChannels; ++ch)
        {
            int chIdx = std::min((int)ch, 1);
            float in = inputBlock.getSample(ch, i);

            if (std::abs(in) < 1.0e-7f)
            {
                outputBlock.setSample(ch, i, 0.0f);
                continue;
            }

            // 1. Acoustic bandpass filtering
            float filtered = hpFilter[chIdx].processSample(0, in);
            filtered = lpFilter[chIdx].processSample(0, filtered);

            // 2. Transducer body resonance injection
            float res = resonancePeak[chIdx].processSample(0, filtered) * (resGain - 1.0f);
            float acousticSig = filtered + res;

            // 3. Nonlinear miniature capsule & cone saturation
            float driven = acousticSig * driveMult;
            float sat = std::tanh(driven * 1.2f) - 0.15f * std::tanh(driven * driven * driven * 0.4f);

            // 4. DC blocker & normalization
            sat = dcBlocker[chIdx].process(sat);
            float out = AudioUtils::sanitize(sat * 0.85f);

            outputBlock.setSample(ch, i, out);
        }
    }
}

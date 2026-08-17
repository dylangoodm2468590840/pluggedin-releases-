#include "VocalResonanceProcessor.h"

VocalResonanceProcessor::VocalResonanceProcessor()
{
    // Calibrated vocal resonance target bands
    bands[0].centerFreq = 340.0f;   // Boxy / Mud buildup (critical when pitch shifted down)
    bands[0].qFactor    = 2.2f;

    bands[1].centerFreq = 1200.0f;  // Nasal / Honk
    bands[1].qFactor    = 2.6f;

    bands[2].centerFreq = 3500.0f;  // Upper-Mid Harshness / Piercing sibilance
    bands[2].qFactor    = 3.0f;

    bands[3].centerFreq = 7400.0f;  // Sibilant Whistle / High Edge
    bands[3].qFactor    = 2.8f;

    for (int b = 0; b < NUM_BANDS; ++b)
        bandReductionDb[b].store(0.0f);
}

void VocalResonanceProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    smoothedAmount.reset(sampleRate, 0.02);
    smoothedAmount.setCurrentAndTargetValue(amount.load());

    juce::dsp::ProcessSpec monoSpec = spec;
    monoSpec.numChannels = 1;

    for (int b = 0; b < NUM_BANDS; ++b)
    {
        // Fast 3ms attack for instant transient resonance catching; 40ms release
        bands[b].attackCoeff = std::exp(-1.0f / (0.003f * static_cast<float>(sampleRate)));
        bands[b].releaseCoeff = std::exp(-1.0f / (0.040f * static_cast<float>(sampleRate)));

        for (int ch = 0; ch < 2; ++ch)
        {
            bands[b].detectorBp[ch].prepare(monoSpec);
            bands[b].detectorBp[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            bands[b].detectorBp[ch].setCutoffFrequency(bands[b].centerFreq);
            bands[b].detectorBp[ch].setResonance(bands[b].qFactor);

            bands[b].dynamicCutBiquad[ch].reset();
            bands[b].dynamicCutBiquad[ch].updateBellCoeffs(sampleRate, bands[b].centerFreq, 0.0, bands[b].qFactor);
        }
    }

    reset();
}

void VocalResonanceProcessor::reset()
{
    for (int b = 0; b < NUM_BANDS; ++b)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            bands[b].detectorBp[ch].reset();
            bands[b].dynamicCutBiquad[ch].reset();
            bands[b].detectorEnv[ch] = 0.0f;
            bands[b].broadEnv[ch] = 0.0f;
            bands[b].currentGainDb[ch] = 0.0f;
        }
        bandReductionDb[b].store(0.0f);
    }
    maxReductionDb.store(0.0f);
}

void VocalResonanceProcessor::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;

    float currentAmt = amount.load();
    smoothedAmount.setTargetValue(currentAmt);

    if (currentAmt <= 0.001f)
    {
        maxReductionDb.store(0.0f);
        for (int b = 0; b < NUM_BANDS; ++b)
            bandReductionDb[b].store(0.0f);
        return;
    }

    float currentFocus = focus.load();
    float peakReduction = 0.0f;

    // Band sensitivity weights based on Focus parameter
    float bandWeights[NUM_BANDS];
    bandWeights[0] = 1.30f * (1.0f - currentFocus * 0.4f); // Low-mid mud
    bandWeights[1] = 1.05f;                                // Nasality
    bandWeights[2] = 1.20f * (0.6f + currentFocus * 0.8f); // Harshness
    bandWeights[3] = 1.10f * (0.5f + currentFocus * 0.9f); // Whistle / Fizz

    for (int b = 0; b < NUM_BANDS; ++b)
    {
        if (!bandsActive[b].load())
        {
            bandReductionDb[b].store(0.0f);
            continue;
        }

        float attCoeff = bands[b].attackCoeff;
        float relCoeff = bands[b].releaseCoeff;
        float maxBandDb = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            int channelIdx = std::min(ch, 1);
            float* channelData = buffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                float inSample = channelData[i];
                float absIn = std::abs(inSample);

                // 1. Broad-band vocal energy tracker
                float broadEnv = bands[b].broadEnv[channelIdx];
                if (absIn > broadEnv) broadEnv = attCoeff * broadEnv + (1.0f - attCoeff) * absIn;
                else                  broadEnv = relCoeff * broadEnv + (1.0f - relCoeff) * absIn;
                bands[b].broadEnv[channelIdx] = AudioUtils::sanitize(broadEnv);

                // 2. Narrow-band resonant envelope detector
                float bpSample = bands[b].detectorBp[channelIdx].processSample(0, inSample);
                float absBp = std::abs(bpSample) * 1.6f;

                float detEnv = bands[b].detectorEnv[channelIdx];
                if (absBp > detEnv) detEnv = attCoeff * detEnv + (1.0f - attCoeff) * absBp;
                else                detEnv = relCoeff * detEnv + (1.0f - relCoeff) * absBp;
                bands[b].detectorEnv[channelIdx] = AudioUtils::sanitize(detEnv);

                // 3. Peak-to-Broadband Resonance Ratio
                float ratio = (detEnv + 1.0e-5f) / (broadEnv + 1.0e-5f);
                float targetCutDb = 0.0f;

                if (ratio > 1.15f && detEnv > 0.003f)
                {
                    float excessDb = 20.0f * std::log10(ratio / 1.15f);
                    float reductionDb = excessDb * currentAmt * bandWeights[b] * 1.5f;
                    targetCutDb = std::min(reductionDb, 9.0f); // Max 9dB dynamic notch cut
                }

                // Smooth gain movement
                bands[b].currentGainDb[channelIdx] = 0.88f * bands[b].currentGainDb[channelIdx] + 0.12f * targetCutDb;
                float curCutDb = bands[b].currentGainDb[channelIdx];

                if (curCutDb > maxBandDb)
                    maxBandDb = curCutDb;

                // 4. Apply True Minimum-Phase Dynamic Bell Cut
                bands[b].dynamicCutBiquad[channelIdx].updateBellCoeffs(
                    sampleRate, 
                    bands[b].centerFreq, 
                    -static_cast<double>(curCutDb), 
                    bands[b].qFactor
                );

                float processed = bands[b].dynamicCutBiquad[channelIdx].process(inSample);
                channelData[i] = AudioUtils::sanitize(processed);
            }
        }

        bandReductionDb[b].store(maxBandDb);
        if (maxBandDb > peakReduction)
            peakReduction = maxBandDb;
    }

    maxReductionDb.store(peakReduction);
}

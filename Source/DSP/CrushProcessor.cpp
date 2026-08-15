#include "CrushProcessor.h"
#include <cmath>
#include <algorithm>

CrushProcessor::CrushProcessor()
{
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
}

CrushProcessor::~CrushProcessor() = default;

void CrushProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
    
    amountSmoother.reset(sampleRate, 0.02);
    toneSmoother.reset(sampleRate, 0.02);
    mixSmoother.reset(sampleRate, 0.02);

    if (oversampler)
        oversampler->initProcessing(spec.maximumBlockSize);

    juce::dsp::ProcessSpec filterSpec = spec;
    if (oversamplingEnabled.load() && oversampler)
        filterSpec.sampleRate = sampleRate * 2.0;

    for (int ch = 0; ch < 2; ++ch)
    {
        preEmphasisFilter[ch].prepare(filterSpec);
        preEmphasisFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        preEmphasisFilter[ch].setCutoffFrequency(2500.0f);
        preEmphasisFilter[ch].setResonance(0.707f);

        deEmphasisFilter[ch].prepare(filterSpec);
        deEmphasisFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        deEmphasisFilter[ch].setCutoffFrequency(6000.0f);
        deEmphasisFilter[ch].setResonance(0.707f);

        tapeHeadBumpFilter[ch].prepare(filterSpec);
        tapeHeadBumpFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        tapeHeadBumpFilter[ch].setCutoffFrequency(65.0f);
        tapeHeadBumpFilter[ch].setResonance(1.4f);

        airExciterFilter[ch].prepare(filterSpec);
        airExciterFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        airExciterFilter[ch].setCutoffFrequency(8500.0f);
        airExciterFilter[ch].setResonance(0.707f);

        lowpassFilter[ch].prepare(filterSpec);
        lowpassFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        lowpassFilter[ch].setResonance(0.707f);

        highpassFilter[ch].prepare(filterSpec);
        highpassFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        highpassFilter[ch].setResonance(0.707f);
    }

    reset();
}

void CrushProcessor::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        holdSample[ch] = 0.0f;
        sampleCounter[ch] = 0.0f;
        preEmphasisFilter[ch].reset();
        deEmphasisFilter[ch].reset();
        tapeHeadBumpFilter[ch].reset();
        airExciterFilter[ch].reset();
        lowpassFilter[ch].reset();
        highpassFilter[ch].reset();
    }
    if (oversampler)
        oversampler->reset();

    amountSmoother.setCurrentAndTargetValue(amountSmoother.getTargetValue());
    toneSmoother.setCurrentAndTargetValue(toneSmoother.getTargetValue());
    mixSmoother.setCurrentAndTargetValue(mixSmoother.getTargetValue());
}

float CrushProcessor::processSample(float inputSample, int channel, float currentAmount, float currentTone)
{
    float sample = AudioUtils::sanitize(inputSample);
    
    if (currentAmount <= 0.0001f || std::abs(sample) < 1.0e-9f)
        return sample;

    int ch = std::min(channel, 1);
    float processed = sample;
    Character currentCharacter = character.load();

    switch (currentCharacter)
    {
        case Character::SoftClip:
        {
            // 1. 12AX7 Triode Tube Warmth & Asymmetric Even-Order Harmonics
            float drive = 1.0f + currentAmount * 10.0f;
            float x = sample * drive;
            
            // Asymmetric triode transfer function with soft saturation curve
            float tubeSat = (x + 0.22f * x * x) / (1.0f + 0.22f * x * x + std::abs(x * 0.8f));
            
            // Dynamic Auto-Gain Compensation
            float autoGainComp = 1.0f / std::sqrt(1.0f + 1.2f * currentAmount * currentAmount);
            processed = tubeSat * autoGainComp;
            break;
        }

        case Character::Bitcrusher:
        {
            // 2. Vintage Digital Sampler (E-mu SP-1200 / Akai MPC60 Style Decimation)
            float bits = 16.0f - currentAmount * 10.5f;
            float steps = std::pow(2.0f, std::max(bits, 4.0f));
            float quantized = std::round(sample * steps) / steps;

            float holdPeriod = 1.0f + currentAmount * 10.0f;
            sampleCounter[ch] += 1.0f;
            if (sampleCounter[ch] >= holdPeriod)
            {
                sampleCounter[ch] -= holdPeriod;
                holdSample[ch] = quantized;
            }

            float alpha = std::clamp(sampleCounter[ch] / holdPeriod, 0.0f, 1.0f);
            processed = holdSample[ch] * (1.0f - alpha * 0.12f);
            break;
        }

        case Character::Overdrive:
        {
            // 3. Studer A800 Magnetic Tape Saturation & Low-End Head Bump
            float drive = 1.0f + currentAmount * 12.0f;
            float x = sample * drive;

            // Tape hysteresis S-curve
            float tapeSat = std::tanh(x * 0.85f) + 0.15f * std::sin(x * 1.5f);
            
            // Add analog 60Hz magnetic head bump resonance for fat vocal body
            float headBump = tapeHeadBumpFilter[ch].processSample(ch, sample) * (currentAmount * 0.45f);
            
            float autoGainComp = 1.0f / std::sqrt(1.0f + 1.4f * currentAmount * currentAmount);
            processed = (tapeSat + headBump) * autoGainComp;
            break;
        }

        case Character::ParallelFuzz:
        {
            // 4. Studio Germanium Diode Fuzz with Intact Dry Vocal Dynamics
            float drive = 1.0f + currentAmount * 15.0f;
            float x = sample * drive;
            
            // Germanium hard-soft diode conduction
            float diodeFuzz = (x > 0.0f) ? std::tanh(x * 1.2f) : -std::tanh(std::abs(x) * 0.9f);
            
            float autoGainComp = 1.0f / std::sqrt(1.0f + 1.8f * currentAmount * currentAmount);
            processed = (sample * 0.35f + diodeFuzz * 0.65f) * autoGainComp;
            break;
        }
    }

    // 5. Slate Fresh Air Style Dynamic Air Exciter
    // When tone > 0.65, dynamically adds high-frequency harmonic sheen (10kHz - 18kHz)
    if (currentTone > 0.65f)
    {
        float airAmount = (currentTone - 0.65f) / 0.35f; // 0.0 to 1.0
        float airHighs = airExciterFilter[ch].processSample(ch, sample);
        // Generate pure 2nd & 3rd harmonics in the ultra-highs
        float exciterHarmonics = (airHighs * airHighs * 2.0f) - 0.05f;
        processed += exciterHarmonics * (airAmount * 0.28f);
    }

    return AudioUtils::sanitize(processed);
}

void CrushProcessor::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    float currentAmount = amountSmoother.getNextValue();
    float currentTone = toneSmoother.getNextValue();
    float currentMix = mixSmoother.getNextValue();

    if (currentAmount <= 0.0001f && amountSmoother.getTargetValue() <= 0.0001f)
        return;

    Character currentCharacter = character.load();
    bool useOversampling = oversamplingEnabled.load() && oversampler && (currentCharacter != Character::Bitcrusher);
    double targetRate = useOversampling ? (sampleRate * 2.0) : sampleRate;
    float maxNyquist = static_cast<float>(targetRate * 0.49);

    float lpCutoff = maxNyquist;
    float hpCutoff = 20.0f;

    if (currentTone < 0.5f)
    {
        float normTone = currentTone * 2.0f;
        lpCutoff = 400.0f + normTone * normTone * (maxNyquist - 400.0f);
        hpCutoff = 20.0f;
    }
    else
    {
        float normTone = (currentTone - 0.5f) * 2.0f;
        hpCutoff = 20.0f + normTone * normTone * 1200.0f;
        lpCutoff = maxNyquist;
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        lowpassFilter[ch].setCutoffFrequency(std::clamp(lpCutoff, 20.0f, maxNyquist));
        highpassFilter[ch].setCutoffFrequency(std::clamp(hpCutoff, 20.0f, maxNyquist));
    }

    if (useOversampling)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::AudioBlock<float> oversampledBlock = oversampler->processSamplesUp(block);

        const size_t osChannels = oversampledBlock.getNumChannels();
        const size_t osSamples = oversampledBlock.getNumSamples();

        for (size_t ch = 0; ch < osChannels; ++ch)
        {
            float* channelData = oversampledBlock.getChannelPointer(ch);
            int stateCh = std::min((int)ch, 1);

            for (size_t i = 0; i < osSamples; ++i)
            {
                float drySample = channelData[i];
                float wetSample = processSample(drySample, stateCh, currentAmount, currentTone);

                wetSample = lowpassFilter[stateCh].processSample(stateCh, wetSample);
                wetSample = highpassFilter[stateCh].processSample(stateCh, wetSample);

                float outputSample = drySample * (1.0f - currentMix) + wetSample * currentMix;
                channelData[i] = AudioUtils::sanitize(outputSample);
            }
        }

        oversampler->processSamplesDown(block);
    }
    else
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            int stateCh = std::min(ch, 1);

            for (int i = 0; i < numSamples; ++i)
            {
                float drySample = channelData[i];
                float wetSample = processSample(drySample, stateCh, currentAmount, currentTone);

                wetSample = lowpassFilter[stateCh].processSample(stateCh, wetSample);
                wetSample = highpassFilter[stateCh].processSample(stateCh, wetSample);

                float outputSample = drySample * (1.0f - currentMix) + wetSample * currentMix;
                channelData[i] = AudioUtils::sanitize(outputSample);
            }
        }
    }

    // Final output sanitization sweep
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            channelData[i] = AudioUtils::sanitize(channelData[i]);
        }
    }
}

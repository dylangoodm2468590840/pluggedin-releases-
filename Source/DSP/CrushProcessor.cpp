#include "CrushProcessor.h"
#include <cmath>
#include <algorithm>

CrushProcessor::CrushProcessor()
{
    // 2-Stage Cascade = 4x Polyphase Minimum-Phase Oversampling
    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
}

CrushProcessor::~CrushProcessor() = default;

void CrushProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
    
    amountSmoother.reset(sampleRate, 0.02);
    toneSmoother.reset(sampleRate, 0.02);
    mixSmoother.reset(sampleRate, 0.02);
    punishGainSmoother.reset(sampleRate, 0.02);

    if (oversampler)
        oversampler->initProcessing(spec.maximumBlockSize);

    // 4x Oversampled processing rate for internal analog modeling filters
    juce::dsp::ProcessSpec filterSpec = spec;
    if (oversamplingEnabled.load() && oversampler)
        filterSpec.sampleRate = sampleRate * 4.0;

    for (int ch = 0; ch < 2; ++ch)
    {
        lowCrossoverFilter[ch].prepare(filterSpec);
        lowCrossoverFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        lowCrossoverFilter[ch].setCutoffFrequency(140.0f);
        lowCrossoverFilter[ch].setResonance(0.707f);

        highCrossoverFilter[ch].prepare(filterSpec);
        highCrossoverFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        highCrossoverFilter[ch].setCutoffFrequency(140.0f);
        highCrossoverFilter[ch].setResonance(0.707f);

        tapeHeadBumpFilter[ch].prepare(filterSpec);
        tapeHeadBumpFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        tapeHeadBumpFilter[ch].setCutoffFrequency(60.0f);
        tapeHeadBumpFilter[ch].setResonance(1.8f);

        lowpassFilter[ch].prepare(filterSpec);
        lowpassFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        lowpassFilter[ch].setResonance(0.707f);

        dcBlocker[ch].reset();
    }

    reset();
}

void CrushProcessor::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        tubeBiasSag[ch] = 0.0f;
        tapeHysteresis[ch] = 0.0f;
        lowCrossoverFilter[ch].reset();
        highCrossoverFilter[ch].reset();
        tapeHeadBumpFilter[ch].reset();
        lowpassFilter[ch].reset();
        dcBlocker[ch].reset();
    }
    if (oversampler)
        oversampler->reset();

    amountSmoother.setCurrentAndTargetValue(amountSmoother.getTargetValue());
    toneSmoother.setCurrentAndTargetValue(toneSmoother.getTargetValue());
    mixSmoother.setCurrentAndTargetValue(mixSmoother.getTargetValue());
    punishGainSmoother.setCurrentAndTargetValue(0.0f);
}

float CrushProcessor::processSample(float inputSample, int channel, float currentAmount, float currentTone, bool isPunished, float transProt)
{
    float sample = AudioUtils::sanitize(inputSample);
    
    if (std::abs(sample) < 1.0e-7f || (currentAmount <= 0.0001f && !isPunished))
        return 0.0f;

    int ch = std::min(channel, 1);
    Character currentCharacter = character.load();

    // Consonant & Transient Attack Protection: Soften drive slightly during aggressive plosives
    float driveMod = 1.0f - 0.20f * transProt;

    // PUNISH Mode: +20dB Analog Input Blast with level management
    float driveBoost = (isPunished ? 7.5f : 1.0f) * driveMod;
    float postPad    = isPunished ? 0.18f : 1.0f;

    float processed = sample;

    switch (currentCharacter)
    {
        case Character::Tube12AX7:
        {
            // 1. 12AX7 Class-A Triode Tube: Asymmetric 2nd-order harmonics with smooth bias curve
            float drive = (1.0f + currentAmount * 7.5f) * driveBoost;
            float x = sample * drive;

            // Dynamic cathode bias sag tracking input energy
            tubeBiasSag[ch] += 0.0005f * (std::abs(x) - tubeBiasSag[ch]);
            float bias = 0.15f * (1.0f - std::clamp(tubeBiasSag[ch], 0.0f, 0.8f));
            
            // Continuous quadratic harmonic injection without zero-crossing cusps
            float x_asym = x + bias * std::tanh(x * x);
            float sat = std::tanh(x_asym);

            processed = dcBlocker[ch].process(sat) / std::sqrt(1.0f + drive * 0.35f) * postPad;
            break;
        }

        case Character::PentodeEL34:
        {
            // 2. EL34 Push-Pull Pentode Tube: Symmetrical biting 3rd & 5th order harmonics
            float drive = (1.0f + currentAmount * 9.0f) * driveBoost;
            float x = sample * drive;

            // Symmetrical polynomial transfer function (C-infinity smooth)
            float sat = std::tanh(x) - 0.08f * std::tanh(x * x * x);
            processed = sat / std::sqrt(1.0f + drive * 0.40f) * postPad;
            break;
        }

        case Character::TapeAmpex:
        {
            // 3. Ampex 350 Magnetic Tape: 60Hz head bump + hysteresis
            float drive = (1.0f + currentAmount * 6.5f) * driveBoost;
            float bump = tapeHeadBumpFilter[ch].processSample(0, sample) * (0.25f * currentAmount);
            float x = (sample + bump) * drive;

            // Magnetic tape hysteresis approximation
            float delta = x - tapeHysteresis[ch];
            tapeHysteresis[ch] += 0.35f * delta * (1.0f - 0.25f * std::tanh(delta * delta));

            float sat = std::tanh(tapeHysteresis[ch] * 1.30f);
            processed = sat / std::sqrt(1.0f + drive * 0.30f) * postPad;
            break;
        }

        case Character::Germanium:
        {
            // 4. Germanium Transistor: Vintage Neve 1057 Console Preamp Overdrive
            float drive = (1.0f + currentAmount * 8.5f) * driveBoost;
            float x = sample * drive;

            // Smooth algebraic soft-clipper (infinitely smooth everywhere)
            float x_shaped = x + 0.12f * std::tanh(x * 1.5f);
            float sat = x_shaped / std::sqrt(1.0f + x_shaped * x_shaped * 0.75f);

            processed = dcBlocker[ch].process(sat) / std::sqrt(1.0f + drive * 0.38f) * postPad;
            break;
        }

        case Character::CyberFuzz:
        {
            // 5. Cyber Fuzz: Modern saturated presence & edge
            float drive = (1.0f + currentAmount * 12.0f) * driveBoost;
            float x = sample * drive;

            float fuzz = std::tanh(x + 0.30f * std::tanh(x * 2.0f));
            processed = fuzz / std::sqrt(1.0f + drive * 0.50f) * postPad;
            break;
        }
    }

    // Dynamic Tone Sculpting Lowpass
    float lpCutoff = 2200.0f + currentTone * 16500.0f;
    lowpassFilter[ch].setCutoffFrequency(std::clamp(lpCutoff, 400.0f, (float)(sampleRate * 1.8)));
    processed = lowpassFilter[ch].processSample(0, processed);

    return AudioUtils::sanitize(processed);
}

void CrushProcessor::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;

    bool isPunished = punishEnabled.load();
    float transProt = transientProtection.load();

    float curAmount = amountSmoother.getTargetValue();
    float curTone   = toneSmoother.getTargetValue();
    float curMix    = mixSmoother.getTargetValue();

    if (curAmount <= 0.001f && !isPunished)
        return;

    if (oversamplingEnabled.load() && oversampler)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::AudioBlock<float> oversampledBlock = oversampler->processSamplesUp(block);

        const size_t osSamples = oversampledBlock.getNumSamples();
        const size_t osChannels = oversampledBlock.getNumChannels();

        for (size_t sample = 0; sample < osSamples; ++sample)
        {
            for (size_t ch = 0; ch < osChannels; ++ch)
            {
                float dry = oversampledBlock.getSample(ch, sample);
                float wet = processSample(dry, static_cast<int>(ch), curAmount, curTone, isPunished, transProt);
                oversampledBlock.setSample(ch, sample, dry * (1.0f - curMix) + wet * curMix);
            }
        }

        oversampler->processSamplesDown(block);
    }
    else
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float amt = amountSmoother.getNextValue();
            float tone = toneSmoother.getNextValue();
            float mix = mixSmoother.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float dry = buffer.getSample(ch, sample);
                float wet = processSample(dry, ch, amt, tone, isPunished, transProt);
                buffer.setSample(ch, sample, dry * (1.0f - mix) + wet * mix);
            }
        }
    }
}

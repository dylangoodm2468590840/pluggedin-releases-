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
        preEmphasisFilter[ch].prepare(filterSpec);
        preEmphasisFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        preEmphasisFilter[ch].setCutoffFrequency(2400.0f);
        preEmphasisFilter[ch].setResonance(0.707f);

        deEmphasisFilter[ch].prepare(filterSpec);
        deEmphasisFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        deEmphasisFilter[ch].setCutoffFrequency(5500.0f);
        deEmphasisFilter[ch].setResonance(0.707f);

        tapeHeadBumpFilter[ch].prepare(filterSpec);
        tapeHeadBumpFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        tapeHeadBumpFilter[ch].setCutoffFrequency(60.0f);
        tapeHeadBumpFilter[ch].setResonance(1.8f);

        lowpassFilter[ch].prepare(filterSpec);
        lowpassFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        lowpassFilter[ch].setResonance(0.707f);

        highpassFilter[ch].prepare(filterSpec);
        highpassFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        highpassFilter[ch].setResonance(0.707f);

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
        preEmphasisFilter[ch].reset();
        deEmphasisFilter[ch].reset();
        tapeHeadBumpFilter[ch].reset();
        lowpassFilter[ch].reset();
        highpassFilter[ch].reset();
        dcBlocker[ch].reset();
    }
    if (oversampler)
        oversampler->reset();

    amountSmoother.setCurrentAndTargetValue(amountSmoother.getTargetValue());
    toneSmoother.setCurrentAndTargetValue(toneSmoother.getTargetValue());
    mixSmoother.setCurrentAndTargetValue(mixSmoother.getTargetValue());
    punishGainSmoother.setCurrentAndTargetValue(0.0f);
}

float CrushProcessor::processSample(float inputSample, int channel, float currentAmount, float currentTone, bool isPunished)
{
    float sample = AudioUtils::sanitize(inputSample);
    
    if (currentAmount <= 0.0001f && !isPunished)
        return sample;

    int ch = std::min(channel, 1);
    float processed = sample;
    Character currentCharacter = character.load();

    // PUNISH Mode: +20dB Analog Input Blast (10x Drive multiplier)
    float driveBoost = isPunished ? 10.0f : 1.0f;
    float postPad    = isPunished ? 0.12f : 1.0f;

    switch (currentCharacter)
    {
        case Character::Tube12AX7:
        {
            // 1. 12AX7 Class-A Triode Tube: Asymmetrical 2nd-Order Warmth + Dynamic Cathode Bias Sag
            float drive = (1.0f + currentAmount * 12.0f) * driveBoost;
            float x = sample * drive;

            // Dynamic Cathode Bias Sag tracking input energy
            tubeBiasSag[ch] += 0.0005f * (std::abs(x) - tubeBiasSag[ch]);
            float bias = 0.25f - 0.15f * std::clamp(tubeBiasSag[ch], 0.0f, 1.0f);
            float x_biased = x + bias;

            // Asymmetrical Triode Transfer Function
            if (x_biased >= 0.0f)
                processed = x_biased / (1.0f + x_biased * 0.85f);
            else
                processed = std::tanh(x_biased * 1.35f);

            // Remove DC offset produced by asymmetric saturation
            processed = dcBlocker[ch].process(processed);

            // Dynamic Tube Output Normalization
            float autoGain = (1.0f / std::sqrt(1.0f + drive * 0.45f)) * postPad;
            processed *= autoGain;
            break;
        }

        case Character::PentodeEL34:
        {
            // 2. EL34 Push-Pull Pentode Tube: Symmetrical Aggressive 3rd & 5th Order Harmonics
            float drive = (1.0f + currentAmount * 14.0f) * driveBoost;
            float x = sample * drive;

            // Pentode Polynomial Transfer Curve
            float x_clamped = std::clamp(x * 0.75f, -3.0f, 3.0f);
            processed = std::tanh(x_clamped) - 0.08f * std::tanh(x_clamped * x_clamped * x_clamped);

            float autoGain = (1.0f / std::sqrt(1.0f + drive * 0.50f)) * postPad;
            processed *= autoGain;
            break;
        }

        case Character::TapeAmpex:
        {
            // 3. Ampex 350 Magnetic Tape: Hysteresis + 60Hz Head Bump + High-End Soft Compression
            float drive = (1.0f + currentAmount * 9.0f) * driveBoost;
            
            // Add 60Hz Head-Bump resonance
            float bump = tapeHeadBumpFilter[ch].processSample(ch, sample) * (0.35f * currentAmount);
            float x = (sample + bump) * drive;

            // Magnetic Tape Hysteresis Approximation
            float delta = x - tapeHysteresis[ch];
            tapeHysteresis[ch] += 0.40f * delta * (1.0f - 0.45f * std::tanh(delta * delta));

            processed = std::tanh(tapeHysteresis[ch] * 1.45f) * 0.92f;

            float autoGain = (1.0f / std::sqrt(1.0f + drive * 0.38f)) * postPad;
            processed *= autoGain;
            break;
        }

        case Character::Germanium:
        {
            // 4. Germanium Transistor: Vintage Neve 1057 Console Preamp Overdrive
            float drive = (1.0f + currentAmount * 11.0f) * driveBoost;
            float x = sample * drive;

            // Asymmetric Germanium Diode Knee
            if (x > 0.0f)
                processed = 1.0f - std::exp(-x * 0.95f);
            else
                processed = -(1.0f - std::exp(x * 0.70f)) * 1.15f;

            processed = dcBlocker[ch].process(processed);

            float autoGain = (1.0f / std::sqrt(1.0f + drive * 0.42f)) * postPad;
            processed *= autoGain;
            break;
        }

        case Character::CyberFuzz:
        {
            // 5. Cyber Fuzz: Full-Wave Rectified Octave Fuzz with Dry Fundamental Mix
            float drive = (1.0f + currentAmount * 18.0f) * driveBoost;
            float x = sample * drive;

            float fuzz = std::tanh(x + 0.35f * std::tanh(x * 2.2f));
            float octave = 0.20f * std::sin(std::clamp(x * 1.57f, -3.14f, 3.14f));
            processed = 0.75f * fuzz + 0.25f * octave;

            float autoGain = (1.0f / std::sqrt(1.0f + drive * 0.60f)) * postPad;
            processed *= autoGain;
            break;
        }
    }

    // Dynamic Tone Sculpting
    float lpCutoff = 1500.0f + currentTone * 16500.0f;
    lowpassFilter[ch].setCutoffFrequency(std::clamp(lpCutoff, 200.0f, 20000.0f));
    processed = lowpassFilter[ch].processSample(ch, processed);

    return AudioUtils::sanitize(processed);
}

void CrushProcessor::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = std::min(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    if (numSamples == 0 || numChannels == 0)
        return;

    bool isOversampled = oversamplingEnabled.load() && (oversampler != nullptr);
    bool punished = punishEnabled.load();

    if (isOversampled)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::AudioBlock<float> oversampledBlock = oversampler->processSamplesUp(block);

        const int osSamples = (int)oversampledBlock.getNumSamples();

        for (int i = 0; i < osSamples; ++i)
        {
            float amt = amountSmoother.getNextValue();
            float tone = toneSmoother.getNextValue();
            float mix = mixSmoother.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* channelData = oversampledBlock.getChannelPointer(ch);
                float drySample = channelData[i];
                float wetSample = processSample(drySample, ch, amt, tone, punished);
                channelData[i] = drySample * (1.0f - mix) + wetSample * mix;
            }
        }

        oversampler->processSamplesDown(block);
    }
    else
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float amt = amountSmoother.getNextValue();
            float tone = toneSmoother.getNextValue();
            float mix = mixSmoother.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float drySample = buffer.getSample(ch, i);
                float wetSample = processSample(drySample, ch, amt, tone, punished);
                buffer.setSample(ch, i, drySample * (1.0f - mix) + wetSample * mix);
            }
        }
    }
}

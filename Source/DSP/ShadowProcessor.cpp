#include "ShadowProcessor.h"
#include <cmath>
#include <algorithm>

ShadowProcessor::ShadowProcessor()
{
    grainBufferL.assign(BUFFER_SIZE, 0.0f);
    grainBufferR.assign(BUFFER_SIZE, 0.0f);
}

ShadowProcessor::~ShadowProcessor() = default;

void ShadowProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
    
    grainBufferL.assign(BUFFER_SIZE, 0.0f);
    grainBufferR.assign(BUFFER_SIZE, 0.0f);
    writeIndex = 0;

    for (int ch = 0; ch < 2; ++ch)
    {
        // 1. Pharynx Formant Resonator F1 (300Hz - 850Hz)
        formantF1[ch].prepare(spec);
        formantF1[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        formantF1[ch].setResonance(2.2f);

        // 2. Oral Cavity Formant Resonator F2 (900Hz - 2200Hz)
        formantF2[ch].prepare(spec);
        formantF2[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        formantF2[ch].setResonance(2.0f);

        // 3. Singing / Throat Formant Resonator F3 (2400Hz - 3600Hz)
        formantF3[ch].prepare(spec);
        formantF3[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        formantF3[ch].setResonance(1.8f);

        // Darkness Lowpass
        darknessFilter[ch].prepare(spec);
        darknessFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        darknessFilter[ch].setResonance(0.707f);

        // Sub Warmth Lowpass (for clean chest sub bass)
        subWarmthFilter[ch].prepare(spec);
        subWarmthFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        subWarmthFilter[ch].setCutoffFrequency(160.0f);
        subWarmthFilter[ch].setResonance(0.707f);
    }

    reset();
}

void ShadowProcessor::reset()
{
    std::fill(grainBufferL.begin(), grainBufferL.end(), 0.0f);
    std::fill(grainBufferR.begin(), grainBufferR.end(), 0.0f);
    writeIndex = 0;

    for (int ch = 0; ch < 2; ++ch)
    {
        grainPhase[ch][0] = 0.00f;
        grainPhase[ch][1] = 0.25f;
        grainPhase[ch][2] = 0.50f;
        grainPhase[ch][3] = 0.75f;

        formantF1[ch].reset();
        formantF2[ch].reset();
        formantF3[ch].reset();
        darknessFilter[ch].reset();
        subWarmthFilter[ch].reset();
        dcBlocker[ch].reset();
    }
}

inline float ShadowProcessor::readHermite(const float* buffer, float delaySamples, int writePos, int mask) noexcept
{
    float readPos = static_cast<float>(writePos) - delaySamples;
    while (readPos < 0.0f) readPos += static_cast<float>(BUFFER_SIZE);

    int i1 = static_cast<int>(readPos) & mask;
    float frac = readPos - static_cast<float>(static_cast<int>(readPos));

    int i0 = (i1 - 1 + BUFFER_SIZE) & mask;
    int i2 = (i1 + 1) & mask;
    int i3 = (i1 + 2) & mask;

    float y0 = buffer[i0];
    float y1 = buffer[i1];
    float y2 = buffer[i2];
    float y3 = buffer[i3];

    // 3rd-order Cubic Hermite Spline
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

float ShadowProcessor::processSample(float inputSample, int channel)
{
    float sample = AudioUtils::sanitize(inputSample);
    int ch = std::min(channel, 1);

    // Calculate target pitch ratio based on interval
    float pitchRatio = 0.5f; // Default -12 semitones
    switch (pitchInterval)
    {
        case PitchInterval::OctaveDown: pitchRatio = 0.5000f; break; // -12 semitones
        case PitchInterval::FifthDown:  pitchRatio = 0.6674f; break; // -7 semitones
        case PitchInterval::FourthDown: pitchRatio = 0.7491f; break; // -5 semitones
        case PitchInterval::TwoOctaves: pitchRatio = 0.2500f; break; // -24 semitones
    }

    // Write to circular delay buffer
    float* buf = (ch == 0) ? grainBufferL.data() : grainBufferR.data();
    buf[writeIndex] = sample;

    // 4-Head Overlapping Granular Math with Cubic Hermite Interpolation
    const float grainSizeSamples = 2048.0f; // ~46ms grain window for vocal pitch shifting
    float phaseAdvance = (1.0f - pitchRatio) / grainSizeSamples;

    float grainAccum = 0.0f;
    float windowSum = 0.0f;

    for (int g = 0; g < 4; ++g)
    {
        float phase = grainPhase[ch][g]; // 0.0 to 1.0
        float delay = phase * grainSizeSamples;

        // Smooth raised cosine (Hann) envelope
        float win = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));

        float grainSample = readHermite(buf, delay, writeIndex, BUFFER_MASK);
        grainAccum += grainSample * win;
        windowSum += win;

        // Advance grain phase
        grainPhase[ch][g] += phaseAdvance;
        if (grainPhase[ch][g] >= 1.0f)
            grainPhase[ch][g] -= 1.0f;
        else if (grainPhase[ch][g] < 0.0f)
            grainPhase[ch][g] += 1.0f;
    }

    float pitchedSample = (windowSum > 1.0e-5f) ? (grainAccum / windowSum) : 0.0f;

    // Anatomical 3-Resonance Vocal Tract Formant Filter Bank
    // Scales F1, F2, F3 proportionally based on formantShift (0.0 = deep demon throat, 1.0 = hyperpop chipmunk)
    float fScale = std::pow(2.0f, (formantShift - 0.5f) * 1.6f); // Range approx 0.57x to 1.74x
    
    float f1Freq = std::clamp(500.0f * fScale, 150.0f, (float)(sampleRate * 0.40));
    float f2Freq = std::clamp(1500.0f * fScale, 400.0f, (float)(sampleRate * 0.45));
    float f3Freq = std::clamp(2800.0f * fScale, 800.0f, (float)(sampleRate * 0.48));

    formantF1[ch].setCutoffFrequency(f1Freq);
    formantF2[ch].setCutoffFrequency(f2Freq);
    formantF3[ch].setCutoffFrequency(f3Freq);

    float f1Out = formantF1[ch].processSample(ch, pitchedSample);
    float f2Out = formantF2[ch].processSample(ch, pitchedSample);
    float f3Out = formantF3[ch].processSample(ch, pitchedSample);

    // Dynamic Formant Shaping (blends pitched core with resonant vocal tract acoustic body)
    float formantLayer = pitchedSample * 0.55f + f1Out * 0.45f + f2Out * 0.35f + f3Out * 0.20f;

    // Vocal Bender / Murda Melodies Sub-Harmonic Chest Warmth
    float subTone = subWarmthFilter[ch].processSample(ch, pitchedSample);
    float subHarmonic = std::sin(subTone * juce::MathConstants<float>::halfPi) * 0.35f;

    float shadowLayer = formantLayer + subHarmonic;

    // Harmonic Sub-Saturation & Warmth Drive
    if (drive > 0.005f)
    {
        float satDrive = 1.0f + drive * 5.5f;
        float x = shadowLayer * satDrive;
        // Asymmetric warm triode saturation (even + odd harmonics)
        shadowLayer = (x + 0.15f * x * x) / (1.0f + 0.15f * x * x + std::abs(x));
        shadowLayer /= std::sqrt(satDrive);
    }

    // DC Blocker Protection
    shadowLayer = dcBlocker[ch].process(shadowLayer);

    // Darkness / Muffle Lowpass Filter (sweeps 400 Hz to 6500 Hz)
    float darknessCutoff = 400.0f + (1.0f - darkness) * 5800.0f;
    darknessCutoff = std::clamp(darknessCutoff, 120.0f, (float)(sampleRate * 0.45));
    darknessFilter[ch].setCutoffFrequency(darknessCutoff);
    shadowLayer = darknessFilter[ch].processSample(ch, shadowLayer);

    return AudioUtils::sanitize(shadowLayer);
}

void ShadowProcessor::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled || mix <= 0.001f)
        return;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    // Constant-power wet/dry crossfade
    float wetGain = std::sin(mix * juce::MathConstants<float>::halfPi);
    float dryGain = std::cos(mix * juce::MathConstants<float>::halfPi);

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            float drySample = channelData[i];
            float shadowSample = processSample(drySample, ch);

            // Blend clean deep shadow vocal with pristine dry signal
            channelData[i] = AudioUtils::sanitize(drySample * dryGain + shadowSample * wetGain * 1.15f);
        }

        writeIndex = (writeIndex + 1) & BUFFER_MASK;
    }
}

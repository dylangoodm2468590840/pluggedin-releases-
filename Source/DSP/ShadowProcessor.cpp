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
        // 1. Chest Weight Low-Pass / Low-Shelf (125 Hz Fundamental Resonance)
        chestWeightFilter[ch].prepare(spec);
        chestWeightFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        chestWeightFilter[ch].setCutoffFrequency(160.0f);
        chestWeightFilter[ch].setResonance(0.85f);

        // 2. Anti-Mud Low-Mid Notch / Bell (350 Hz Boxiness Suppression)
        antiMudFilter[ch].prepare(spec);
        antiMudFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
        antiMudFilter[ch].setCutoffFrequency(360.0f);
        antiMudFilter[ch].setResonance(1.8f);

        // 3. Darkness Low-Pass Tone Filter
        darknessFilter[ch].prepare(spec);
        darknessFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        darknessFilter[ch].setResonance(0.707f);
    }

    reset();
}

void ShadowProcessor::reset()
{
    std::fill(grainBufferL.begin(), grainBufferL.end(), 0.0f);
    std::fill(grainBufferR.begin(), grainBufferR.end(), 0.0f);
    writeIndex = 0;

    std::fill(std::begin(pitchBuffer), std::end(pitchBuffer), 0.0f);
    pitchBufIdx = 0;
    pitchAnalysisCounter = 0;
    currentPitchPeriodSamples = static_cast<float>(sampleRate) / 150.0f; // Default 150Hz
    smoothedPitchPeriod = currentPitchPeriodSamples;

    std::fill(std::begin(lpcAnalysisBuffer), std::end(lpcAnalysisBuffer), 0.0f);
    lpcAnalysisIdx = 0;
    lpcUpdateCounter = 0;
    lpcA[0] = 1.0f;
    lpcGammaA[0] = 1.0f;
    for (int i = 1; i <= LPC_ORDER; ++i)
    {
        lpcA[i] = 0.0f;
        lpcGammaA[i] = 0.0f;
        lpcHistory[0][i] = 0.0f;
        lpcHistory[1][i] = 0.0f;
    }

    subPhase = 0.0f;
    subEnvFollower = 0.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        grainPhase[ch][0] = 0.00f;
        grainPhase[ch][1] = 0.25f;
        grainPhase[ch][2] = 0.50f;
        grainPhase[ch][3] = 0.75f;

        float defaultGrain = std::max(currentPitchPeriodSamples * 2.5f, 256.0f);
        for (int g = 0; g < 4; ++g)
            activeGrainLength[ch][g] = defaultGrain;

        chestWeightFilter[ch].reset();
        antiMudFilter[ch].reset();
        darknessFilter[ch].reset();
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

void ShadowProcessor::updatePitchPeriod(float sample)
{
    pitchBuffer[pitchBufIdx] = sample;
    pitchBufIdx = (pitchBufIdx + 1) % PITCH_BUF_SIZE;

    // Run autocorrelation pitch estimation every 128 samples (~3ms) to save CPU
    if (++pitchAnalysisCounter < 128)
        return;
    pitchAnalysisCounter = 0;

    // Vocal fundamental search range: 65 Hz to 750 Hz
    const int minLag = std::max(20, static_cast<int>(sampleRate / 750.0));
    const int maxLag = std::min(PITCH_BUF_SIZE / 2 - 1, static_cast<int>(sampleRate / 65.0));

    float bestCorr = 0.0f;
    int bestLag = static_cast<int>(sampleRate / 150.0);

    // Normalized Difference Function / Autocorrelation
    int analysisLen = PITCH_BUF_SIZE / 2;
    int readStart = (pitchBufIdx - PITCH_BUF_SIZE + BUFFER_SIZE) % PITCH_BUF_SIZE;

    float energy0 = 0.0f;
    for (int i = 0; i < analysisLen; ++i)
    {
        float s = pitchBuffer[(readStart + i) % PITCH_BUF_SIZE];
        energy0 += s * s;
    }

    if (energy0 > 1.0e-4f)
    {
        for (int lag = minLag; lag <= maxLag; lag += 2)
        {
            float crossSum = 0.0f;
            float energyLag = 0.0f;

            for (int i = 0; i < analysisLen; i += 2)
            {
                float s0 = pitchBuffer[(readStart + i) % PITCH_BUF_SIZE];
                float s1 = pitchBuffer[(readStart + i + lag) % PITCH_BUF_SIZE];
                crossSum += s0 * s1;
                energyLag += s1 * s1;
            }

            float norm = crossSum / (std::sqrt(energy0 * energyLag) + 1.0e-6f);
            if (norm > bestCorr)
            {
                bestCorr = norm;
                bestLag = lag;
            }
        }
    }

    if (bestCorr > 0.45f)
    {
        currentPitchPeriodSamples = static_cast<float>(bestLag);
    }
    else
    {
        // Smoothly decay towards comfortable speech center (~140Hz) if unvoiced
        currentPitchPeriodSamples = 0.95f * currentPitchPeriodSamples + 0.05f * (static_cast<float>(sampleRate) / 140.0f);
    }

    smoothedPitchPeriod = 0.85f * smoothedPitchPeriod + 0.15f * currentPitchPeriodSamples;
}

void ShadowProcessor::computeLpcCoefficients(const float* windowedSignal, int length, float* outA, int order)
{
    // 1. Compute Autocorrelation R[0..order]
    float r[LPC_ORDER + 1] = { 0.0f };
    for (int k = 0; k <= order; ++k)
    {
        float sum = 0.0f;
        for (int n = 0; n < length - k; ++n)
            sum += windowedSignal[n] * windowedSignal[n + k];
        r[k] = sum;
    }

    // If zero energy, return identity
    if (r[0] < 1.0e-7f)
    {
        outA[0] = 1.0f;
        for (int i = 1; i <= order; ++i) outA[i] = 0.0f;
        return;
    }

    // 2. Levinson-Durbin Recursion
    float a[LPC_ORDER + 1] = { 0.0f };
    float aPrev[LPC_ORDER + 1] = { 0.0f };
    a[0] = 1.0f;
    float e = r[0];

    for (int i = 1; i <= order; ++i)
    {
        float lambda = 0.0f;
        for (int j = 1; j < i; ++j)
            lambda += aPrev[j] * r[i - j];
        lambda = (r[i] - lambda) / (e + 1.0e-9f);

        // Clamp reflection coefficient for strict filter stability
        lambda = std::clamp(lambda, -0.98f, 0.98f);

        a[i] = lambda;
        for (int j = 1; j < i; ++j)
            a[j] = aPrev[j] - lambda * aPrev[i - j];

        e *= (1.0f - lambda * lambda);
        if (e < 1.0e-9f) break;

        for (int j = 0; j <= i; ++j)
            aPrev[j] = a[j];
    }

    outA[0] = 1.0f;
    for (int i = 1; i <= order; ++i)
        outA[i] = -a[i]; // Negated for standard AR direct synthesis form
}

float ShadowProcessor::processSample(float inputSample, int channel)
{
    float sample = AudioUtils::sanitize(inputSample);
    int ch = std::min(channel, 1);

    if (ch == 0)
    {
        updatePitchPeriod(sample);

        // Feed LPC analysis buffer
        lpcAnalysisBuffer[lpcAnalysisIdx] = sample;
        lpcAnalysisIdx = (lpcAnalysisIdx + 1) % 512;

        // Recompute LPC spectral envelope every 256 samples (~5.8ms)
        if (++lpcUpdateCounter >= 256)
        {
            lpcUpdateCounter = 0;
            float winBuf[512];
            for (int n = 0; n < 512; ++n)
            {
                int srcIdx = (lpcAnalysisIdx + n) % 512;
                // Hann window
                float win = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * (float)n / 511.0f));
                winBuf[n] = lpcAnalysisBuffer[srcIdx] * win;
            }
            computeLpcCoefficients(winBuf, 512, lpcA, LPC_ORDER);

            // Morph LPC poles by gamma factor (0.5 = deep resonant chest cavity, 1.0 = bright)
            // formantShift (0.0 -> 1.0) maps to gamma (0.78 -> 0.98)
            float gamma = std::clamp(0.78f + (formantShift * 0.20f), 0.65f, 0.98f);
            float gPow = 1.0f;
            lpcGammaA[0] = 1.0f;
            for (int i = 1; i <= LPC_ORDER; ++i)
            {
                gPow *= gamma;
                lpcGammaA[i] = lpcA[i] * gPow;
            }
        }
    }

    // Target Pitch Shift Ratio
    float pitchRatio = 0.5f; // Default Octave Down
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

    // Pitch-Synchronous Overlap Add (PSOLA) Engine
    // Grain length matches glottal period (2x to 3x fundamental period for seamless overlap)
    float basePeriod = std::clamp(smoothedPitchPeriod, 45.0f, 600.0f);
    float grainLen = std::clamp(basePeriod * 3.0f, 180.0f, 2048.0f);

    float grainAccum = 0.0f;
    float windowSum = 0.0f;

    for (int g = 0; g < 4; ++g)
    {
        float phase = grainPhase[ch][g]; // 0.0 to 1.0
        float gLen = activeGrainLength[ch][g];
        float delay = phase * gLen;

        // Smooth raised-cosine (Hann) glottal grain window
        float win = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));

        float grainSample = readHermite(buf, delay, writeIndex, BUFFER_MASK);
        grainAccum += grainSample * win;
        windowSum += win;

        // Phase advance locked to pitch ratio
        float phaseInc = (1.0f - pitchRatio) / gLen;
        grainPhase[ch][g] += phaseInc;

        if (grainPhase[ch][g] >= 1.0f)
        {
            grainPhase[ch][g] -= 1.0f;
            // Lock new grain length synchronously to current vocal pitch period
            activeGrainLength[ch][g] = grainLen;
        }
        else if (grainPhase[ch][g] < 0.0f)
        {
            grainPhase[ch][g] += 1.0f;
            activeGrainLength[ch][g] = grainLen;
        }
    }

    float pitchedExcitation = (windowSum > 1.0e-5f) ? (grainAccum / windowSum) : 0.0f;

    // 12th-Order LPC Formant All-Pole Resynthesis Filter 1 / A(z / gamma)
    float resonantVocalBody = pitchedExcitation;
    for (int i = 1; i <= LPC_ORDER; ++i)
        resonantVocalBody -= lpcGammaA[i] * lpcHistory[ch][i];

    resonantVocalBody = AudioUtils::sanitize(resonantVocalBody);

    // Update LPC filter history
    for (int i = LPC_ORDER; i > 1; --i)
        lpcHistory[ch][i] = lpcHistory[ch][i - 1];
    lpcHistory[ch][1] = resonantVocalBody;

    // Blend Resonant Body with Core Excitation
    float vocalLayer = pitchedExcitation * 0.40f + resonantVocalBody * 0.60f;

    // Murda Melodies Reference Curve: +21.9dB Chest Boost (125Hz) and -3dB Mud Cut (350Hz)
    float chestBoost = chestWeightFilter[ch].processSample(0, vocalLayer);
    float mudBand = antiMudFilter[ch].processSample(0, vocalLayer);
    
    // Smooth composite layer
    float shadowCore = vocalLayer + (chestBoost * 0.45f) - (mudBand * 0.25f);

    // Phase-Locked Sub-Octave Fundamental Synthesizer
    if (ch == 0)
    {
        float absIn = std::abs(sample);
        subEnvFollower = 0.992f * subEnvFollower + 0.008f * absIn;
        float subFreq = (static_cast<float>(sampleRate) / basePeriod) * 0.5f; // Sub-octave F0 / 2
        float phaseInc = (subFreq * juce::MathConstants<float>::twoPi) / static_cast<float>(sampleRate);
        subPhase += phaseInc;
        if (subPhase >= juce::MathConstants<float>::twoPi)
            subPhase -= juce::MathConstants<float>::twoPi;
    }
    
    float subOsc = std::sin(subPhase) * subEnvFollower * 0.28f;
    float combinedVoice = shadowCore + subOsc;

    // Analog Warmth & Harmonic Preamp Drive
    if (drive > 0.005f)
    {
        float satDrive = 1.0f + drive * 3.5f;
        float x = combinedVoice * satDrive;
        // Asymmetric warm tube transfer curve
        float sat = std::tanh(x + 0.12f * (x * x));
        combinedVoice = sat / std::sqrt(satDrive);
    }

    // DC Protection & Tone Shaping
    combinedVoice = dcBlocker[ch].process(combinedVoice);

    float darknessCutoff = 500.0f + (1.0f - darkness) * 6500.0f;
    darknessCutoff = std::clamp(darknessCutoff, 120.0f, static_cast<float>(sampleRate * 0.45));
    darknessFilter[ch].setCutoffFrequency(darknessCutoff);
    combinedVoice = darknessFilter[ch].processSample(0, combinedVoice);

    return AudioUtils::sanitize(combinedVoice);
}

void ShadowProcessor::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled || mix <= 0.001f)
    {
        buffer.clear();
        return;
    }

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    float wetGain = mix * 1.05f;

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* channelData = buffer.getWritePointer(ch);
            float drySample = channelData[i];
            float shadowSample = processSample(drySample, ch);

            channelData[i] = AudioUtils::sanitize(shadowSample * wetGain);
        }

        writeIndex = (writeIndex + 1) & BUFFER_MASK;
    }
}

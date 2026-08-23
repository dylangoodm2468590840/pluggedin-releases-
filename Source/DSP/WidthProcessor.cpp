#include "WidthProcessor.h"
#include <cmath>
#include <algorithm>

WidthProcessor::WidthProcessor()
{
    delayBufferL.resize(BUFFER_SIZE, 0.0f);
    delayBufferR.resize(BUFFER_SIZE, 0.0f);
}

WidthProcessor::~WidthProcessor() = default;

void WidthProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    delayBufferL.assign(BUFFER_SIZE, 0.0f);
    delayBufferR.assign(BUFFER_SIZE, 0.0f);
    writeIndex = 0;

    lfoPhase1 = 0.0f;
    lfoPhase2 = 0.25f;
    lfoPhase3 = 0.50f;
    lfoPhase4 = 0.75f;

    feedbackL = 0.0f;
    feedbackR = 0.0f;

    // 1. Prepare BBD Warmth Filters (Low-pass at 8.5 kHz to eliminate harsh digital sizzle)
    bbdWarmthFilterL.prepare(spec);
    bbdWarmthFilterL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    bbdWarmthFilterL.setCutoffFrequency(8500.0f);
    bbdWarmthFilterL.setResonance(0.707f);

    bbdWarmthFilterR.prepare(spec);
    bbdWarmthFilterR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    bbdWarmthFilterR.setCutoffFrequency(8500.0f);
    bbdWarmthFilterR.setResonance(0.707f);

    // 2. Prepare Bass Mono-Maker (High-pass on side at 130 Hz)
    bassMonoFilter.prepare(spec);
    bassMonoFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    bassMonoFilter.setCutoffFrequency(130.0f);
    bassMonoFilter.setResonance(0.707f);
}

void WidthProcessor::reset()
{
    std::fill(delayBufferL.begin(), delayBufferL.end(), 0.0f);
    std::fill(delayBufferR.begin(), delayBufferR.end(), 0.0f);
    writeIndex = 0;
    feedbackL = 0.0f;
    feedbackR = 0.0f;

    bbdWarmthFilterL.reset();
    bbdWarmthFilterR.reset();
    bassMonoFilter.reset();
}

void WidthProcessor::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    auto& block = context.getOutputBlock();
    const size_t numChannels = block.getNumChannels();
    const size_t numSamples = block.getNumSamples();

    if (numChannels == 0 || numSamples == 0 || amount <= 0.001f)
        return;

    // MicroShift / Chorus LFO Rate (0.15 Hz to 4.5 Hz)
    float lfoRateHz = 0.15f + 4.35f * rate * rate;
    float lfoPhaseInc = lfoRateHz / static_cast<float>(sampleRate);

    // MicroShift modulation depth (sub-millisecond pitch detuning)
    float modDepthMs = 0.25f + 2.5f * depth;
    float modDepthSamples = (modDepthMs * 0.001f) * static_cast<float>(sampleRate);

    // Staggered prime base delay times (11.3ms, 17.1ms, 23.7ms, 29.4ms)
    float baseDelayL1 = 0.0113f * static_cast<float>(sampleRate);
    float baseDelayL2 = 0.0237f * static_cast<float>(sampleRate);
    float baseDelayR1 = 0.0171f * static_cast<float>(sampleRate);
    float baseDelayR2 = 0.0294f * static_cast<float>(sampleRate);

    float fbGain = (mode == ModulationMode::AnalogFlangus) ? (0.28f * amount) : 0.0f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        float inL = block.getSample(0, i);
        float inR = (numChannels > 1) ? block.getSample(1, i) : inL;

        // 1. Mid/Side Split: The direct dry Mid channel is preserved 100% uncorrupted
        float dryMid  = 0.5f * (inL + inR);
        float drySide = 0.5f * (inL - inR);

        // 2. Write to circular delay buffer with soft-clipped feedback
        float writeSampleL = AudioUtils::sanitize(inL + std::tanh(feedbackL * fbGain));
        float writeSampleR = AudioUtils::sanitize(inR + std::tanh(feedbackR * fbGain));

        delayBufferL[writeIndex] = writeSampleL;
        delayBufferR[writeIndex] = writeSampleR;

        // Quadrature Sine LFO oscillations
        float lfo1 = std::sin(juce::MathConstants<float>::twoPi * lfoPhase1);
        float lfo2 = std::sin(juce::MathConstants<float>::twoPi * lfoPhase2);
        float lfo3 = std::sin(juce::MathConstants<float>::twoPi * lfoPhase3);
        float lfo4 = std::sin(juce::MathConstants<float>::twoPi * lfoPhase4);

        // Calculate 4 modulated delay taps
        float dL1 = baseDelayL1 + lfo1 * modDepthSamples;
        float dL2 = baseDelayL2 + lfo2 * modDepthSamples;
        float dR1 = baseDelayR1 + lfo3 * modDepthSamples;
        float dR2 = baseDelayR2 + lfo4 * modDepthSamples;

        // Read 4 multi-tap voices using 3rd-order Cubic Hermite interpolation
        float tapL1 = readCubicHermite(delayBufferL, dL1, writeIndex, BUFFER_MASK);
        float tapL2 = readCubicHermite(delayBufferL, dL2, writeIndex, BUFFER_MASK);
        float tapR1 = readCubicHermite(delayBufferR, dR1, writeIndex, BUFFER_MASK);
        float tapR2 = readCubicHermite(delayBufferR, dR2, writeIndex, BUFFER_MASK);

        // Decorrelated stereo diffuse field
        float diffuseL = 0.60f * tapL1 - 0.40f * tapR2;
        float diffuseR = 0.60f * tapR1 - 0.40f * tapL2;

        feedbackL = diffuseL;
        feedbackR = diffuseR;

        // Warm BBD filtering
        diffuseL = bbdWarmthFilterL.processSample(0, diffuseL);
        diffuseR = bbdWarmthFilterR.processSample(0, diffuseR);

        // 3. Extract purely decorrelated Side energy
        float decorrelatedSide = 0.5f * (diffuseL - diffuseR);

        // 4. Bass Mono-Maker: High-pass Side at 130 Hz so sub-bass stays 100% mono and punchy
        decorrelatedSide = bassMonoFilter.processSample(0, decorrelatedSide);

        // 5. Synthesize Output: Mid is completely clean, Side is widened
        float sideWidth = drySide + decorrelatedSide * (amount * 1.25f);

        float outL = dryMid + sideWidth;
        float outR = dryMid - sideWidth;

        block.setSample(0, i, AudioUtils::sanitize(outL));
        if (numChannels > 1)
        {
            block.setSample(1, i, AudioUtils::sanitize(outR));
        }

        // Advance buffer & LFO phases
        writeIndex = (writeIndex + 1) & BUFFER_MASK;

        lfoPhase1 += lfoPhaseInc;
        if (lfoPhase1 >= 1.0f) lfoPhase1 -= 1.0f;

        lfoPhase2 += lfoPhaseInc;
        if (lfoPhase2 >= 1.0f) lfoPhase2 -= 1.0f;

        lfoPhase3 += lfoPhaseInc;
        if (lfoPhase3 >= 1.0f) lfoPhase3 -= 1.0f;

        lfoPhase4 += lfoPhaseInc;
        if (lfoPhase4 >= 1.0f) lfoPhase4 -= 1.0f;
    }
}

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

    // MicroShift / Chorus LFO Rate (0.15 Hz to 6.0 Hz)
    float lfoRateHz = 0.15f + 5.85f * rate * rate;
    float lfoPhaseInc = lfoRateHz / static_cast<float>(sampleRate);

    // MicroShift modulation depth (sub-millisecond pitch detuning)
    float modDepthMs = 0.3f + 3.2f * depth * amount;
    float modDepthSamples = (modDepthMs * 0.001f) * static_cast<float>(sampleRate);

    // Soundtoys MicroShift / Haas Staggered Base Delays
    float baseDelayL1 = (0.0092f) * static_cast<float>(sampleRate); // 9.2ms
    float baseDelayL2 = (0.0174f) * static_cast<float>(sampleRate); // 17.4ms
    float baseDelayR1 = (0.0128f) * static_cast<float>(sampleRate); // 12.8ms
    float baseDelayR2 = (0.0226f) * static_cast<float>(sampleRate); // 22.6ms

    // Feedback amount for flanger mode
    float fbGain = (mode == ModulationMode::AnalogFlangus) ? (0.35f * amount) : 0.0f;

    for (size_t i = 0; i < numSamples; ++i)
    {
        float inL = block.getSample(0, i);
        float inR = (numChannels > 1) ? block.getSample(1, i) : inL;

        // Write to circular delay buffer with soft-clipped feedback
        float writeSampleL = AudioUtils::sanitize(inL + std::tanh(feedbackL * fbGain));
        float writeSampleR = AudioUtils::sanitize(inR + std::tanh(feedbackR * fbGain));

        delayBufferL[writeIndex] = writeSampleL;
        delayBufferR[writeIndex] = writeSampleR;

        // Quadrature Sine LFO oscillations
        float lfo1 = std::sin(juce::MathConstants<float>::twoPi * lfoPhase1);
        float lfo2 = std::sin(juce::MathConstants<float>::twoPi * lfoPhase2);
        float lfo3 = std::sin(juce::MathConstants<float>::twoPi * lfoPhase3);
        float lfo4 = std::sin(juce::MathConstants<float>::twoPi * lfoPhase4);

        // Calculate 4 modulated delay times (micro-pitch detuning)
        float dL1 = baseDelayL1 + lfo1 * modDepthSamples;
        float dL2 = baseDelayL2 + lfo2 * modDepthSamples;
        float dR1 = baseDelayR1 + lfo3 * modDepthSamples;
        float dR2 = baseDelayR2 + lfo4 * modDepthSamples;

        // Read 4 multi-tap voices using 3rd-order Cubic Hermite interpolation
        float tapL1 = readCubicHermite(delayBufferL, dL1, writeIndex, BUFFER_MASK);
        float tapL2 = readCubicHermite(delayBufferL, dL2, writeIndex, BUFFER_MASK);
        float tapR1 = readCubicHermite(delayBufferR, dR1, writeIndex, BUFFER_MASK);
        float tapR2 = readCubicHermite(delayBufferR, dR2, writeIndex, BUFFER_MASK);

        // Mix into ultra-wide stereo image (Left: tapL1 + tapR2, Right: tapR1 + tapL2)
        float wetL = 0.65f * tapL1 + 0.35f * tapR2;
        float wetR = 0.65f * tapR1 + 0.35f * tapL2;

        feedbackL = wetL;
        feedbackR = wetR;

        // Apply Analog BBD Warmth Filtering
        wetL = bbdWarmthFilterL.processSample(0, wetL);
        wetR = bbdWarmthFilterR.processSample(0, wetR);

        // Mid/Side Matrix & Bass Mono-Maker (100% Mono-Safe Sub Bass)
        float mid = 0.5f * (wetL + wetR);
        float side = 0.5f * (wetL - wetR);

        // Remove stereo side below 130Hz so 808s and low vocals never lose center punch
        side = bassMonoFilter.processSample(0, side);

        // Expand side energy based on amount
        float widthGain = 1.0f + 0.75f * amount;
        side *= widthGain;

        float finalWetL = AudioUtils::sanitize(mid + side);
        float finalWetR = AudioUtils::sanitize(mid - side);

        // Smooth Dry/Wet Mix
        float dryGain = 1.0f - (0.45f * amount);
        float wetGain = 0.85f * amount;

        block.setSample(0, i, inL * dryGain + finalWetL * wetGain);
        if (numChannels > 1)
        {
            block.setSample(1, i, inR * dryGain + finalWetR * wetGain);
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

#include "SpaceProcessor.h"
#include <cmath>
#include <algorithm>

void SpaceProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
    float srScale = static_cast<float>(sampleRate / 44100.0);

    // Initialize Dattorro Diffusion Allpasses
    diff1.init(static_cast<int>(142 * srScale), 0.75f);
    diff2.init(static_cast<int>(107 * srScale), 0.75f);
    diff3.init(static_cast<int>(379 * srScale), 0.625f);
    diff4.init(static_cast<int>(277 * srScale), 0.625f);

    tankDiff1.init(static_cast<int>(908 * srScale), 0.70f);
    tankDiff2.init(static_cast<int>(672 * srScale), 0.70f);

    tankDelayL1.assign(std::max(16, static_cast<int>(4453 * srScale)), 0.0f);
    tankDelayL2.assign(std::max(16, static_cast<int>(3720 * srScale)), 0.0f);
    tankDelayR1.assign(std::max(16, static_cast<int>(4217 * srScale)), 0.0f);
    tankDelayR2.assign(std::max(16, static_cast<int>(3163 * srScale)), 0.0f);

    // Prepare Ping-Pong Delay Lines
    delayLineL.prepare(spec);
    delayLineR.prepare(spec);
    delayLineL.setMaximumDelayInSamples(192000);
    delayLineR.setMaximumDelayInSamples(192000);

    // Analog 3.5kHz lowpass tape damping filter
    delayDampFilterL.prepare(spec);
    delayDampFilterL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    delayDampFilterL.setCutoffFrequency(3500.0f);
    delayDampFilterL.setResonance(0.707f);

    delayDampFilterR.prepare(spec);
    delayDampFilterR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    delayDampFilterR.setCutoffFrequency(3500.0f);
    delayDampFilterR.setResonance(0.707f);

    // Reverb Low-Cut filter (150Hz) to prevent vocal muddiness in reverb tank
    reverbLowCutL.prepare(spec);
    reverbLowCutL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    reverbLowCutL.setCutoffFrequency(150.0f);
    reverbLowCutL.setResonance(0.707f);

    reverbLowCutR.prepare(spec);
    reverbLowCutR.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    reverbLowCutR.setCutoffFrequency(150.0f);
    reverbLowCutR.setResonance(0.707f);

    reverbMixSmoother.reset(sampleRate, 0.02);
    delayMixSmoother.reset(sampleRate, 0.02);
    feedbackSmoother.reset(sampleRate, 0.02);

    int preDelaySize = static_cast<int>(0.028f * sampleRate);
    preDelayBuffer.assign(std::max(64, preDelaySize), 0.0f);

    reset();
}

void SpaceProcessor::reset()
{
    diff1.reset();
    diff2.reset();
    diff3.reset();
    diff4.reset();
    tankDiff1.reset();
    tankDiff2.reset();

    std::fill(preDelayBuffer.begin(), preDelayBuffer.end(), 0.0f);
    preDelayWritePos = 0;

    std::fill(tankDelayL1.begin(), tankDelayL1.end(), 0.0f);
    std::fill(tankDelayL2.begin(), tankDelayL2.end(), 0.0f);
    std::fill(tankDelayR1.begin(), tankDelayR1.end(), 0.0f);
    std::fill(tankDelayR2.begin(), tankDelayR2.end(), 0.0f);

    tankIdxL1 = tankIdxL2 = 0;
    tankIdxR1 = tankIdxR2 = 0;
    tankLFOPhase = 0.0f;
    dampingFilterL = dampingFilterR = 0.0f;

    delayLineL.reset();
    delayLineR.reset();
    delayDampFilterL.reset();
    delayDampFilterR.reset();
    reverbLowCutL.reset();
    reverbLowCutR.reset();

    envFollower = 0.0f;
    wowPhase = 0.0f;

    reverbMixSmoother.setCurrentAndTargetValue(reverbMix);
    delayMixSmoother.setCurrentAndTargetValue(delayMix);
    feedbackSmoother.setCurrentAndTargetValue(feedback);
}

void SpaceProcessor::process(const juce::dsp::ProcessContextReplacing<float>& context)
{
    auto& inputBlock = context.getInputBlock();
    auto& outputBlock = context.getOutputBlock();

    const size_t numChannels = inputBlock.getNumChannels();
    const size_t numSamples = inputBlock.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    reverbMixSmoother.setTargetValue(reverbMix);
    delayMixSmoother.setTargetValue(delayMix);
    feedbackSmoother.setTargetValue(feedback);

    // Delay time targets: Left = T, Right = 1.5 * T (Polyrhythmic Stereo Width)
    float targetDelaySamplesL = delayTime * static_cast<float>(sampleRate);
    float targetDelaySamplesR = (delayTime * 1.5f) * static_cast<float>(sampleRate);

    const float attackCoef = std::exp(-1.0f / (0.008f * static_cast<float>(sampleRate))); // 8ms fast attack
    const float releaseCoef = std::exp(-1.0f / (0.28f * static_cast<float>(sampleRate)));  // 280ms smooth bloom
    const float wowPhaseInc = 0.40f / static_cast<float>(sampleRate);                      // 0.40 Hz tape drift
    const float tankLFOInc  = 0.65f / static_cast<float>(sampleRate);                      // 0.65 Hz chorus modulation

    // Scale reverb decay sustain (0.35 to 0.86)
    const float decayDecay = std::clamp(0.35f + reverbDecay * 0.48f, 0.20f, 0.88f);

    for (size_t i = 0; i < numSamples; ++i)
    {
        float curRevMix = reverbMixSmoother.getNextValue();
        float curDelMix = delayMixSmoother.getNextValue();
        float curFb     = feedbackSmoother.getNextValue();

        float inL = inputBlock.getSample(0, i);
        float inR = (numChannels > 1) ? inputBlock.getSample(1, i) : inL;

        // 1. Dynamic Sidechain Envelope Follower for Auto-Ducking
        float inAbs = 0.5f * (std::abs(inL) + std::abs(inR));
        if (inAbs > envFollower)
            envFollower = attackCoef * envFollower + (1.0f - attackCoef) * inAbs;
        else
            envFollower = releaseCoef * envFollower + (1.0f - releaseCoef) * inAbs;

        float duckGain = 1.0f - std::min(0.85f, envFollower * ducking * 3.0f);

        // 2. High-Density Dattorro Reverb Processing with 28ms Pre-Delay
        float monoIn = 0.5f * (inL + inR);
        float delayedMonoIn = monoIn;
        if (!preDelayBuffer.empty())
        {
            delayedMonoIn = preDelayBuffer[preDelayWritePos];
            preDelayBuffer[preDelayWritePos] = monoIn;
            preDelayWritePos = (preDelayWritePos + 1) % static_cast<int>(preDelayBuffer.size());
        }

        // Pre-diffusion stage on pre-delayed vocal
        float d = diff1.process(delayedMonoIn);
        d = diff2.process(d);
        d = diff3.process(d);
        d = diff4.process(d);

        // Reverb low cut (remove mud below 150Hz)
        d = reverbLowCutL.processSample(0, d);

        // Modulated Delay excursion for chorused Lexicon tail
        float modSamples = std::sin(juce::MathConstants<float>::twoPi * tankLFOPhase) * 12.0f;

        // Left Tank Branch
        float tankInL = d + tankDelayR2[tankIdxR2] * decayDecay;
        float diffOutL = tankDiff1.process(tankInL);
        tankDelayL1[tankIdxL1] = diffOutL;
        tankIdxL1 = (tankIdxL1 + 1) % static_cast<int>(tankDelayL1.size());

        // One-pole high damping absorption (silky top-end roll-off)
        dampingFilterL = dampingFilterL * 0.45f + tankDelayL1[tankIdxL1] * 0.55f;
        tankDelayL2[tankIdxL2] = dampingFilterL;
        tankIdxL2 = (tankIdxL2 + 1) % static_cast<int>(tankDelayL2.size());

        // Right Tank Branch
        float tankInR = d + tankDelayL2[tankIdxL2] * decayDecay;
        float diffOutR = tankDiff2.process(tankInR);
        tankDelayR1[tankIdxR1] = diffOutR;
        tankIdxR1 = (tankIdxR1 + 1) % static_cast<int>(tankDelayR1.size());

        dampingFilterR = dampingFilterR * 0.45f + tankDelayR1[tankIdxR1] * 0.55f;
        tankDelayR2[tankIdxR2] = dampingFilterR;
        tankIdxR2 = (tankIdxR2 + 1) % static_cast<int>(tankDelayR2.size());

        // Stereo Reverb Output Taps
        float reverbOutL = (tankDelayL1[tankIdxL1] + tankDelayL2[tankIdxL2] - tankDelayR1[tankIdxR1]) * 0.45f;
        float reverbOutR = (tankDelayR1[tankIdxR1] + tankDelayR2[tankIdxR2] - tankDelayL1[tankIdxL1]) * 0.45f;

        // Pro-Calibrated Reverb Mix Curve (Parabolic scaling: 0.05 is a subtle 2% halo, 0.30 is 12% plate)
        float revScale = curRevMix * curRevMix * 0.65f;
        float wetReverbL = reverbOutL * (revScale * duckGain);
        float wetReverbR = reverbOutR * (revScale * duckGain);

        // 3. Analog Ping-Pong Tape Delay
        float wow = std::sin(juce::MathConstants<float>::twoPi * wowPhase) * 14.0f;
        delayLineL.setDelay(targetDelaySamplesL + wow);
        delayLineR.setDelay(targetDelaySamplesR - wow);

        float delayOutL = delayLineL.popSample(0);
        float delayOutR = delayLineR.popSample(0);

        // 3.5kHz analog tape damping filter on repeats
        delayOutL = delayDampFilterL.processSample(0, delayOutL);
        delayOutR = delayDampFilterR.processSample(0, delayOutR);

        // Ping-pong cross feedback with warm soft clipping
        float fbL = std::tanh(delayOutR * curFb);
        float fbR = std::tanh(delayOutL * curFb);

        delayLineL.pushSample(0, inL + fbL);
        delayLineR.pushSample(0, inR + fbR);

        float wetDelayL = delayOutL * (curDelMix * duckGain * 1.1f);
        float wetDelayR = delayOutR * (curDelMix * duckGain * 1.1f);

        // Sum Dry + Reverb + Delay
        float outL = AudioUtils::sanitize(inL + wetReverbL + wetDelayL);
        float outR = AudioUtils::sanitize((numChannels > 1 ? inR : inL) + wetReverbR + wetDelayR);

        outputBlock.setSample(0, i, outL);
        if (numChannels > 1)
            outputBlock.setSample(1, i, outR);

        // Advance modulation phases
        wowPhase += wowPhaseInc;
        if (wowPhase >= 1.0f) wowPhase -= 1.0f;

        tankLFOPhase += tankLFOInc;
        if (tankLFOPhase >= 1.0f) tankLFOPhase -= 1.0f;
    }
}

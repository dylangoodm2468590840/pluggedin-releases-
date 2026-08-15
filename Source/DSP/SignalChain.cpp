#include "SignalChain.h"
#include <cmath>

SignalChain::SignalChain() = default;
SignalChain::~SignalChain() = default;

void SignalChain::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    dryBuffer.setSize((int)spec.numChannels, (int)spec.maximumBlockSize);

    inputGainSmoother.reset(sampleRate, 0.02);
    outputGainSmoother.reset(sampleRate, 0.02);
    globalMixSmoother.reset(sampleRate, 0.02);
    degenerateSmoother.reset(sampleRate, 0.02);

    outputLimiter.prepare(spec);
    outputLimiter.setThreshold(-0.3f); // -0.3 dBFS Commercial Pro Ceiling Protection
    outputLimiter.setRelease(35.0f);

    eqLowCutFilter1.prepare(spec);
    eqLowCutFilter2.prepare(spec);
    eqLowBellFilter.prepare(spec);
    eqMidBellFilter.prepare(spec);
    eqHighShelfFilter.prepare(spec);
    eqHighCutFilter1.prepare(spec);
    eqHighCutFilter2.prepare(spec);

    for (int i = 0; i < maxDynamicNodes; ++i)
        dynamicBiquads[i].prepare(spec);

    masterEQEngine.prepare(spec.sampleRate, (int)spec.maximumBlockSize);

    reset();
}

void SignalChain::reset()
{
    inputGainSmoother.setCurrentAndTargetValue(1.0f);
    outputGainSmoother.setCurrentAndTargetValue(1.0f);
    globalMixSmoother.setCurrentAndTargetValue(1.0f);
    degenerateSmoother.setCurrentAndTargetValue(0.0f);

    outputLimiter.reset();

    eqLowCutFilter1.reset();
    eqLowCutFilter2.reset();
    eqLowBellFilter.reset();
    eqMidBellFilter.reset();
    eqHighShelfFilter.reset();
    eqHighCutFilter1.reset();
    eqHighCutFilter2.reset();

    std::fill(std::begin(fifoBuffer), std::end(fifoBuffer), 0.0f);
    std::fill(std::begin(fftData), std::end(fftData), 0.0f);
    std::fill(std::begin(spectrumBars), std::end(spectrumBars), 0.0f);
    fifoIndex = 0;
    nextFFTBlockReady = false;

    inputLevelPeak.store(0.0f);
    outputLevelPeak.store(0.0f);
}

void SignalChain::getSpectrumData(float* dest64Bars) const noexcept
{
    if (nextFFTBlockReady.exchange(false))
    {
        windowEngine.multiplyWithWindowingTable(const_cast<float*>(fftData), fftSize);
        const_cast<juce::dsp::FFT&>(fftEngine).performFrequencyOnlyForwardTransform(const_cast<float*>(fftData));

        for (int i = 0; i < 64; ++i)
        {
            float normFreq = std::pow((float)i / 63.0f, 2.2f);
            int binIndex = juce::roundToInt(normFreq * 512.0f);
            binIndex = std::clamp(binIndex, 0, 511);

            float magnitude = fftData[binIndex];
            float level = std::clamp((juce::Decibels::gainToDecibels(magnitude) + 60.0f) / 60.0f, 0.0f, 1.0f);

            spectrumBars[i] = std::max(level, spectrumBars[i] * 0.72f);
        }
    }

    for (int i = 0; i < 64; ++i)
        dest64Bars[i] = spectrumBars[i];
}

void SignalChain::process(juce::AudioBuffer<float>& buffer, 
                          float inputGainDb,
                          float outputGainDb,
                          float globalMix,
                          float degenerateMacro,
                          float subEnable,
                          float gritEnable,
                          float modEnable,
                          float delayEnable,
                          float reverbEnable,
                          float eqLowCutFreq,
                          float eqLowGainDb,
                          float eqMidGainDb,
                          float eqHighGainDb,
                          float eqHighCutFreq,
                          float eqLowQ,
                          float eqMidQ,
                          float eqHighQ,
                          int eqLowCutSlope,
                          int eqHighCutSlope,
                          float macroDepth,
                          float macroDark,
                          float macroMotion,
                          float macroChaos,
                          float macroAge,
                          float macroGhost,
                          float macroTone,
                          ShadowProcessor& shadowModule,
                          CrushProcessor& crushModule,
                          WidthProcessor& widthModule,
                          SpaceProcessor& spaceModule,
                          DeviceProcessor& deviceModule)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;

    // 1. Calculate Input Peak Metering
    float inPeak = 0.0f;
    for (int c = 0; c < numChannels; ++c)
    {
        float p = buffer.getMagnitude(c, 0, numSamples);
        if (p > inPeak) inPeak = p;
    }
    inputLevelPeak.store(inPeak);

    // 2. Apply Input Trim Gain
    inputGainSmoother.setTargetValue(juce::Decibels::decibelsToGain(inputGainDb));
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float gain = inputGainSmoother.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample(ch, sample, buffer.getSample(ch, sample) * gain);
    }

    // 3. Save pre-allocated Dry Buffer copy
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // 4. Update 5-Band Parametric Audio Biquads with Dynamic Q & Slope Orders
    float safeLowQ  = std::clamp(eqLowQ, 0.10f, 50.0f);
    float safeMidQ  = std::clamp(eqMidQ, 0.10f, 50.0f);
    float safeHighQ = std::clamp(eqHighQ, 0.10f, 50.0f);

    auto lowCutCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, std::clamp(eqLowCutFreq, 20.0f, 500.0f));
    auto lowBellCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 200.0f, safeLowQ, juce::Decibels::decibelsToGain(eqLowGainDb));
    auto midBellCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 2200.0f, safeMidQ, juce::Decibels::decibelsToGain(eqMidGainDb));
    auto highShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 8000.0f, safeHighQ, juce::Decibels::decibelsToGain(eqHighGainDb));
    auto highCutCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, std::clamp(eqHighCutFreq, 500.0f, 20000.0f));

    *eqLowCutFilter1.coefficients = *lowCutCoeffs;
    *eqLowCutFilter2.coefficients = *lowCutCoeffs;
    *eqLowBellFilter.coefficients = *lowBellCoeffs;
    *eqMidBellFilter.coefficients = *midBellCoeffs;
    *eqHighShelfFilter.coefficients = *highShelfCoeffs;
    *eqHighCutFilter1.coefficients = *highCutCoeffs;
    *eqHighCutFilter2.coefficients = *highCutCoeffs;

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    eqLowCutFilter1.process(context);
    if (eqLowCutSlope >= 2) eqLowCutFilter2.process(context);

    eqLowBellFilter.process(context);
    eqMidBellFilter.process(context);
    eqHighShelfFilter.process(context);

    eqHighCutFilter1.process(context);
    if (eqHighCutSlope >= 2) eqHighCutFilter2.process(context);

    // 4b. Process Live 64-Bit Transposed Direct Form II Master EQ Engine with Nyquist De-cramping
    {
        juce::ScopedLock sl(nodeLock);
        int numDyn = std::min((int)activeNodes.size(), maxDynamicNodes);
        for (int i = 0; i < maxDynamicNodes; ++i)
        {
            if (i < numDyn && activeNodes[i].active)
            {
                double fHz = std::clamp((double)activeNodes[i].freqHz, 10.0, sampleRate * 0.495);
                double gDb = std::clamp((double)activeNodes[i].gainDb, -24.0, 24.0);
                double qVal = std::clamp((double)activeNodes[i].qFactor, 0.10, 50.0);
                int shape = activeNodes[i].filterType;

                TransposedDirectFormIIBiquad::FilterType engineType = TransposedDirectFormIIBiquad::Peaking;
                if (shape == 1) engineType = TransposedDirectFormIIBiquad::LowCut;
                else if (shape == 2) engineType = TransposedDirectFormIIBiquad::HighCut;
                else if (shape == 3) engineType = TransposedDirectFormIIBiquad::LowShelf;
                else if (shape == 4) engineType = TransposedDirectFormIIBiquad::HighShelf;
                else if (shape == 5) engineType = TransposedDirectFormIIBiquad::Notch;

                masterEQEngine.updateBand(i, engineType, fHz, gDb, qVal, true);
            }
            else
            {
                masterEQEngine.updateBand(i, TransposedDirectFormIIBiquad::Peaking, 1000.0, 0.0, 0.707, false);
            }
        }

        masterEQEngine.processBlock(buffer);
    }

    // 5. DEGENERATE Demonic Vocal Multi-FX Engine Dynamics
    float degenVal = std::clamp(degenerateMacro, 0.0f, 1.0f);
    if (degenVal > 0.01f)
    {
        // Formant lowering & sub octave tracking drive
        shadowModule.setFormantShift(0.40f * degenVal);
        shadowModule.setDrive(0.35f * degenVal);
        shadowModule.setMix(0.50f * degenVal);

        // Tube Fuzz & Harmonic Grit
        crushModule.setAmount(std::clamp(crushModule.getAmount() + 0.40f * degenVal, 0.0f, 1.0f));
        crushModule.setTone(std::clamp(0.65f + 0.25f * degenVal, 0.0f, 1.0f));

        // Demonic Width Expansion
        widthModule.setAmount(std::clamp(widthModule.getAmount() + 0.35f * degenVal, 0.0f, 1.0f));
    }

    // Process Modules & Macro Engine conditionally based on Bypass Switches
    if (subEnable > 0.5f) shadowModule.process(buffer);
    if (gritEnable > 0.5f) crushModule.process(buffer);
    if (modEnable > 0.5f) widthModule.process(context);
    if (delayEnable > 0.5f || reverbEnable > 0.5f) spaceModule.process(context);
    deviceModule.process(context);

    // 6. Global Dry/Wet Mix
    globalMixSmoother.setTargetValue(globalMix);
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float mix = globalMixSmoother.getNextValue();
        float dryAmount = 1.0f - mix;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float wetSample = buffer.getSample(ch, sample);
            float drySample = dryBuffer.getSample(ch, sample);
            buffer.setSample(ch, sample, drySample * dryAmount + wetSample * mix);
        }
    }

    // 7. Output Trim Gain with Auto-Gain Compensation & Pro Limiter
    float autoGainTrim = 1.0f / std::sqrt(1.0f + 1.25f * degenVal * degenVal);

    outputGainSmoother.setTargetValue(juce::Decibels::decibelsToGain(outputGainDb) * autoGainTrim);
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float gain = outputGainSmoother.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float s = AudioUtils::sanitize(buffer.getSample(ch, sample) * gain);
            buffer.setSample(ch, sample, s);
        }
    }

    outputLimiter.process(context);

    // 8. Output Peak Level & Push Audio Samples to 2048-Point FFT FIFO
    float outPeak = 0.0f;
    const float* channelData = buffer.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        float s = std::abs(channelData[i]);
        if (s > outPeak) outPeak = s;

        if (fifoIndex == fftSize)
        {
            if (!nextFFTBlockReady)
            {
                std::copy(fifoBuffer, fifoBuffer + fftSize, fftData);
                nextFFTBlockReady = true;
            }
            fifoIndex = 0;
        }
        fifoBuffer[fifoIndex++] = channelData[i];
    }
    outputLevelPeak.store(outPeak);
}

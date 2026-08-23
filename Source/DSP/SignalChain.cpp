#include "SignalChain.h"
#include <algorithm>
#include <cmath>

SignalChain::SignalChain()
{
}

SignalChain::~SignalChain()
{
}

void SignalChain::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;

    dryBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
    parallelSatBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
    parallelShadowBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
    parallelWidthBuffer.setSize(spec.numChannels, spec.maximumBlockSize);
    parallelSpaceBuffer.setSize(spec.numChannels, spec.maximumBlockSize);

    inputGainSmoother.reset(sampleRate, 0.02);
    outputGainSmoother.reset(sampleRate, 0.02);
    globalMixSmoother.reset(sampleRate, 0.02);
    degenerateSmoother.reset(sampleRate, 0.02);

    inputConditioner.prepare(spec);
    analogTransformerCore.prepare(spec);
    vocalResonanceProcessor.prepare(spec);
    transientPreserver.prepare(spec);
    studioMicroDetuner.prepare(spec);
    autoLevelCompensator.prepare(spec);

    masterEQEngine.prepare(sampleRate, spec.maximumBlockSize);

    juce::dsp::ProcessSpec monoSpec = spec;
    outputLimiter.prepare(spec);
    outputLimiter.setThreshold(0.0f);
    outputLimiter.setRelease(35.0f);

    reset();
}

void SignalChain::reset()
{
    inputConditioner.reset();
    analogTransformerCore.reset();
    vocalResonanceProcessor.reset();
    transientPreserver.reset();
    studioMicroDetuner.reset();
    autoLevelCompensator.reset();

    masterEQEngine.reset();
    outputLimiter.reset();

    fifoIndex = 0;
    std::fill(std::begin(fifoBuffer), std::end(fifoBuffer), 0.0f);
    std::fill(std::begin(fftData), std::end(fftData), 0.0f);
    std::fill(std::begin(spectrumBars), std::end(spectrumBars), 0.0f);

    inputGainSmoother.setCurrentAndTargetValue(inputGainSmoother.getTargetValue());
    outputGainSmoother.setCurrentAndTargetValue(outputGainSmoother.getTargetValue());
    globalMixSmoother.setCurrentAndTargetValue(globalMixSmoother.getTargetValue());
    degenerateSmoother.setCurrentAndTargetValue(degenerateSmoother.getTargetValue());
}

void SignalChain::getSpectrumData(float* dest64Bars) const noexcept
{
    if (dest64Bars == nullptr) return;

    if (nextFFTBlockReady.load())
    {
        float localFFT[fftSize * 2] = { 0.0f };
        std::copy(fftData, fftData + fftSize, localFFT);

        const_cast<juce::dsp::WindowingFunction<float>&>(windowEngine).multiplyWithWindowingTable(localFFT, fftSize);
        const_cast<juce::dsp::FFT&>(fftEngine).performFrequencyOnlyForwardTransform(localFFT);

        int numBins = fftSize / 2;
        for (int i = 0; i < 64; ++i)
        {
            float normIdx = (float)i / 64.0f;
            float freqHz = 20.0f * std::pow(1000.0f, normIdx);
            int bin = std::clamp((int)(freqHz * (float)fftSize / (float)sampleRate), 1, numBins - 1);

            float mag = localFFT[bin];
            float db = juce::Decibels::gainToDecibels(mag, -80.0f);
            float barLevel = std::clamp((db + 60.0f) / 60.0f, 0.0f, 1.0f);

            spectrumBars[i] = std::max(barLevel, spectrumBars[i] * 0.75f);
        }
        nextFFTBlockReady.store(false);
    }
    else
    {
        for (int i = 0; i < 64; ++i)
            spectrumBars[i] *= 0.85f;
    }

    std::copy(spectrumBars, spectrumBars + 64, dest64Bars);
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
                          float compSqueeze,
                          float deEssAmount,
                          float deEssFreq,
                          float airMid,
                          float airTop,
                          float spaceDucking,
                          ShadowProcessor& shadowModule,
                          CrushProcessor& crushModule,
                          WidthProcessor& widthModule,
                          SpaceProcessor& spaceModule,
                          DeviceProcessor& deviceModule,
                          VocalCompressor& compModule,
                          DeEsserProcessor& deEssModule,
                          AirExciterProcessor& airModule)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numSamples == 0 || numChannels == 0)
        return;

    // 1. Input Conditioning & Sub-DC Cleaning & Soft Headroom Protection
    float inputLinear = juce::Decibels::decibelsToGain(inputGainDb);
    inputConditioner.process(buffer, inputLinear);
    inputLevelPeak.store(inputConditioner.getPeakLevel());

    // 2. Analog Transformer Iron Core Preamp Saturation (Neve 1073 / Tube-Tech Warmth)
    analogTransformerCore.setWarmth(0.50f + 0.35f * compSqueeze);
    analogTransformerCore.process(buffer);

    // 3. Save pre-allocated Dry Buffer copy for clean dry/wet blend & auto-loudness tracking
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // 4. Process Unified 64-Bit Transposed Direct Form II Master EQ Engine
    {
        float safeLowQ  = std::clamp(eqLowQ, 0.10f, 50.0f);
        float safeMidQ  = std::clamp(eqMidQ, 0.10f, 50.0f);
        float safeHighQ = std::clamp(eqHighQ, 0.10f, 50.0f);

        // Fixed Band 0: Low Cut (High Pass)
        masterEQEngine.updateBand(0, TransposedDirectFormIIBiquad::LowCut, std::clamp((double)eqLowCutFreq, 20.0, 500.0), 0.0, 0.707, eqLowCutFreq > 21.0f);

        // Fixed Band 1: Low Bell (200Hz warm punch)
        masterEQEngine.updateBand(1, TransposedDirectFormIIBiquad::Peaking, 200.0, (double)eqLowGainDb, (double)safeLowQ, std::abs(eqLowGainDb) > 0.05f);

        // Fixed Band 2: Mid Bell (2200Hz clarity)
        masterEQEngine.updateBand(2, TransposedDirectFormIIBiquad::Peaking, 2200.0, (double)eqMidGainDb, (double)safeMidQ, std::abs(eqMidGainDb) > 0.05f);

        // Fixed Band 3: High Shelf (8000Hz air)
        masterEQEngine.updateBand(3, TransposedDirectFormIIBiquad::HighShelf, 8000.0, (double)eqHighGainDb, (double)safeHighQ, std::abs(eqHighGainDb) > 0.05f);

        // Fixed Band 4: High Cut (Low Pass)
        masterEQEngine.updateBand(4, TransposedDirectFormIIBiquad::HighCut, std::clamp((double)eqHighCutFreq, 500.0, 20000.0), 0.0, 0.707, eqHighCutFreq < 19500.0f);

        // Dynamic Interactive UI Bands (Bands 5..15)
        juce::ScopedLock sl(nodeLock);
        int numDyn = std::min((int)activeNodes.size(), maxDynamicNodes);
        for (int i = 0; i < maxDynamicNodes; ++i)
        {
            int engineBandIdx = 5 + i;
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

                masterEQEngine.updateBand(engineBandIdx, engineType, fHz, gDb, qVal, true);
            }
            else
            {
                masterEQEngine.updateBand(engineBandIdx, TransposedDirectFormIIBiquad::Peaking, 1000.0, 0.0, 0.707, false);
            }
        }

        masterEQEngine.processBlock(buffer);
    }

    // 6. Dynamic Split-Band De-Esser (Silk Vocal / Pro-DS caliber)
    if (deEssAmount > 0.001f)
    {
        deEssModule.setAmount(deEssAmount);
        deEssModule.setFrequency(deEssFreq);
        deEssModule.process(buffer);
    }

    // 7. Dual-Stage Vocal Dynamics & Leveling Engine (R-Vox / 1176 FET / LA-2A)
    if (compSqueeze > 0.001f)
    {
        compModule.setSqueeze(compSqueeze);
        compModule.process(buffer);
    }

    // 8. Dynamic Vocal Resonance & Harshness Suppression Engine
    vocalResonanceProcessor.process(buffer);

    // 9. Psychoacoustic Fresh Air Exciter (Mid-Air & Top-Air Sheen)
    if (airMid > 0.001f || airTop > 0.001f)
    {
        airModule.setMidAir(airMid);
        airModule.setTopAir(airTop);
        airModule.process(buffer);
    }

    // 10. Transient & Consonant Attack Protection Analysis
    float transProt = transientPreserver.analyze(buffer);
    crushModule.setTransientProtection(transProt);

    // 11. Non-Linear DEGENERATE Demonic Vocal Multi-FX Matrix (Parabolic Morphing)
    float degenVal = std::clamp(degenerateMacro, 0.0f, 1.0f);

    if (degenVal > 0.01f)
    {
        float degenCurve = degenVal * degenVal;

        // Smoothly pull formant down into the deep resonant chest cavity without phase distortion
        float currentFormant = shadowModule.getFormantShift();
        float deeperFormant = std::clamp(currentFormant - 0.25f * degenVal, 0.15f, 1.0f);
        shadowModule.setFormantShift(deeperFormant);

        shadowModule.setDrive(std::clamp(shadowModule.getDrive() + 0.20f * degenCurve, 0.0f, 0.70f));
        shadowModule.setMix(std::clamp(shadowModule.getMix() + 0.25f * degenVal, 0.0f, 0.85f));

        widthModule.setAmount(std::clamp(widthModule.getAmount() + 0.25f * degenVal, 0.0f, 0.90f));
        studioMicroDetuner.setAmount(std::clamp(studioMicroDetuner.getAmount() + 0.25f * degenVal, 0.0f, 0.80f));
    }

    // 12. Parallel Texture Processing (Shadow sub-harmonics & Crush analog saturation)
    if (subEnable > 0.5f && shadowModule.getMix() > 0.001f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            parallelShadowBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
        
        shadowModule.process(parallelShadowBuffer);

        // Transient-aware consonant protection: reduce shadow during hard plosives only
        // NOTE: 0.65 scalar removed — was causing SHADOW_MIX to deliver only ~14% of dialed-in level
        float shadowGain = 1.0f - transProt * 0.35f;
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.addFrom(ch, 0, parallelShadowBuffer, ch, 0, numSamples, shadowGain);
    }

    if (gritEnable > 0.5f && (crushModule.getAmount() > 0.001f || crushModule.isPunishEnabled()))
    {
        crushModule.process(buffer);
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // 13. Stereo Dimension & Soundtoys MicroShift 3D Aura Detuner
    if (modEnable > 0.5f && (widthModule.getAmount() > 0.001f || degenVal > 0.05f))
    {
        widthModule.process(context);
        if (studioMicroDetuner.getAmount() > 0.001f)
            studioMicroDetuner.process(buffer);
    }
    
    // 14. Space Processor with Auto-Ducking Envelope Follower & Transducer Box
    spaceModule.setDucking(spaceDucking);
    if (delayEnable > 0.5f || reverbEnable > 0.5f)
        spaceModule.process(context);
    deviceModule.process(context);

    // 14.5. Auto-Level Loudness Matching Engine (Compensates perceived loudness transparently)
    autoLevelCompensator.process(dryBuffer, buffer);

    // 15. Global Dry/Wet Mix
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

    // 16. Output Trim Gain & Pro Limiter
    outputGainSmoother.setTargetValue(juce::Decibels::decibelsToGain(outputGainDb));
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

    // 17. Output Peak Level & Push Audio Samples to 2048-Point FFT FIFO
    float outPeak = 0.0f;
    const float* channelData = buffer.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        float s = std::abs(channelData[i]);
        if (s > outPeak) outPeak = s;

        fifoBuffer[fifoIndex] = channelData[i];
        if (++fifoIndex >= fftSize)
        {
            if (!nextFFTBlockReady.load())
            {
                std::copy(fifoBuffer, fifoBuffer + fftSize, fftData);
                std::fill(fftData + fftSize, fftData + (fftSize * 2), 0.0f);
                nextFFTBlockReady.store(true);
            }
            fifoIndex = 0;
        }
    }
    outputLevelPeak.store(outPeak);
}

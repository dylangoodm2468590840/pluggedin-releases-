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

    // Early silence gate: If incoming signal is pure silence/below -100 dBFS, zero and return immediately!
    // This eliminates all background hiss, phase vocoder idling, and compressor makeup noise on silence.
    float inPeakRaw = buffer.getMagnitude(0, numSamples);
    if (inPeakRaw < 1.0e-5f)
    {
        buffer.clear();
        inputLevelPeak.store(0.0f);
        outputLevelPeak.store(0.0f);
        return;
    }

    // =========================================================================
    // STAGE 1: Input Conditioning & Sub-Rumble Protection
    // =========================================================================
    float inputLinear = juce::Decibels::decibelsToGain(inputGainDb);
    inputConditioner.process(buffer, inputLinear);
    inputLevelPeak.store(inputConditioner.getPeakLevel());

    // Save bit-exact Dry Buffer copy for parallel wet/dry blending & loudness tracking
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // =========================================================================
    // STAGE 2: 5-Band High-Definition Master EQ Engine (64-Bit TDF-II)
    // =========================================================================
    {
        float safeLowQ  = std::clamp(eqLowQ, 0.10f, 50.0f);
        float safeMidQ  = std::clamp(eqMidQ, 0.10f, 50.0f);
        float safeHighQ = std::clamp(eqHighQ, 0.10f, 50.0f);

        // Fixed Band 0: Low Cut (High Pass - Clears mud and protects 808 subs)
        masterEQEngine.updateBand(0, TransposedDirectFormIIBiquad::LowCut, std::clamp((double)eqLowCutFreq, 20.0, 500.0), 0.0, 0.707, eqLowCutFreq > 21.0f);

        // Fixed Band 1: Low Bell (200Hz warm punch)
        masterEQEngine.updateBand(1, TransposedDirectFormIIBiquad::Peaking, 200.0, (double)eqLowGainDb, (double)safeLowQ, std::abs(eqLowGainDb) > 0.05f);

        // Fixed Band 2: Mid Bell (2200Hz presence & intelligibility)
        masterEQEngine.updateBand(2, TransposedDirectFormIIBiquad::Peaking, 2200.0, (double)eqMidGainDb, (double)safeMidQ, std::abs(eqMidGainDb) > 0.05f);

        // Fixed Band 3: High Shelf (8000Hz air sheen)
        masterEQEngine.updateBand(3, TransposedDirectFormIIBiquad::HighShelf, 8000.0, (double)eqHighGainDb, (double)safeHighQ, std::abs(eqHighGainDb) > 0.05f);

        // Fixed Band 4: High Cut (Low Pass)
        masterEQEngine.updateBand(4, TransposedDirectFormIIBiquad::HighCut, std::clamp((double)eqHighCutFreq, 500.0, 20000.0), 0.0, 0.707, eqHighCutFreq < 19500.0f);

        // Dynamic Interactive UI EQ Bands (Bands 5..15)
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

    // =========================================================================
    // STAGE 3: Vocal Dynamics & Air (De-Esser, FET Compressor, Fresh Air)
    // =========================================================================
    if (deEssAmount > 0.001f)
    {
        deEssModule.setAmount(deEssAmount);
        deEssModule.setFrequency(deEssFreq);
        deEssModule.process(buffer);
    }

    if (compSqueeze > 0.001f)
    {
        compModule.setSqueeze(compSqueeze);
        compModule.process(buffer);
    }

    if (airMid > 0.001f || airTop > 0.001f)
    {
        airModule.setMidAir(airMid);
        airModule.setTopAir(airTop);
        airModule.process(buffer);
    }

    // =========================================================================
    // DEGENERATE Signature Macro (Deterministic, Single Musical Transformation)
    // =========================================================================
    float degenVal = std::clamp(degenerateMacro, 0.0f, 1.0f);
    if (degenVal > 0.01f)
    {
        // 0.0 -> 0.35: Deepens pitch transposition (-12 semitones) and pulls formants down
        float pitchTarget = (subEnable > 0.5f) ? (-12.0f * std::clamp(degenVal * 2.85f, 0.0f, 1.0f)) : 0.0f;
        float formantTarget = (subEnable > 0.5f) ? (-5.0f * std::clamp(degenVal * 2.5f, 0.0f, 1.0f)) : 0.0f;
        float shadowMixTarget = (subEnable > 0.5f) ? std::clamp(degenVal * 0.85f, 0.0f, 0.85f) : 0.0f;

        shadowModule.setPitchSemitones(pitchTarget);
        shadowModule.setFormantSemitones(formantTarget);
        shadowModule.setMix(std::max(shadowModule.getMix(), shadowMixTarget));

        // 0.35 -> 0.70: Injects 12AX7 tube saturation drive
        float driveTarget = std::clamp((degenVal - 0.20f) * 0.85f, 0.0f, 0.75f);
        crushModule.setAmount(std::max(crushModule.getAmount(), driveTarget));

        // 0.70 -> 1.00: Expands stereo width aura
        float widthTarget = std::clamp((degenVal - 0.30f) * 0.90f, 0.0f, 0.85f);
        widthModule.setAmount(std::max(widthModule.getAmount(), widthTarget));
    }

    // =========================================================================
    // STAGE 4: Boutique Pitch & Formant Demon Engine (Shadow Processor)
    // =========================================================================
    if (subEnable > 0.5f && shadowModule.getMix() > 0.001f)
    {
        shadowModule.process(buffer);
    }

    if (gritEnable > 0.5f && (crushModule.getAmount() > 0.001f || crushModule.isPunishEnabled()))
    {
        crushModule.process(buffer);
    }

    // =========================================================================
    // STAGE 5: 3D Spatial Aura Engine (Stereo Width + Auto-Ducking Plate/Delay)
    // =========================================================================
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    if (modEnable > 0.5f && (widthModule.getAmount() > 0.001f || degenVal > 0.05f))
    {
        widthModule.process(context);
    }

    spaceModule.setDucking(spaceDucking);
    if (delayEnable > 0.5f || reverbEnable > 0.5f)
    {
        spaceModule.process(context);
    }

    deviceModule.process(context);

    // =========================================================================
    // STAGE 6: Auto-Level Matching, Global Dry/Wet, Trim & Limiter
    // =========================================================================
    autoLevelCompensator.process(dryBuffer, buffer);

    // Global Dry/Wet Mix
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

    // Output Trim Gain
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

    // Safety Mastering Limiter
    outputLimiter.process(context);

    // FFT Spectrum Visualizer FIFO
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

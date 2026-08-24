#include "ShadowProcessor.h"
#include <cmath>
#include <algorithm>

ShadowProcessor::ShadowProcessor()
{
}

ShadowProcessor::~ShadowProcessor() = default;

void ShadowProcessor::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate > 0.0 ? spec.sampleRate : 44100.0;
    numChannels = std::max(1, static_cast<int>(spec.numChannels));

    // Vocal-Optimized Signalsmith Stretch Phase Vocoder:
    // 22ms analysis window (960 samples @ 44.1k) with 5.5ms hop (240 samples)
    // Eliminates the 120ms polyphonic phase smearing, transient destruction, and white noise!
    int blockSamples = static_cast<int>(sampleRate * 0.022);
    int intervalSamples = static_cast<int>(sampleRate * 0.0055);
    stretchEngine.configure(numChannels, blockSamples, intervalSamples);
    latencySamples = stretchEngine.outputLatency();

    // Pre-allocate scratch processing buffers (Zero audio-thread allocations)
    const int maxBlock = static_cast<int>(spec.maximumBlockSize) + 512;
    stretchScratchBuffer.setSize(numChannels, maxBlock);
    stretchScratchBuffer.clear();

    inputChannelPointers.resize(numChannels, nullptr);
    outputChannelPointers.resize(numChannels, nullptr);

    // Smoothers (30ms zipper-free glide)
    mixSmoother.reset(sampleRate, 0.02);
    pitchSmoother.reset(sampleRate, 0.030);
    formantSmoother.reset(sampleRate, 0.030);
    driveSmoother.reset(sampleRate, 0.02);
    claritySmoother.reset(sampleRate, 0.02);

    for (int ch = 0; ch < 2; ++ch)
    {
        dryDelayLine[ch].prepare(spec);
        dryDelayLine[ch].setMaximumDelayInSamples(latencySamples + 2048);
        dryDelayLine[ch].setDelay(static_cast<float>(latencySamples));

        toneDarknessFilter[ch].prepare(spec);
        toneDarknessFilter[ch].setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        toneDarknessFilter[ch].setCutoffFrequency(16000.0f);
        toneDarknessFilter[ch].setResonance(0.707f);

        subRumbleHighpass[ch].prepare(spec);
        subRumbleHighpass[ch].setType(juce::dsp::StateVariableTPTFilterType::highpass);
        subRumbleHighpass[ch].setCutoffFrequency(35.0f);
        subRumbleHighpass[ch].setResonance(0.707f);

        dcBlocker[ch].prepare(sampleRate, 20.0f);
    }

    reset();
}

void ShadowProcessor::reset()
{
    stretchEngine.reset();
    stretchScratchBuffer.clear();

    mixSmoother.setCurrentAndTargetValue(mix);
    pitchSmoother.setCurrentAndTargetValue(pitchSemitones);
    formantSmoother.setCurrentAndTargetValue(formantSemitones);
    driveSmoother.setCurrentAndTargetValue(drive);
    claritySmoother.setCurrentAndTargetValue(clarity);

    for (int ch = 0; ch < 2; ++ch)
    {
        dryDelayLine[ch].reset();
        toneDarknessFilter[ch].reset();
        subRumbleHighpass[ch].reset();
        dcBlocker[ch].reset();
    }
}

void ShadowProcessor::setPitchInterval(PitchInterval interval) noexcept
{
    switch (interval)
    {
        case PitchInterval::OctaveDown: setPitchSemitones(-12.0f); break;
        case PitchInterval::FifthDown:  setPitchSemitones(-7.0f);  break;
        case PitchInterval::FourthDown: setPitchSemitones(-5.0f);  break;
        case PitchInterval::TwoOctaves: setPitchSemitones(-24.0f); break;
    }
}

ShadowProcessor::PitchInterval ShadowProcessor::getPitchInterval() const noexcept
{
    if (pitchSemitones <= -18.0f) return PitchInterval::TwoOctaves;
    if (pitchSemitones <= -9.0f)  return PitchInterval::OctaveDown;
    if (pitchSemitones <= -6.0f)  return PitchInterval::FifthDown;
    return PitchInterval::FourthDown;
}

void ShadowProcessor::setFormantShift(float normalizedShift) noexcept
{
    setFormantSemitones((std::clamp(normalizedShift, 0.0f, 1.0f) - 0.5f) * 24.0f);
}

float ShadowProcessor::getFormantShift() const noexcept
{
    return std::clamp((formantSemitones + 12.0f) / 24.0f, 0.0f, 1.0f);
}

void ShadowProcessor::process(juce::AudioBuffer<float>& buffer)
{
    if (!enabled)
        return;

    const int nChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    if (nChannels == 0 || numSamples == 0)
        return;

    // Silence gating: bypass and flush if input buffer is silence/whisper below -100 dBFS
    float inPeak = buffer.getMagnitude(0, numSamples);
    if (inPeak < 1.0e-5f)
    {
        buffer.clear();
        return;
    }

    // Ensure scratch buffer capacity
    if (stretchScratchBuffer.getNumSamples() < numSamples || stretchScratchBuffer.getNumChannels() < nChannels)
    {
        stretchScratchBuffer.setSize(nChannels, numSamples + 256, false, false, true);
    }
    inputChannelPointers.resize(nChannels);
    outputChannelPointers.resize(nChannels);

    for (int ch = 0; ch < nChannels; ++ch)
    {
        inputChannelPointers[ch] = buffer.getReadPointer(ch);
        outputChannelPointers[ch] = stretchScratchBuffer.getWritePointer(ch);
    }

    // Advance smoothers to target
    float targetPitchST = pitchSmoother.getNextValue();
    float targetFormantST = formantSmoother.getNextValue();
    float curDrive = driveSmoother.getNextValue();
    float currentMix = mixSmoother.getNextValue();

    if (linkEnabled)
    {
        targetFormantST = targetPitchST * 0.50f;
    }

    if (mode == DemonMode::HardTune)
    {
        targetPitchST = std::round(targetPitchST);
    }
    else if (mode == DemonMode::Robot)
    {
        // Robot mode: locked deep drone
        targetPitchST = -12.0f;
        targetFormantST = -3.5f;
    }

    // If no pitch or formant transposition is active and no drive, bypass vocoder for 100% pristine fidelity
    if (std::abs(targetPitchST) < 0.01f && std::abs(targetFormantST) < 0.01f && curDrive < 0.01f)
    {
        for (int ch = 0; ch < nChannels; ++ch)
            stretchScratchBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }
    else
    {
        // Configure vocal-tuned Signalsmith Stretch parameters:
        // Tonality limit: 0.45 keeps high-frequency breath/sibilants clean without phase noise!
        stretchEngine.setTransposeSemitones(targetPitchST, 0.45f);
        stretchEngine.setFormantSemitones(targetFormantST, true);

        // Run phase-locked spectral transformation
        stretchEngine.process(inputChannelPointers.data(), numSamples, outputChannelPointers.data(), numSamples);
    }

    // Equal-Power Wet/Dry Crossfade calculation
    float wetMix = std::clamp(currentMix, 0.0f, 1.0f);
    float dryGain = std::cos(wetMix * 0.5f * juce::MathConstants<float>::pi);
    float wetGain = std::sin(wetMix * 0.5f * juce::MathConstants<float>::pi);

    // Post-processing: 12AX7 tube saturation, darkness tone filter, and sub-rumble cleanup
    const float driveGain = 1.0f + curDrive * 2.5f;
    const float toneCutoff = std::clamp(1000.0f + (1.0f - darkness) * 16000.0f, 1000.0f, static_cast<float>(sampleRate * 0.45));

    for (int ch = 0; ch < nChannels; ++ch)
    {
        const int filterCh = std::min(ch, 1);
        toneDarknessFilter[filterCh].setCutoffFrequency(toneCutoff);

        float* outData = buffer.getWritePointer(ch);
        const float* wetData = stretchScratchBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float inDry = AudioUtils::sanitize(outData[i]);

            // Delay-compensate dry signal to align perfectly with vocoder latency
            dryDelayLine[filterCh].pushSample(0, inDry);
            float alignedDry = dryDelayLine[filterCh].popSample(0);

            float wetSample = AudioUtils::sanitize(wetData[i]);

            // Clean 12AX7 tube saturation on the pitched vocal (Murda Melodies / AlterBoy style)
            if (curDrive > 0.01f)
            {
                float biased = wetSample * driveGain;
                wetSample = std::tanh(biased) / (1.0f + curDrive * 0.30f);
            }

            // Darkness & Tone filter
            float toneShaped = toneDarknessFilter[filterCh].processSample(0, wetSample);

            // Sub-rumble cleanup (35Hz HPF) & DC block
            float cleanWet = subRumbleHighpass[filterCh].processSample(0, toneShaped);
            cleanWet = dcBlocker[filterCh].process(cleanWet);

            // Latency-compensated equal-power blend
            outData[i] = AudioUtils::sanitize(alignedDry * dryGain + cleanWet * wetGain);
        }
    }
}

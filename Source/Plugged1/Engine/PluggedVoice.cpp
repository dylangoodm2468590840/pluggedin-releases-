#include "PluggedVoice.h"
#include <cmath>

namespace Plugged1
{

PluggedVoice::PluggedVoice()
{
    subOsc.setWaveform(PolyBLEP::Waveform::Sine);
    for (auto& osc : synthOscs)
        osc.setWaveform(PolyBLEP::Waveform::Saw);
}

bool PluggedVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<PluggedSound*>(sound) != nullptr;
}

void PluggedVoice::setCurrentPlaybackSampleRate(double newRate)
{
    SynthesiserVoice::setCurrentPlaybackSampleRate(newRate);
    currentSampleRate = newRate > 0.0 ? newRate : 44100.0;

    ampAdsr.setSampleRate(currentSampleRate);
    filtAdsr.setSampleRate(currentSampleRate);

    subOsc.setSampleRate(currentSampleRate);
    for (auto& osc : synthOscs)
        osc.setSampleRate(currentSampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;
    svfFilter.prepare(spec);
    svfFilter.reset();

    updateEnvelopes();
}

void PluggedVoice::updateVoiceParams(const VoiceParams& newParams)
{
    params = newParams;
    updateEnvelopes();
}

void PluggedVoice::updateEnvelopes()
{
    ampAdsrParams.attack  = params.ampAttackMs / 1000.0f;
    ampAdsrParams.decay   = params.ampDecayMs / 1000.0f;
    ampAdsrParams.sustain = params.ampSustain;
    ampAdsrParams.release = params.ampReleaseMs / 1000.0f;
    ampAdsr.setParameters(ampAdsrParams);

    filtAdsrParams.attack  = params.filtAttackMs / 1000.0f;
    filtAdsrParams.decay   = params.filtDecayMs / 1000.0f;
    filtAdsrParams.sustain = params.filtSustain;
    filtAdsrParams.release = params.filtReleaseMs / 1000.0f;
    filtAdsr.setParameters(filtAdsrParams);

    // Calculate punch decay coefficient per sample
    float punchTimeSec = std::max(0.005f, (params.subPunchDecayMs / 1000.0f) * (1.0f - params.macroPunch * 0.5f));
    punchDecayCoeff = std::exp(-1.0f / (punchTimeSec * static_cast<float>(currentSampleRate)));

    // Calculate glide coefficient
    if (params.glideTimeMs <= 1.0f)
    {
        glideCoeff = 1.0f; // Instant
    }
    else
    {
        float glideSec = params.glideTimeMs / 1000.0f;
        glideCoeff = 1.0f - std::exp(-1.0f / (glideSec * static_cast<float>(currentSampleRate) * 0.25f));
    }
}

void PluggedVoice::setTargetPitch(int midiNoteNumber, float velocity, bool isLegatoGlide)
{
    currentPlayingNoteNumber = midiNoteNumber;
    targetFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    currentVelocity = std::clamp(velocity, 0.05f, 1.0f);

    if (!isLegatoGlide)
    {
        isPlaying = true;
        currentFrequency = targetFrequency;
        punchEnvelope = 1.0f; // Trigger 808 transient kick dive
        ampAdsr.noteOn();
        filtAdsr.noteOn();
    }
}

void PluggedVoice::startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound* /*sound*/, int currentPitchWheelPosition)
{
    isPlaying = true;
    currentPlayingNoteNumber = midiNoteNumber;
    currentVelocity = std::clamp(velocity, 0.05f, 1.0f);
    pitchWheelMoved(currentPitchWheelPosition);
    setTargetPitch(midiNoteNumber, currentVelocity, false);

    subOsc.reset();
    for (auto& osc : synthOscs)
        osc.reset();

    svfFilter.reset();
}

void PluggedVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        ampAdsr.noteOff();
        filtAdsr.noteOff();
    }
    else
    {
        isPlaying = false;
        currentPlayingNoteNumber = -1;
        ampAdsr.reset();
        filtAdsr.reset();
        clearCurrentNote();
    }
}

void PluggedVoice::pitchWheelMoved(int newPitchWheelValue)
{
    // Default 2 semitones bend
    float bendSemitones = (static_cast<float>(newPitchWheelValue - 8192) / 8192.0f) * 2.0f;
    pitchBendRatio = std::pow(2.0f, bendSemitones / 12.0f);
}

void PluggedVoice::controllerMoved(int /*controllerNumber*/, int /*newControllerValue*/)
{
}

float PluggedVoice::processSubSample(double currentFreq)
{
    if (!params.subEnabled)
        return 0.0f;

    // Apply octave shift & sub tuning
    float subNoteFreq = static_cast<float>(currentFreq * std::pow(2.0, params.subOctave + (params.subTuneSemi / 12.0)));

    // Apply 808 transient punch (pitch dive from 1.0 to 4.0x freq)
    float punchMultiplier = 1.0f + (punchEnvelope * (params.subPunchAmount + params.macroPunch * 0.5f) * 3.5f);
    float finalSubFreq = std::clamp(subNoteFreq * punchMultiplier, 10.0f, 1200.0f);

    subOsc.setFrequency(finalSubFreq);
    
    // 5 Dedicated Sub Algorithms
    switch (params.subWaveform)
    {
        case 0: subOsc.setWaveform(PolyBLEP::Waveform::Sine); break;      // Pure Clean Sine
        case 1: subOsc.setWaveform(PolyBLEP::Waveform::Triangle); break;  // Warm Triangle
        case 2: subOsc.setWaveform(PolyBLEP::Waveform::Sine); break;      // Tube Saturated Sine
        case 3: subOsc.setWaveform(PolyBLEP::Waveform::Saw); break;       // Drill Distort Saw
        case 4: subOsc.setWaveform(PolyBLEP::Waveform::Square); break;    // Punch Transient Square
        default: subOsc.setWaveform(PolyBLEP::Waveform::Sine); break;
    }

    float subSample = subOsc.process();

    // Dedicated Sub Distortion Modes
    float drive = std::clamp(params.subDrive + params.macroDirt * 0.6f, 0.0f, 1.0f);
    if (params.subWaveform == 3) // Drill Foldback Distortion
    {
        float gain = 1.0f + drive * 8.0f;
        float driven = std::sin(subSample * gain);
        subSample = std::tanh(driven * 1.5f);
    }
    else if (params.subWaveform == 2) // Tube Warmth
    {
        float gain = 1.0f + drive * 5.0f;
        float driven = std::tanh(subSample * gain);
        subSample = driven + (subSample * subSample * 0.25f * drive);
    }
    else if (params.subWaveform == 4) // Slap Punch Knock
    {
        float gain = 1.0f + drive * 3.5f;
        float driven = std::clamp(subSample * gain, -0.95f, 0.95f);
        subSample = driven + (punchEnvelope * 0.35f);
    }
    else if (drive > 0.01f)
    {
        float gain = 1.0f + drive * 4.0f;
        subSample = std::tanh(subSample * gain);
    }

    return subSample * params.subGain;
}

float PluggedVoice::processSynthSample(double currentFreq, float& rightChannelOut)
{
    if (!params.synthEnabled)
    {
        rightChannelOut = 0.0f;
        return 0.0f;
    }

    float synthNoteFreq = static_cast<float>(currentFreq * std::pow(2.0, params.synthOctave));
    int numVoices = std::clamp(params.synthUnison, 1, maxUnison);

    PolyBLEP::Waveform wave = PolyBLEP::Waveform::Saw;
    switch (params.synthShape)
    {
        case 0: wave = PolyBLEP::Waveform::Saw; break;
        case 1: wave = PolyBLEP::Waveform::Square; break;
        case 2: wave = PolyBLEP::Waveform::Triangle; break;
        case 3: wave = PolyBLEP::Waveform::AcousticGrand; break;
        case 4: wave = PolyBLEP::Waveform::VintageRhodes; break;
        case 5: wave = PolyBLEP::Waveform::FMBell; break;
        case 6: wave = PolyBLEP::Waveform::PluckGuitar; break;
        case 7: wave = PolyBLEP::Waveform::VocalFormant; break;
        case 8: wave = PolyBLEP::Waveform::Supersaw; break;
        case 9: wave = PolyBLEP::Waveform::DrawbarOrgan; break;
        case 10: wave = PolyBLEP::Waveform::AcidSync; break;
        default: wave = PolyBLEP::Waveform::Saw; break;
    }

    float leftSum = 0.0f;
    float rightSum = 0.0f;

    for (int i = 0; i < numVoices; ++i)
    {
        synthOscs[i].setWaveform(wave);

        // Unison detune spread
        float detuneOffset = 0.0f;
        float pan = 0.5f;

        if (numVoices > 1)
        {
            float spreadIndex = (static_cast<float>(i) / static_cast<float>(numVoices - 1)) * 2.0f - 1.0f; // -1 to +1
            detuneOffset = spreadIndex * params.synthDetune * 0.05f; // Semitones
            pan = 0.5f + (spreadIndex * params.synthSpread * 0.4f);
        }

        float voiceFreq = synthNoteFreq * std::pow(2.0f, detuneOffset);
        synthOscs[i].setFrequency(voiceFreq);

        float s = synthOscs[i].process();
        leftSum += s * (1.0f - pan);
        rightSum += s * pan;
    }

    float norm = 1.0f / std::sqrt(static_cast<float>(numVoices));
    rightChannelOut = rightSum * norm * params.synthGain;
    return leftSum * norm * params.synthGain;
}

void PluggedVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (!isVoiceActive())
        return;

    auto* leftOut = outputBuffer.getWritePointer(0);
    auto* rightOut = outputBuffer.getNumChannels() > 1 ? outputBuffer.getWritePointer(1) : nullptr;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Smooth pitch glide toward target frequency
        currentFrequency += (targetFrequency - currentFrequency) * glideCoeff;

        // Advance punch envelope
        punchEnvelope *= punchDecayCoeff;

        // Process envelopes
        float ampVal = ampAdsr.getNextSample() * currentVelocity;
        float filtVal = filtAdsr.getNextSample();

        if (!ampAdsr.isActive())
        {
            isPlaying = false;
            currentPlayingNoteNumber = -1;
            clearCurrentNote();
            break;
        }

        double finalFreq = currentFrequency * pitchBendRatio;

        // Generate Layer 1 (808 / Sub)
        float subSample = processSubSample(finalFreq);

        // Generate Layer 2 (Synth / Leads / Brass)
        float synthR = 0.0f;
        float synthL = processSynthSample(finalFreq, synthR);

        // Mix layers
        float mixL = subSample + synthL;
        float mixR = subSample + synthR;

        // Dynamic Filter Processing per voice
        float effectiveCutoff = params.cutoffHz + (params.envAmount * filtVal * 12000.0f) + (params.macroAir * 4000.0f);
        effectiveCutoff = std::clamp(effectiveCutoff, 20.0f, 20000.0f);

        svfFilter.setCutoffFrequency(effectiveCutoff);
        svfFilter.setResonance(std::clamp(params.resonance, 0.1f, 8.0f));

        switch (params.filterType)
        {
            case 0: svfFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass); break;
            case 1: svfFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass); break;
            case 2: svfFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass); break;
            case 3: svfFilter.setType(juce::dsp::StateVariableTPTFilterType::bandpass); break;
            default: svfFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass); break;
        }

        // Apply filter to synth / highs (keep sub clear or filtered according to design)
        float filteredL = svfFilter.processSample(0, mixL);
        float filteredR = svfFilter.processSample(0, mixR);

        // Apply Amp Envelope
        float outL = filteredL * ampVal;
        float outR = filteredR * ampVal;

        int destIndex = startSample + sample;
        leftOut[destIndex] += outL;
        if (rightOut != nullptr)
            rightOut[destIndex] += outR;
    }
}

} // namespace Plugged1

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace Plugged1
{

Plugged1AudioProcessor::Plugged1AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    presetManager = std::make_unique<PresetManager>(apvts);
    // Load initial default preset
    presetManager->loadPreset(0);
}

juce::AudioProcessorValueTreeState::ParameterLayout Plugged1AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Macro Knobs
    params.push_back(std::make_unique<juce::AudioParameterFloat>("macro_punch", "Punch", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("macro_dirt", "Dirt", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("macro_space", "Space", 0.0f, 1.0f, 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("macro_air", "Air", 0.0f, 1.0f, 0.0f));

    // Voice & Master
    params.push_back(std::make_unique<juce::AudioParameterChoice>("voice_mode", "Voice Mode", juce::StringArray{"Poly", "Mono", "Legato"}, 2));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("glide_time", "Glide Time", juce::NormalisableRange<float>(0.0f, 1000.0f, 1.0f, 0.35f), 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("master_gain", "Master Gain", juce::NormalisableRange<float>(-60.0f, 6.0f, 0.1f), 0.0f));

    // Layer 1: Sub / 808
    params.push_back(std::make_unique<juce::AudioParameterBool>("sub_enabled", "Sub Enabled", true));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("sub_waveform", "Sub Waveform", 
        juce::StringArray{"Pure Sine", "Warm Triangle", "Tube Saturated", "Drill Distort Saw", "Punch Transient"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("sub_octave", "Sub Octave", -2, 1, -1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sub_punch_amount", "Sub Punch Amt", 0.0f, 1.0f, 0.6f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sub_punch_decay", "Sub Punch Decay", 5.0f, 200.0f, 40.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sub_drive", "Sub Drive", 0.0f, 1.0f, 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sub_gain", "Sub Gain", 0.0f, 1.0f, 0.85f));

    // Layer 2: Synth / Leads / Brass / Keys
    params.push_back(std::make_unique<juce::AudioParameterBool>("synth_enabled", "Synth Enabled", true));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("synth_shape", "Synth Shape", 
        juce::StringArray{"Saw", "Square", "Triangle", "Acoustic Grand", "Vintage Rhodes", "FM Bell", "Pluck Guitar", "Vocal Formant", "Supersaw Lead", "Drawbar Organ", "Acid Reso Sync"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterInt>("synth_unison", "Synth Unison", 1, 4, 1));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("synth_detune", "Synth Detune", 0.0f, 1.0f, 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("synth_spread", "Synth Spread", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterInt>("synth_octave", "Synth Octave", -2, 2, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("synth_gain", "Synth Gain", 0.0f, 1.0f, 0.75f));

    // Amp ADSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>("amp_attack", "Amp Attack", juce::NormalisableRange<float>(0.1f, 5000.0f, 0.1f, 0.25f), 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("amp_decay", "Amp Decay", juce::NormalisableRange<float>(1.0f, 5000.0f, 0.1f, 0.25f), 300.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("amp_sustain", "Amp Sustain", 0.0f, 1.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("amp_release", "Amp Release", juce::NormalisableRange<float>(1.0f, 5000.0f, 0.1f, 0.25f), 200.0f));

    // Filter Section
    params.push_back(std::make_unique<juce::AudioParameterChoice>("filter_type", "Filter Type", juce::StringArray{"Moog 24dB LP", "SVF 12dB LP", "HP 12dB", "BP 12dB"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter_cutoff", "Filter Cutoff", juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.25f), 12000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter_resonance", "Filter Resonance", 0.1f, 8.0f, 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filter_env_amount", "Filter Env Amt", -1.0f, 1.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filt_attack", "Filt Attack", juce::NormalisableRange<float>(0.1f, 5000.0f, 0.1f, 0.25f), 5.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filt_decay", "Filt Decay", juce::NormalisableRange<float>(1.0f, 5000.0f, 0.1f, 0.25f), 300.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filt_sustain", "Filt Sustain", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("filt_release", "Filt Release", juce::NormalisableRange<float>(1.0f, 5000.0f, 0.1f, 0.25f), 200.0f));

    // FX Rack
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fx_drive_amount", "FX Drive", 0.0f, 1.0f, 0.3f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("fx_drive_type", "Drive Type", juce::StringArray{"Soft Clip", "Tube Warmth", "Hard Clip", "Bitcrush"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("fx_delay_time", "Delay Time", juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.35f), 250.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fx_delay_feedback", "Delay Feedback", 0.0f, 0.9f, 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fx_delay_mix", "Delay Mix", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("fx_reverb_size", "Reverb Size", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fx_reverb_decay", "Reverb Decay", 0.1f, 10.0f, 2.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fx_reverb_damp", "Reverb Damp", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("fx_reverb_mix", "Reverb Mix", 0.0f, 1.0f, 0.0f));

    return { params.begin(), params.end() };
}

void Plugged1AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    voiceManager.prepare(sampleRate, samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    saturator.prepare(spec);
    masterFilter.prepare(spec);
    studioDelay.prepare(spec);
    studioReverb.prepare(spec);

    masterGain.prepare(spec);
    masterGain.setRampDurationSeconds(0.02);

    masterLimiter.prepare(spec);
    masterLimiter.setThreshold(0.0f);
    masterLimiter.setRelease(100.0f);
}

void Plugged1AudioProcessor::releaseResources()
{
    voiceManager.reset();
}

bool Plugged1AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void Plugged1AudioProcessor::updateDSPFromParameters()
{
    VoiceParams vp;

    vp.macroPunch = *apvts.getRawParameterValue("macro_punch");
    vp.macroDirt  = *apvts.getRawParameterValue("macro_dirt");
    vp.macroSpace = *apvts.getRawParameterValue("macro_space");
    vp.macroAir   = *apvts.getRawParameterValue("macro_air");

    vp.voiceMode   = static_cast<int>(*apvts.getRawParameterValue("voice_mode"));
    vp.glideTimeMs = *apvts.getRawParameterValue("glide_time");

    vp.subEnabled       = *apvts.getRawParameterValue("sub_enabled") > 0.5f;
    vp.subWaveform      = static_cast<int>(*apvts.getRawParameterValue("sub_waveform"));
    vp.subOctave        = static_cast<int>(*apvts.getRawParameterValue("sub_octave"));
    vp.subPunchAmount   = *apvts.getRawParameterValue("sub_punch_amount");
    vp.subPunchDecayMs  = *apvts.getRawParameterValue("sub_punch_decay");
    vp.subDrive         = *apvts.getRawParameterValue("sub_drive");
    vp.subGain          = *apvts.getRawParameterValue("sub_gain");

    vp.synthEnabled = *apvts.getRawParameterValue("synth_enabled") > 0.5f;
    vp.synthShape   = static_cast<int>(*apvts.getRawParameterValue("synth_shape"));
    vp.synthUnison  = static_cast<int>(*apvts.getRawParameterValue("synth_unison"));
    vp.synthDetune  = *apvts.getRawParameterValue("synth_detune");
    vp.synthSpread  = *apvts.getRawParameterValue("synth_spread");
    vp.synthOctave  = static_cast<int>(*apvts.getRawParameterValue("synth_octave"));
    vp.synthGain    = *apvts.getRawParameterValue("synth_gain");

    vp.ampAttackMs  = *apvts.getRawParameterValue("amp_attack");
    vp.ampDecayMs   = *apvts.getRawParameterValue("amp_decay");
    vp.ampSustain   = *apvts.getRawParameterValue("amp_sustain");
    vp.ampReleaseMs = *apvts.getRawParameterValue("amp_release");

    vp.filtAttackMs  = *apvts.getRawParameterValue("filt_attack");
    vp.filtDecayMs   = *apvts.getRawParameterValue("filt_decay");
    vp.filtSustain   = *apvts.getRawParameterValue("filt_sustain");
    vp.filtReleaseMs = *apvts.getRawParameterValue("filt_release");

    vp.filterType = static_cast<int>(*apvts.getRawParameterValue("filter_type"));
    vp.cutoffHz   = *apvts.getRawParameterValue("filter_cutoff");
    vp.resonance  = *apvts.getRawParameterValue("filter_resonance");
    vp.envAmount  = *apvts.getRawParameterValue("filter_env_amount");

    voiceManager.updateParams(vp);

    // FX Updates
    float driveAmount = *apvts.getRawParameterValue("fx_drive_amount") + vp.macroDirt * 0.4f;
    saturator.setDrive(std::clamp(driveAmount, 0.0f, 1.0f));
    int driveType = static_cast<int>(*apvts.getRawParameterValue("fx_drive_type"));
    saturator.setDriveType(static_cast<StereoSaturator::DriveType>(driveType));

    float delayTime = *apvts.getRawParameterValue("fx_delay_time");
    float delayFb   = *apvts.getRawParameterValue("fx_delay_feedback");
    float delayMix  = *apvts.getRawParameterValue("fx_delay_mix") + vp.macroSpace * 0.35f;
    studioDelay.setParameters(delayTime, delayFb, std::clamp(delayMix, 0.0f, 1.0f));

    float revSize = *apvts.getRawParameterValue("fx_reverb_size");
    float revDamp = *apvts.getRawParameterValue("fx_reverb_damp");
    float revMix  = *apvts.getRawParameterValue("fx_reverb_mix") + vp.macroSpace * 0.4f;
    studioReverb.setParameters(revSize, revDamp, std::clamp(revMix, 0.0f, 1.0f));

    float gainDb = *apvts.getRawParameterValue("master_gain");
    masterGain.setGainDecibels(gainDb);
}

void Plugged1AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // 1. Process MIDI from host and GUI Virtual Piano Keyboard
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    updateDSPFromParameters();

    // 2. Render Synthesizer / Sampler / 808 Voices
    voiceManager.process(buffer, midiMessages);

    // 2. Process FX Rack
    saturator.process(buffer);
    studioDelay.process(buffer);
    studioReverb.process(buffer);

    // 3. Process Master Gain & Limiter
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    masterGain.process(context);
    masterLimiter.process(context);

    // 4. Update Visualizer FIFO for GUI
    if (buffer.getNumChannels() > 0 && buffer.getNumSamples() > 0)
    {
        const float* left = buffer.getReadPointer(0);
        int numSamples = buffer.getNumSamples();
        int writePos = visualizerWritePos.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            visualizerBuffer[(writePos + i) % visualizerBufferSize] = left[i];
        }

        visualizerWritePos.store((writePos + numSamples) % visualizerBufferSize, std::memory_order_relaxed);
    }
}

void Plugged1AudioProcessor::getVisualizerData(float* dest, int numSamples) const
{
    int writePos = visualizerWritePos.load(std::memory_order_relaxed);
    int readStart = (writePos - numSamples + visualizerBufferSize) % visualizerBufferSize;

    for (int i = 0; i < numSamples; ++i)
    {
        dest[i] = visualizerBuffer[(readStart + i) % visualizerBufferSize];
    }
}

juce::AudioProcessorEditor* Plugged1AudioProcessor::createEditor()
{
    return new Plugged1AudioProcessorEditor(*this);
}

bool Plugged1AudioProcessor::hasEditor() const { return true; }

const juce::String Plugged1AudioProcessor::getName() const { return "Plugged 1"; }
bool Plugged1AudioProcessor::acceptsMidi() const { return true; }
bool Plugged1AudioProcessor::producesMidi() const { return false; }
bool Plugged1AudioProcessor::isMidiEffect() const { return false; }
double Plugged1AudioProcessor::getTailLengthSeconds() const { return 2.0; }

int Plugged1AudioProcessor::getNumPrograms() { return 1; }
int Plugged1AudioProcessor::getCurrentProgram() { return 0; }
void Plugged1AudioProcessor::setCurrentProgram(int) {}
const juce::String Plugged1AudioProcessor::getProgramName(int) { return {}; }
void Plugged1AudioProcessor::changeProgramName(int, const juce::String&) {}

void Plugged1AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void Plugged1AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

} // namespace Plugged1

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Plugged1::Plugged1AudioProcessor();
}

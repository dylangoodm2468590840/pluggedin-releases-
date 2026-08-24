#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

static const char* sRootNoteChoices[] = {
    "C", "C# / Db", "D", "D# / Eb", "E", "F", "F# / Gb", "G", "G# / Ab", "A", "A# / Bb", "B"
};

static const char* sScaleChoices[] = {
    "Minor (m)", "Major (M)", "Chromatic (All)", "Harmonic Minor", "Pentatonic Minor", "Pentatonic Major", "Custom"
};

static const char* sVocalRangeChoices[] = {
    "Low (Male/Bass)", "Mid (Tenor/Alto)", "High (Soprano/Pop)"
};

static const char* sGroupChoices[] = {
    "None", "Group A", "Group B", "Group C", "Group D"
};

juce::AudioProcessorValueTreeState::ParameterLayout PlugTuneAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Master Tune Amount: 0.0% (Natural/Off) to 100.0% (Hard Robot Snap)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("tuneAmount", 1), "Tune Amount",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 100.0f));

    // Formant Shift: -12.0st to +12.0st (default 0st)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("formant", 1), "Formant Shift",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    // Doubler Amount: 0.0 to 100.0% (default 0%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("doubler", 1), "Vocal Doubler",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.5f), 0.0f));

    // Root Key: 0 to 11
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("rootKey", 1), "Root Key",
        juce::StringArray(sRootNoteChoices, 12), 0));

    // Scale Type: 0 to 6
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("scaleType", 1), "Scale Type",
        juce::StringArray(sScaleChoices, 7), 0));

    // Vocal Range: 0 to 2
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("vocalRange", 1), "Vocal Range",
        juce::StringArray(sVocalRangeChoices, 3), 1));

    // Live Mode Toggle
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("liveMode", 1), "Live Rec Mode", true));

    // Session Group: 0=None, 1=A, 2=B, 3=C, 4=D
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("group", 1), "Session Group",
        juce::StringArray(sGroupChoices, 5), 0));

    return { params.begin(), params.end() };
}

PlugTuneAudioProcessor::PlugTuneAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      mValueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
}

PlugTuneAudioProcessor::~PlugTuneAudioProcessor()
{
}

const juce::String PlugTuneAudioProcessor::getName() const
{
    return "PlugTune";
}

bool PlugTuneAudioProcessor::acceptsMidi() const { return true; }
bool PlugTuneAudioProcessor::producesMidi() const { return false; }
bool PlugTuneAudioProcessor::isMidiEffect() const { return false; }
double PlugTuneAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int PlugTuneAudioProcessor::getNumPrograms() { return 1; }
int PlugTuneAudioProcessor::getCurrentProgram() { return 0; }
void PlugTuneAudioProcessor::setCurrentProgram(int) {}
const juce::String PlugTuneAudioProcessor::getProgramName(int) { return {}; }
void PlugTuneAudioProcessor::changeProgramName(int, const juce::String&) {}

bool PlugTuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void PlugTuneAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    int numChannels = std::max(1, getTotalNumOutputChannels());

    bool initLiveMode = true;
    auto* liveModeParam = mValueTreeState.getRawParameterValue("liveMode");
    if (liveModeParam != nullptr)
    {
        initLiveMode = (liveModeParam->load() > 0.5f);
    }
    mCurrentLiveMode.store(initLiveMode);
    mPitchShifter.setLiveMode(initLiveMode);
    mPitchDetector.setLiveMode(initLiveMode);

    mPitchDetector.prepare(sampleRate, samplesPerBlock);
    mPitchShifter.prepare(sampleRate, samplesPerBlock, numChannels);
    mScaleQuantizer.prepare(sampleRate);
    mStereoDoubler.prepare(sampleRate);
    mToneGenerator.prepare(sampleRate);

    setLatencySamples(mPitchShifter.getLatencySamples());
}

void PlugTuneAudioProcessor::releaseResources()
{
    mPitchDetector.reset();
    mPitchShifter.reset();
    mScaleQuantizer.reset();
    mStereoDoubler.reset();
    mToneGenerator.reset();
}

void PlugTuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    if (numSamples == 0) return;

    // Handle MIDI Target Note Override (if sent by host/keyboard)
    for (const auto metadata : midiMessages)
    {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn())
        {
            mScaleQuantizer.setMidiTargetNote(msg.getNoteNumber(), true);
        }
        else if (msg.isNoteOff())
        {
            mScaleQuantizer.setMidiTargetNote(msg.getNoteNumber(), false);
        }
    }

    // Read and update APVTS Parameters
    float tuneAmountVal = mValueTreeState.getRawParameterValue("tuneAmount")->load();
    float formantVal = mValueTreeState.getRawParameterValue("formant")->load();
    float doublerVal = mValueTreeState.getRawParameterValue("doubler")->load();
    int rootIdx = static_cast<int>(mValueTreeState.getRawParameterValue("rootKey")->load());
    int scaleIdx = static_cast<int>(mValueTreeState.getRawParameterValue("scaleType")->load());
    int rangeIdx = static_cast<int>(mValueTreeState.getRawParameterValue("vocalRange")->load());
    bool liveModeVal = (mValueTreeState.getRawParameterValue("liveMode")->load() > 0.5f);

    // Dynamic Live Rec Mode Switching & Host Latency Update
    if (liveModeVal != mCurrentLiveMode.load())
    {
        mCurrentLiveMode.store(liveModeVal);
        mPitchShifter.setLiveMode(liveModeVal);
        mPitchDetector.setLiveMode(liveModeVal);
        setLatencySamples(mPitchShifter.getLatencySamples());
        updateHostDisplay(juce::AudioProcessorListener::ChangeDetails().withLatencyChanged(true));
    }

    mPitchDetector.setVocalRange(static_cast<PlugTuneDSP::VocalRange>(rangeIdx));
    mScaleQuantizer.setRootKey(rootIdx);
    mScaleQuantizer.setScaleType(static_cast<PlugTuneDSP::ScaleType>(scaleIdx));
    mScaleQuantizer.setTuneAmount(tuneAmountVal);

    mStereoDoubler.setAmount(doublerVal * 0.01f);

    // Step 1: Detect Pitch on Primary (Mono/Left) Channel
    const float* monoIn = buffer.getReadPointer(0);
    auto pitchResult = mPitchDetector.processBlock(monoIn, numSamples);

    // Step 2: Quantize Pitch to Active Scale
    float targetMidi = mScaleQuantizer.processQuantization(pitchResult.midiNote, pitchResult.isVoiced, numSamples);

    // Update Telemetry Atomics for GUI
    mLiveInputMidi.store(pitchResult.midiNote);
    mLiveTargetMidi.store(targetMidi);
    mLiveClarity.store(pitchResult.clarity);
    mLiveIsVoiced.store(pitchResult.isVoiced);
    if (pitchResult.isVoiced)
    {
        mLiveDetectedNote.store(((static_cast<int>(std::round(pitchResult.midiNote)) % 12) + 12) % 12);
    }

    // Step 3: Compute Pitch Shift Delta in Semitones
    float shiftSemitones = 0.0f;
    if (pitchResult.isVoiced && pitchResult.midiNote > 0.0f)
    {
        shiftSemitones = mScaleQuantizer.getCorrectionDeltaSemitones();
    }

    // Step 4: Apply Pristine Signalsmith Spectral Pitch & Formant Processing
    mPitchShifter.setPitchAndFormant(shiftSemitones, formantVal, pitchResult.frequencyHz, pitchResult.isVoiced, tuneAmountVal);
    mPitchShifter.process(buffer);

    // Step 5: Apply Stereo Doubler
    mStereoDoubler.process(buffer);

    // Step 6: Add Reference Audition Tone (if active)
    mToneGenerator.processAdding(buffer);
}

bool PlugTuneAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PlugTuneAudioProcessor::createEditor()
{
    return new PlugTuneAudioProcessorEditor(*this);
}

void PlugTuneAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = mValueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PlugTuneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(mValueTreeState.state.getType()))
        {
            mValueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlugTuneAudioProcessor();
}

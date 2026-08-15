#include "PluginProcessor.h"
#include "PluginEditor.h"

UndergroundAudioProcessor::UndergroundAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // Retrieve raw atomic parameter pointers
    degenerateParam   = apvts.getRawParameterValue(ParameterIDs::DEGENERATE);
    presetModeParam   = apvts.getRawParameterValue(ParameterIDs::PRESET_MODE);
    inputGainParam    = apvts.getRawParameterValue(ParameterIDs::INPUT_GAIN);
    outputGainParam   = apvts.getRawParameterValue(ParameterIDs::OUTPUT_GAIN);
    mixGlobalParam    = apvts.getRawParameterValue(ParameterIDs::MIX_GLOBAL);

    macroDepthParam   = apvts.getRawParameterValue(ParameterIDs::MACRO_DEPTH);
    macroDarkParam    = apvts.getRawParameterValue(ParameterIDs::MACRO_DARK);
    macroMotionParam  = apvts.getRawParameterValue(ParameterIDs::MACRO_MOTION);
    macroChaosParam   = apvts.getRawParameterValue(ParameterIDs::MACRO_CHAOS);
    macroAgeParam     = apvts.getRawParameterValue(ParameterIDs::MACRO_AGE);
    macroGhostParam   = apvts.getRawParameterValue(ParameterIDs::MACRO_GHOST);
    macroToneParam    = apvts.getRawParameterValue(ParameterIDs::MACRO_TONE);

    shadowEnableParam = apvts.getRawParameterValue(ParameterIDs::SHADOW_ENABLE);
    shadowMixParam    = apvts.getRawParameterValue(ParameterIDs::SHADOW_MIX);
    shadowPitchParam  = apvts.getRawParameterValue(ParameterIDs::SHADOW_PITCH);
    shadowFormantParam= apvts.getRawParameterValue(ParameterIDs::SHADOW_FORMANT);
    shadowDarkParam   = apvts.getRawParameterValue(ParameterIDs::SHADOW_DARKNESS);
    shadowDriveParam  = apvts.getRawParameterValue(ParameterIDs::SHADOW_DRIVE);

    crushAmountParam  = apvts.getRawParameterValue(ParameterIDs::CRUSH_AMOUNT);
    crushCharParam    = apvts.getRawParameterValue(ParameterIDs::CRUSH_CHARACTER);
    crushToneParam    = apvts.getRawParameterValue(ParameterIDs::CRUSH_TONE);
    crushMixParam     = apvts.getRawParameterValue(ParameterIDs::CRUSH_MIX);

    widthAmountParam  = apvts.getRawParameterValue(ParameterIDs::WIDTH_AMOUNT);
    spaceReverbParam  = apvts.getRawParameterValue(ParameterIDs::SPACE_REVERB);
    spaceDelayParam   = apvts.getRawParameterValue(ParameterIDs::SPACE_DELAY);
    deviceTypeParam   = apvts.getRawParameterValue(ParameterIDs::DEVICE_TYPE);
}

UndergroundAudioProcessor::~UndergroundAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout UndergroundAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Global / Signature Controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::DEGENERATE, 1 },
        "DEGENERATE",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        0.65f));

    juce::StringArray presetChoices { 
        "DEGENERATE RAGE", "SLUDGE PLUGG", "CYBER AD-LIB", "GLITCH MONSTER", "UNDERGROUND TUBE",
        "DEMON BELOW", "UNDERWORLD", "POSSESSED", "DEEP SHADOW", "HELL LAYER",
        "WIDE LEAD", "FLOATING HOOK", "HEADPHONE WIDE", "CYBER DOUBLE", "STEREO AURA",
        "DEEP CHEST", "GOBLIN", "TINY VOICE", "FORMANT SHIFTER", "OCTAVE STACK",
        "CATHEDRAL", "SLAP ROOM", "FLOATING ECHO", "DARK VOID", "WASHED VOCAL",
        "TAPE WARMTH", "CELL PHONE", "FRIED MIC", "BROKEN INTERCOM", "RADIO STATIC",
        "CYBER CHORUS", "UNSTABLE PITCH", "UNDERWATER", "SPINNING VOCAL", "PULSING GATE",
        "DARK SLAP", "THROW ECHO", "PING PONG SPACE", "FILTERED TAPE DELAY", "PITCH ECHO",
        "MODERN RAP LEAD", "RAGE VOCAL", "DARK PLUGG LEAD", "INTIMATE TRAP", "RAW UNDERGROUND",
        "MONSTER ADLIB", "TELEPHONE SHOUT", "DISTANCE ADLIB", "GHOST LAYER", "SPECTRAL GHOST",
        "CUSTOM"
    };

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::PRESET_MODE, 1 },
        "Preset Mode",
        presetChoices,
        0));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::INPUT_GAIN, 1 },
        "Input Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float val, int) { return juce::String(val, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::OUTPUT_GAIN, 1 },
        "Output Gain",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f),
        0.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float val, int) { return juce::String(val, 1) + " dB"; }));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::MIX_GLOBAL, 1 },
        "Mix Global",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        1.0f,
        juce::String(),
        juce::AudioProcessorParameter::genericParameter,
        [](float val, int) { return juce::String(juce::roundToInt(val * 100.0f)) + " %"; }));

    // 5 Hardware Module Bypass Enable Parameters
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParameterIDs::MODULE_SUB_ENABLE, 1 }, "Sub Bass Module Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParameterIDs::MODULE_GRIT_ENABLE, 1 }, "Grit Module Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParameterIDs::MODULE_MOD_ENABLE, 1 }, "Modulation Module Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParameterIDs::MODULE_DELAY_ENABLE, 1 }, "Delay Module Enable", true));
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParameterIDs::MODULE_REVERB_ENABLE, 1 }, "Reverb Module Enable", true));

    // 5-Band Visual & Audio EQ Controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::EQ_LOW_CUT, 1 },
        "EQ Low Cut",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.5f),
        30.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::EQ_LOW_GAIN, 1 },
        "EQ Low Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::EQ_MID_GAIN, 1 },
        "EQ Mid Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::EQ_HIGH_GAIN, 1 },
        "EQ High Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::EQ_HIGH_CUT, 1 },
        "EQ High Cut",
        juce::NormalisableRange<float>(500.0f, 20000.0f, 10.0f, 0.5f),
        18000.0f));

    // Q Bandwidth & Slope Parameters (Pro-Q 3 / Slate Infinity Standard)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::EQ_LOW_Q, 1 },
        "EQ Low Q",
        juce::NormalisableRange<float>(0.10f, 50.0f, 0.05f, 0.35f),
        0.707f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::EQ_MID_Q, 1 },
        "EQ Mid Q",
        juce::NormalisableRange<float>(0.10f, 50.0f, 0.05f, 0.35f),
        0.707f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::EQ_HIGH_Q, 1 },
        "EQ High Q",
        juce::NormalisableRange<float>(0.10f, 50.0f, 0.05f, 0.35f),
        0.707f));

    juce::StringArray slopeChoices { "6 dB/oct", "12 dB/oct", "24 dB/oct", "48 dB/oct", "96 dB/oct" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::EQ_LOW_CUT_SLOPE, 1 },
        "EQ Low Cut Slope",
        slopeChoices,
        1));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::EQ_HIGH_CUT_SLOPE, 1 },
        "EQ High Cut Slope",
        slopeChoices,
        1));

    // Macro Controls
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::MACRO_DEPTH, 1 }, "Macro Depth", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::MACRO_DARK, 1 }, "Macro Dark", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::MACRO_MOTION, 1 }, "Macro Motion", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::MACRO_CHAOS, 1 }, "Macro Chaos", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::MACRO_AGE, 1 }, "Macro Age", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::MACRO_GHOST, 1 }, "Macro Ghost", 0.0f, 1.0f, 0.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::MACRO_TONE, 1 }, "Macro Tone", 0.0f, 1.0f, 0.5f));

    // Shadow Module Parameters
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { ParameterIDs::SHADOW_ENABLE, 1 }, "Shadow Enable", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::SHADOW_MIX, 1 }, "Shadow Mix", 0.0f, 1.0f, 0.0f));

    juce::StringArray pitchIntervalChoices { "OCTAVE DOWN (-12)", "FIFTH DOWN (-7)", "FOURTH DOWN (-5)", "TWO OCTAVES (-24)" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { ParameterIDs::SHADOW_PITCH, 1 }, "Shadow Pitch", pitchIntervalChoices, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::SHADOW_FORMANT, 1 }, "Shadow Formant", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::SHADOW_DARKNESS, 1 }, "Shadow Darkness", 0.0f, 1.0f, 0.5f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::SHADOW_DRIVE, 1 }, "Shadow Drive", 0.0f, 1.0f, 0.2f));

    // CRUSH Module Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::CRUSH_AMOUNT, 1 }, "Crush Amount", 0.0f, 1.0f, 0.60f));
    juce::StringArray crushCharChoices { "SOFT CLIP", "BITCRUSHER", "OVERDRIVE", "PARALLEL FUZZ" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { ParameterIDs::CRUSH_CHARACTER, 1 }, "Crush Character", crushCharChoices, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::CRUSH_TONE, 1 }, "Crush Tone", 0.0f, 1.0f, 0.65f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::CRUSH_MIX, 1 }, "Crush Mix", 0.0f, 1.0f, 1.0f));

    // WIDTH Module Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::WIDTH_AMOUNT, 1 }, "Width Amount", 0.0f, 1.0f, 0.50f));

    // SPACE Module Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::SPACE_REVERB, 1 }, "Space Reverb", 0.0f, 1.0f, 0.30f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::SPACE_DELAY, 1 }, "Space Delay", 0.0f, 1.0f, 0.25f));

    // DEVICE Module Parameters
    juce::StringArray deviceChoices { "OFF", "CELL PHONE", "WEBCAM", "EARBUDS", "LAPTOP", "VOICE MEMO" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { ParameterIDs::DEVICE_TYPE, 1 }, "Device Type", deviceChoices, 0));

    return layout;
}

bool UndergroundAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void UndergroundAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    shadowProcessor.prepare(spec);
    crushProcessor.prepare(spec);
    widthProcessor.prepare(spec);
    spaceProcessor.prepare(spec);
    deviceProcessor.prepare(spec);
    signalChain.prepare(spec);

    // Trigger non-blocking cloud update check
    autoUpdater.checkForUpdatesAsync("2.1.0");
}

void UndergroundAudioProcessor::releaseResources()
{
    shadowProcessor.reset();
    crushProcessor.reset();
    widthProcessor.reset();
    spaceProcessor.reset();
    deviceProcessor.reset();
    signalChain.reset();
}

void UndergroundAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    float degenVal = degenerateParam ? degenerateParam->load() : 0.65f;

    // Macro params
    float mDepth  = macroDepthParam ? macroDepthParam->load() : 0.0f;
    float mDark   = macroDarkParam ? macroDarkParam->load() : 0.0f;
    float mMotion = macroMotionParam ? macroMotionParam->load() : 0.0f;
    float mChaos  = macroChaosParam ? macroChaosParam->load() : 0.0f;
    float mAge    = macroAgeParam ? macroAgeParam->load() : 0.0f;
    float mGhost  = macroGhostParam ? macroGhostParam->load() : 0.0f;
    float mTone   = macroToneParam ? macroToneParam->load() : 0.5f;

    // Update Shadow Settings
    shadowProcessor.setEnabled(shadowEnableParam ? (shadowEnableParam->load() > 0.5f) : true);
    shadowProcessor.setMix(shadowMixParam ? shadowMixParam->load() : 0.0f);
    int pIdx = shadowPitchParam ? juce::roundToInt(shadowPitchParam->load()) : 0;
    shadowProcessor.setPitchInterval(static_cast<ShadowProcessor::PitchInterval>(pIdx));
    shadowProcessor.setFormantShift(shadowFormantParam ? shadowFormantParam->load() : 0.5f);
    shadowProcessor.setDarkness(shadowDarkParam ? shadowDarkParam->load() : 0.5f);
    shadowProcessor.setDrive(shadowDriveParam ? shadowDriveParam->load() : 0.2f);

    // Update Crush Settings
    int charIdx = crushCharParam ? juce::roundToInt(crushCharParam->load()) : 0;
    crushProcessor.setCharacter(static_cast<CrushProcessor::Character>(charIdx));
    crushProcessor.setAmount(crushAmountParam ? crushAmountParam->load() : 0.6f);
    crushProcessor.setTone(crushToneParam ? crushToneParam->load() : 0.65f);
    crushProcessor.setMix(crushMixParam ? crushMixParam->load() : 1.0f);

    // Update Width Settings
    widthProcessor.setAmount(widthAmountParam ? widthAmountParam->load() : 0.5f);
    widthProcessor.setMix(1.0f);

    // Update Space Settings
    spaceProcessor.setReverbMix(spaceReverbParam ? spaceReverbParam->load() : 0.3f);
    spaceProcessor.setDelayTime(spaceDelayParam ? spaceDelayParam->load() : 0.25f);

    // Update Device Settings
    int devIdx = deviceTypeParam ? juce::roundToInt(deviceTypeParam->load()) : 0;
    deviceProcessor.setDeviceType(static_cast<DeviceProcessor::DeviceType>(devIdx));

    // Run Master Signal Pipeline
    float inGainDb  = inputGainParam ? inputGainParam->load() : 0.0f;
    float outGainDb = outputGainParam ? outputGainParam->load() : 0.0f;
    float mixGlob   = mixGlobalParam ? mixGlobalParam->load() : 1.0f;

    float subEn     = apvts.getRawParameterValue(ParameterIDs::MODULE_SUB_ENABLE)->load();
    float gritEn    = apvts.getRawParameterValue(ParameterIDs::MODULE_GRIT_ENABLE)->load();
    float modEn     = apvts.getRawParameterValue(ParameterIDs::MODULE_MOD_ENABLE)->load();
    float delayEn   = apvts.getRawParameterValue(ParameterIDs::MODULE_DELAY_ENABLE)->load();
    float verbEn    = apvts.getRawParameterValue(ParameterIDs::MODULE_REVERB_ENABLE)->load();

    signalChain.process(buffer, inGainDb, outGainDb, mixGlob, degenVal, 
                        subEn, gritEn, modEn, delayEn, verbEn,
                        apvts.getRawParameterValue(ParameterIDs::EQ_LOW_CUT)->load(),
                        apvts.getRawParameterValue(ParameterIDs::EQ_LOW_GAIN)->load(),
                        apvts.getRawParameterValue(ParameterIDs::EQ_MID_GAIN)->load(),
                        apvts.getRawParameterValue(ParameterIDs::EQ_HIGH_GAIN)->load(),
                        apvts.getRawParameterValue(ParameterIDs::EQ_HIGH_CUT)->load(),
                        apvts.getRawParameterValue(ParameterIDs::EQ_LOW_Q)->load(),
                        apvts.getRawParameterValue(ParameterIDs::EQ_MID_Q)->load(),
                        apvts.getRawParameterValue(ParameterIDs::EQ_HIGH_Q)->load(),
                        juce::roundToInt(apvts.getRawParameterValue(ParameterIDs::EQ_LOW_CUT_SLOPE)->load()),
                        juce::roundToInt(apvts.getRawParameterValue(ParameterIDs::EQ_HIGH_CUT_SLOPE)->load()),
                        mDepth, mDark, mMotion, mChaos, mAge, mGhost, mTone,
                        shadowProcessor, crushProcessor, widthProcessor, spaceProcessor, deviceProcessor);
}

juce::AudioProcessorEditor* UndergroundAudioProcessor::createEditor()
{
    return new UndergroundAudioProcessorEditor(*this);
}

void UndergroundAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void UndergroundAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UndergroundAudioProcessor();
}

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
    crushPunishParam  = apvts.getRawParameterValue(ParameterIDs::CRUSH_PUNISH);


    widthAmountParam  = apvts.getRawParameterValue(ParameterIDs::WIDTH_AMOUNT);
    modRateParam      = apvts.getRawParameterValue(ParameterIDs::MOD_RATE);
    modDepthParam     = apvts.getRawParameterValue(ParameterIDs::MOD_DEPTH);

    spaceReverbParam  = apvts.getRawParameterValue(ParameterIDs::SPACE_REVERB);
    reverbDecayParam  = apvts.getRawParameterValue(ParameterIDs::REVERB_DECAY);
    reverbMixParam    = apvts.getRawParameterValue(ParameterIDs::REVERB_MIX);
    spaceDelayParam   = apvts.getRawParameterValue(ParameterIDs::SPACE_DELAY);
    delayFbParam      = apvts.getRawParameterValue(ParameterIDs::DELAY_FEEDBACK);
    delayMixParam     = apvts.getRawParameterValue(ParameterIDs::DELAY_MIX);
    deviceTypeParam   = apvts.getRawParameterValue(ParameterIDs::DEVICE_TYPE);

    compSqueezeParam  = apvts.getRawParameterValue(ParameterIDs::COMP_SQUEEZE);
    compCharParam     = apvts.getRawParameterValue(ParameterIDs::COMP_CHARACTER);
    deEssAmountParam  = apvts.getRawParameterValue(ParameterIDs::DEESS_AMOUNT);
    deEssFreqParam    = apvts.getRawParameterValue(ParameterIDs::DEESS_FREQ);

    airMidParam       = apvts.getRawParameterValue(ParameterIDs::AIR_MID);
    airTopParam       = apvts.getRawParameterValue(ParameterIDs::AIR_TOP);
    spaceDuckingParam = apvts.getRawParameterValue(ParameterIDs::SPACE_DUCKING);
    masterBypassParam = apvts.getRawParameterValue(ParameterIDs::MASTER_BYPASS);
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
        "01. POLISHED / CRISP",
        "02. DARK / UNDERGROUND",
        "03. DEMON / DEEP",
        "04. WIDE / FLOATING",
        "05. DESTROYED / CRUSHED",
        "06. TELEPHONE / DEVICE",
        "07. FUTURISTIC / ALIEN",
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

    // CRUSH Module Parameters (5-Circuit Analog Suite)
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::CRUSH_AMOUNT, 1 }, "Crush Amount", 0.0f, 1.0f, 0.60f));
    juce::StringArray crushCharChoices { "12AX7 TUBE", "EL34 PENTODE", "AMPEX TAPE", "GERMANIUM", "CYBER FUZZ" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { ParameterIDs::CRUSH_CHARACTER, 1 }, "Crush Character", crushCharChoices, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::CRUSH_TONE, 1 }, "Crush Tone", 0.0f, 1.0f, 0.65f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::CRUSH_MIX, 1 }, "Crush Mix", 0.0f, 1.0f, 1.0f));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { ParameterIDs::CRUSH_PUNISH, 1 }, "PUNISH Mode (+20dB)", false));


    // WIDTH & MODULATION Module Parameters
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::WIDTH_AMOUNT, 1 }, "Chorus Amount", 0.0f, 1.0f, 0.50f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::MOD_RATE, 1 }, "Mod Rate", 0.0f, 1.0f, 0.35f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::MOD_DEPTH, 1 }, "Mod Depth", 0.0f, 1.0f, 0.50f));

    // SPACE & TIME Module Parameters (Delay & Reverb)
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::SPACE_DELAY, 1 }, "Delay Time", 0.01f, 1.5f, 0.25f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::DELAY_FEEDBACK, 1 }, "Delay Feedback", 0.0f, 0.95f, 0.35f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::DELAY_MIX, 1 }, "Delay Mix", 0.0f, 1.0f, 0.25f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::SPACE_REVERB, 1 }, "Reverb Size", 0.0f, 1.0f, 0.30f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::REVERB_DECAY, 1 }, "Reverb Decay", 0.0f, 1.0f, 0.50f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::REVERB_MIX, 1 }, "Reverb Mix", 0.0f, 1.0f, 0.30f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { ParameterIDs::SPACE_DUCKING, 1 }, "Space Ducking", 0.0f, 1.0f, 0.50f));

    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { ParameterIDs::MASTER_BYPASS, 1 }, "Master Bypass", false));

    // DEVICE Module Parameters
    juce::StringArray deviceChoices { "OFF", "CELL PHONE", "WEBCAM", "EARBUDS", "LAPTOP", "VOICE MEMO" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { ParameterIDs::DEVICE_TYPE, 1 }, "Device Type", deviceChoices, 0));

    // DYNAMICS & TAMING CORE (Vocal Compressor & De-Esser)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::COMP_SQUEEZE, 1 },
        "Vocal Squeeze",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    juce::StringArray compCharChoices { "MODERN FET", "VINTAGE OPTO", "PUNCHY BLEND" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParameterIDs::COMP_CHARACTER, 1 },
        "Comp Character",
        compCharChoices,
        2));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::DEESS_AMOUNT, 1 },
        "De-Ess Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::DEESS_FREQ, 1 },
        "De-Ess Frequency",
        juce::NormalisableRange<float>(4000.0f, 10000.0f, 10.0f, 0.5f),
        6500.0f));

    // PSYCHOACOUSTIC FRESH AIR EXCITER (Mid-Air & Top-Air Sheen)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::AIR_MID, 1 },
        "Mid-Air Presence",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParameterIDs::AIR_TOP, 1 },
        "Top-Air Sheen",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
        0.0f));

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

void UndergroundAudioProcessor::toggleABState()
{
    // Save current active state into XML memory snapshot
    auto currentXml = apvts.copyState().createXml();
    if (activeStateSlot == 0)
        stateSlotA = std::move(currentXml);
    else
        stateSlotB = std::move(currentXml);

    // Toggle slot: 0 -> 1 (B) or 1 -> 0 (A)
    activeStateSlot = 1 - activeStateSlot;

    // Restore state from newly active slot if available
    auto& targetXml = (activeStateSlot == 0) ? stateSlotA : stateSlotB;
    if (targetXml != nullptr)
    {
        apvts.replaceState(juce::ValueTree::fromXml(*targetXml));
    }
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
    vocalCompressor.prepare(spec);
    deEsserProcessor.prepare(spec);
    airExciterProcessor.prepare(spec);
    signalChain.prepare(spec);

    // Trigger non-blocking cloud update check
    autoUpdater.checkForUpdatesAsync("3.0.0");
}


void UndergroundAudioProcessor::releaseResources()
{
    shadowProcessor.reset();
    crushProcessor.reset();
    widthProcessor.reset();
    spaceProcessor.reset();
    deviceProcessor.reset();
    vocalCompressor.reset();
    deEsserProcessor.reset();
    airExciterProcessor.reset();
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

    // Dynamics & Taming params
    float compSq   = compSqueezeParam ? compSqueezeParam->load() : 0.0f;
    int compCh     = compCharParam ? juce::roundToInt(compCharParam->load()) : 2;
    vocalCompressor.setCharacter(compCh);

    float deEssAmt = deEssAmountParam ? deEssAmountParam->load() : 0.0f;
    float deEssFrq = deEssFreqParam ? deEssFreqParam->load() : 6500.0f;

    // Fresh Air & Space Ducking params
    float aMid     = airMidParam ? airMidParam->load() : 0.0f;
    float aTop     = airTopParam ? airTopParam->load() : 0.0f;
    float sDuck    = spaceDuckingParam ? spaceDuckingParam->load() : 0.50f;

    // Update Shadow Settings
    shadowProcessor.setEnabled(shadowEnableParam ? (shadowEnableParam->load() > 0.5f) : true);
    shadowProcessor.setMix(shadowMixParam ? shadowMixParam->load() : 0.0f);
    int pIdx = shadowPitchParam ? juce::roundToInt(shadowPitchParam->load()) : 0;
    shadowProcessor.setPitchInterval(static_cast<ShadowProcessor::PitchInterval>(pIdx));
    shadowProcessor.setFormantShift(shadowFormantParam ? shadowFormantParam->load() : 0.5f);
    shadowProcessor.setDarkness(shadowDarkParam ? shadowDarkParam->load() : 0.5f);
    shadowProcessor.setDrive(shadowDriveParam ? shadowDriveParam->load() : 0.2f);

    // Update Crush Settings (5 Analog Topologies & PUNISH Mode)
    int charIdx = crushCharParam ? juce::roundToInt(crushCharParam->load()) : 0;
    crushProcessor.setCharacter(static_cast<CrushProcessor::Character>(charIdx));
    crushProcessor.setAmount(crushAmountParam ? crushAmountParam->load() : 0.6f);
    crushProcessor.setTone(crushToneParam ? crushToneParam->load() : 0.65f);
    crushProcessor.setMix(crushMixParam ? crushMixParam->load() : 1.0f);
    crushProcessor.setPunish(crushPunishParam ? (crushPunishParam->load() > 0.5f) : false);

    // Master Bypass
    if (masterBypassParam && masterBypassParam->load() > 0.5f)
        return;

    // Update Width & Modulation Settings
    widthProcessor.setAmount(widthAmountParam ? widthAmountParam->load() : 0.5f);
    widthProcessor.setRate(modRateParam ? modRateParam->load() : 0.35f);
    widthProcessor.setDepth(modDepthParam ? modDepthParam->load() : 0.50f);
    widthProcessor.setMix(1.0f);

    // Update Space (Delay & Reverb) Settings
    spaceProcessor.setDelayTime(spaceDelayParam ? spaceDelayParam->load() : 0.25f);
    spaceProcessor.setFeedback(delayFbParam ? delayFbParam->load() : 0.35f);
    spaceProcessor.setDelayMix(delayMixParam ? delayMixParam->load() : 0.25f);

    spaceProcessor.setReverbSize(spaceReverbParam ? spaceReverbParam->load() : 0.30f);
    spaceProcessor.setReverbDecay(reverbDecayParam ? reverbDecayParam->load() : 0.50f);
    spaceProcessor.setReverbMix(reverbMixParam ? reverbMixParam->load() : 0.30f);
    spaceProcessor.setDucking(sDuck);

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
                        compSq, deEssAmt, deEssFrq,
                        aMid, aTop, sDuck,
                        shadowProcessor, crushProcessor, widthProcessor, spaceProcessor, deviceProcessor,
                        vocalCompressor, deEsserProcessor, airExciterProcessor);
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

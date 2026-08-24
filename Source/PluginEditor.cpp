#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BuildInfo.h"

UndergroundAudioProcessorEditor::UndergroundAudioProcessorEditor (UndergroundAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel(&customLookAndFeel);

    // Generate high-quality procedural brushed gunmetal texture
    chassisTexture = HardwareMaterials::createBrushedMetalTexture(800, 600);

    // Helper for rotary sliders
    auto setupRotary = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colour(0xffa0b2c6));
        label.setFont(juce::Font(8.5f, juce::Font::bold));
        addAndMakeVisible(label);
    };

    // 0. Top Preset Selector & Navigation Buttons
    juce::StringArray presetChoices { 
        "01. POLISHED / CRISP",
        "02. DARK / UNDERGROUND",
        "03. DEMON / DEEP",
        "04. WIDE / FLOATING",
        "05. DESTROYED / CRUSHED",
        "06. TELEPHONE / DEVICE",
        "07. FUTURISTIC / ALIEN",
        "08. SUBTERRANEAN 808",
        "09. VINTAGE TAPE / MELLOTRON",
        "10. HYPERPOP / WARP",
        "11. TRAVIS SUB-DEMON",
        "12. EVIL DRILL 5TH",
        "13. ABYSS MONSTER",
        "14. CYBORG DRONE",
        "15. CHIPMUNK / ANIME LEAD",
        "16. GHOST HARMONY BED",
        "CUSTOM"
    };

    presetModeBox.addItemList(presetChoices, 1);
    addAndMakeVisible(presetModeBox);

    presetModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::PRESET_MODE, presetModeBox);

    presetModeBox.onChange = [this]() {
        int idx = presetModeBox.getSelectedId() - 1;
        if (idx >= 0 && idx < PresetManager::NUM_REFERENCE_PRESETS)
        {
            PresetManager::applyPreset(audioProcessor.getAPVTS(), static_cast<PresetManager::PresetIndex>(idx));
        }
    };

    addAndMakeVisible(prevPresetButton);
    prevPresetButton.onClick = [this]() {
        int current = presetModeBox.getSelectedId();
        if (current > 1) presetModeBox.setSelectedId(current - 1, juce::sendNotificationSync);
    };

    addAndMakeVisible(nextPresetButton);
    nextPresetButton.onClick = [this]() {
        int current = presetModeBox.getSelectedId();
        if (current < PresetManager::NUM_REFERENCE_PRESETS) presetModeBox.setSelectedId(current + 1, juce::sendNotificationSync);
    };


    addAndMakeVisible(abButton);
    abButton.setButtonText("STATE: A");
    abButton.onClick = [this]() { audioProcessor.toggleABState(); };

    addAndMakeVisible(bypassButton);
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff18181f));
    bypassButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff2244));
    bypassButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    bypassButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffa0a0b0));
    bypassAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::MASTER_BYPASS, bypassButton);

    addAndMakeVisible(oversamplingButton);
    oversamplingButton.setClickingTogglesState(true);
    oversamplingButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff18181f));
    oversamplingButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00e5ff));
    oversamplingButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    oversamplingButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00e5ff));
    oversamplingAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::CRUSH_OVERSAMPLE, oversamplingButton);


    // 1. DEGENERATE Signature Macro (Top Crown Hero)
    addAndMakeVisible(degenerateKnob);
    degenerateAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::DEGENERATE, degenerateKnob);

    // Dynamics & Taming Controls (Flanking DEGENERATE Crown)
    setupRotary(compSqueezeSlider, compSqueezeLabel, "SQUEEZE");
    compSqueezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::COMP_SQUEEZE, compSqueezeSlider);

    setupRotary(deEssAmountSlider, deEssAmountLabel, "DE-ESS");
    deEssAmountAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::DEESS_AMOUNT, deEssAmountSlider);

    // 2. Real-Time Visual EQ Display Window
    visualEQDisplay.setAPVTS(&audioProcessor.getAPVTS());
    addAndMakeVisible(visualEQDisplay);


    // 3. Global Master Trim Controls
    setupRotary(inputGainSlider, inputGainLabel, "INPUT");
    inputGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::INPUT_GAIN, inputGainSlider);

    setupRotary(outputGainSlider, outputGainLabel, "OUTPUT");
    outputGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::OUTPUT_GAIN, outputGainSlider);

    setupRotary(mixGlobalSlider, mixGlobalLabel, "DRY/WET");
    mixGlobalAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::MIX_GLOBAL, mixGlobalSlider);

    // Fresh Air Psychoacoustic Controls
    setupRotary(airMidSlider, airMidLabel, "MID AIR");
    airMidAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::AIR_MID, airMidSlider);

    setupRotary(airTopSlider, airTopLabel, "TOP AIR");
    airTopAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::AIR_TOP, airTopSlider);


    // 4. 15 Module Knobs across 5 Channels (Matching Sketch Photo 1:1)
    // Column 1: DEMON / PITCH ENGINE (Pitch, Formant/Throat, Drive, Mix, Link, Mode)
    setupRotary(demonPitchSlider, demonPitchLabel, "PITCH");
    demonPitchAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::DEMON_PITCH, demonPitchSlider);

    setupRotary(demonFormantSlider, demonFormantLabel, "THROAT");
    demonFormantAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::DEMON_FORMANT, demonFormantSlider);

    setupRotary(demonMixSlider, demonMixLabel, "MIX");
    demonMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::DEMON_MIX, demonMixSlider);

    setupRotary(demonDriveSlider, demonDriveLabel, "DRIVE");
    demonDriveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::DEMON_DRIVE, demonDriveSlider);

    demonLinkButton.setClickingTogglesState(true);
    demonLinkButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff18181f));
    demonLinkButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffffa800));
    demonLinkButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    demonLinkButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffa0a0b0));
    addAndMakeVisible(demonLinkButton);
    demonLinkAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::DEMON_LINK, demonLinkButton);

    demonModeBox.addItemList({ "TRANSPOSE", "ROBOT", "TUNE" }, 1);
    addAndMakeVisible(demonModeBox);
    demonModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::DEMON_MODE, demonModeBox);

    // Column 2: GRIT (Crush Engine & 5-Circuit Saturation)
    setupRotary(gritFuzzSlider, gritFuzzLabel, "DRIVE");
    gritFuzzAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::CRUSH_AMOUNT, gritFuzzSlider);

    setupRotary(gritDustSlider, gritDustLabel, "TONE");
    gritDustAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::CRUSH_TONE, gritDustSlider);

    setupRotary(gritBitSlider, gritBitLabel, "MIX");
    gritBitAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::CRUSH_MIX, gritBitSlider);

    punishButton.setClickingTogglesState(true);
    punishButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff18050a));
    punishButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff0044));
    punishButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    punishButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff5577));
    addAndMakeVisible(punishButton);
    punishAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::CRUSH_PUNISH, punishButton);


    // Column 3: MODULATION
    setupRotary(modChorusSlider, modChorusLabel, "CHORUS");
    modChorusAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::WIDTH_AMOUNT, modChorusSlider);

    setupRotary(modPhaseSlider, modPhaseLabel, "RATE");
    modPhaseAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::MOD_RATE, modPhaseSlider);

    setupRotary(modVibeSlider, modVibeLabel, "DEPTH");
    modVibeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::MOD_DEPTH, modVibeSlider);

    // Column 4: DELAY
    setupRotary(delayTimeSlider, delayTimeLabel, "TIME");
    delayTimeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::SPACE_DELAY, delayTimeSlider);

    setupRotary(delayFbSlider, delayFbLabel, "FEEDBACK");
    delayFbAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::DELAY_FEEDBACK, delayFbSlider);

    setupRotary(delayMixSlider, delayMixLabel, "MIX");
    delayMixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::DELAY_MIX, delayMixSlider);

    // Column 5: REVERB
    setupRotary(verbSizeSlider, verbSizeLabel, "SIZE");
    verbSizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::SPACE_REVERB, verbSizeSlider);

    setupRotary(verbDecaySlider, verbDecayLabel, "DECAY");
    verbDecayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::REVERB_DECAY, verbDecaySlider);

    setupRotary(verbSpaceSlider, verbSpaceLabel, "MIX");
    verbSpaceAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::REVERB_MIX, verbSpaceSlider);

    // 5 Module Bypass Toggle Buttons & APVTS Attachments
    auto setupModuleToggle = [this](juce::ToggleButton& b) {
        b.setButtonText("");
        b.setAlpha(0.0f); // Invisible click hitbox directly over the custom painted Ruby Red LED
        b.onClick = [this] { repaint(); };
        addAndMakeVisible(b);
    };

    setupModuleToggle(subEnableToggle);
    subEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::MODULE_SUB_ENABLE, subEnableToggle);

    setupModuleToggle(gritEnableToggle);
    gritEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::MODULE_GRIT_ENABLE, gritEnableToggle);

    setupModuleToggle(modEnableToggle);
    modEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::MODULE_MOD_ENABLE, modEnableToggle);

    setupModuleToggle(delayEnableToggle);
    delayEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::MODULE_DELAY_ENABLE, delayEnableToggle);

    setupModuleToggle(reverbEnableToggle);
    reverbEnableAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getAPVTS(), ParameterIDs::MODULE_REVERB_ENABLE, reverbEnableToggle);

    // Compact resolution for FL Studio fit (780x560)
    setSize(780, 560);
    startTimerHz(30);
}

UndergroundAudioProcessorEditor::~UndergroundAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void UndergroundAudioProcessorEditor::timerCallback()
{
    auto& chain = audioProcessor.getSignalChain();
    currentInLevel  = currentInLevel * 0.7f  + chain.getInputLevel() * 0.3f;
    currentOutLevel = currentOutLevel * 0.7f + chain.getOutputLevel() * 0.3f;
    currentCompGr   = currentCompGr * 0.7f   + audioProcessor.getVocalCompressor().getGainReductionDb() * 0.3f;
    currentDeEssGr  = currentDeEssGr * 0.7f  + audioProcessor.getDeEsser().getGainReductionDb() * 0.3f;

    for (int b = 0; b < 4; ++b)
    {
        float target = chain.getResonanceProcessor().getBandReductionDb(b);
        currentResReduction[b] = currentResReduction[b] * 0.7f + target * 0.3f;
    }

    int activeSlot = audioProcessor.getActiveStateSlot();
    abButton.setButtonText(activeSlot == 0 ? "STATE: A" : "STATE: B");
    abButton.setColour(juce::TextButton::textColourOffId, activeSlot == 0 ? juce::Colour(0xff00f0ff) : juce::Colour(0xffffaa00));

    // Fetch real FFT spectrum bars from audio DSP chain
    float fftBars[64];
    chain.getSpectrumData(fftBars);


    // Fetch 5-band EQ parameter values from APVTS
    auto& apvts = audioProcessor.getAPVTS();
    float lowCut   = apvts.getRawParameterValue(ParameterIDs::EQ_LOW_CUT)->load();
    float lowGain  = apvts.getRawParameterValue(ParameterIDs::EQ_LOW_GAIN)->load();
    float midGain  = apvts.getRawParameterValue(ParameterIDs::EQ_MID_GAIN)->load();
    float highGain = apvts.getRawParameterValue(ParameterIDs::EQ_HIGH_GAIN)->load();
    float highCut  = apvts.getRawParameterValue(ParameterIDs::EQ_HIGH_CUT)->load();

    float lowQ     = apvts.getRawParameterValue(ParameterIDs::EQ_LOW_Q)->load();
    float midQ     = apvts.getRawParameterValue(ParameterIDs::EQ_MID_Q)->load();
    float highQ    = apvts.getRawParameterValue(ParameterIDs::EQ_HIGH_Q)->load();

    int lowSlope   = juce::roundToInt(apvts.getRawParameterValue(ParameterIDs::EQ_LOW_CUT_SLOPE)->load());
    int highSlope  = juce::roundToInt(apvts.getRawParameterValue(ParameterIDs::EQ_HIGH_CUT_SLOPE)->load());

    visualEQDisplay.updateEQState(fftBars, lowCut, lowGain, midGain, highGain, highCut, lowQ, midQ, highQ, lowSlope, highSlope);

    // Sync all UI dynamic nodes to live C++ audio biquad filtering thread!
    const auto& uiNodes = visualEQDisplay.getDynamicNodes();
    std::vector<AudioDynamicNode> dspNodes;
    for (const auto& node : uiNodes)
    {
        dspNodes.push_back({ node.freqHz, node.gainDb, node.qFactor, node.filterType, node.stereoMode, node.active });
    }
    audioProcessor.getSignalChain().updateDynamicNodes(dspNodes);

    repaint();
}

void UndergroundAudioProcessorEditor::paint (juce::Graphics& g)
{
    // 0. Base Gunmetal Metallic Chassis with Brushed Metal Texture & Top-Lit Studio Lighting
    HardwareMaterials::drawGunmetalChassis(g, getLocalBounds().toFloat().reduced(2.0f), 10.0f, chassisTexture);

    // Heavy Side Rack Mounting Ears with Countersunk Screws (Left & Right)
    HardwareMaterials::drawRackEars(g, getLocalBounds().toFloat(), 24.0f);

    // 1. Top Crown Arch Header (Housing DEGENERATE Macro Knob)
    juce::Path crownPath;
    float crownLeft = 285.0f;
    float crownRight = 495.0f;
    crownPath.startNewSubPath(crownLeft - 35.0f, 60.0f);
    crownPath.lineTo(crownLeft + 20.0f, 10.0f);
    crownPath.lineTo(crownRight - 20.0f, 10.0f);
    crownPath.lineTo(crownRight + 35.0f, 60.0f);
    crownPath.closeSubPath();

    g.setColour(juce::Colour(0xff121722));
    g.fillPath(crownPath);
    g.setColour(juce::Colour(0xff00ff66).withAlpha(0.6f));
    g.strokePath(crownPath, juce::PathStrokeType(1.8f));

    // Screw Bolts on Crown
    HardwareMaterials::drawHexBolt(g, crownLeft - 20.0f, 48.0f);
    HardwareMaterials::drawHexBolt(g, crownRight + 20.0f, 48.0f);

    // Clean Stencil Badge Underneath DEGENERATE Knob
    g.setColour(juce::Colour(0xff00ff66));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("DEGENERATE", (int)crownLeft - 20, 118, 250, 16, juce::Justification::centred, true);

    // Stencil Branding Title (Top-Left)
    g.setColour(juce::Colour(0xff00f0ff));
    g.setFont(juce::Font(22.0f, juce::Font::bold));
    g.drawText("UNDER//GROUND", 28, 12, 220, 22, juce::Justification::left, true);

    g.setColour(juce::Colour(0xff00ff66));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText("PluggedIN AUDIO  |  v" JucePlugin_VersionString " VST3  •  " UNDERGROUND_BUILD_ID, 28, 34, 320, 14, juce::Justification::left, true);

    // SQUEEZE & DE-ESS Mini Gain Reduction (GR) Meters
    auto drawMiniGrMeter = [&](float meterX, float meterY, float grDb) {
        g.setColour(juce::Colour(0xff090c10));
        g.fillRoundedRectangle(meterX, meterY, 5.0f, 44.0f, 2.0f);
        g.setColour(juce::Colour(0xff1b222d));
        g.drawRoundedRectangle(meterX, meterY, 5.0f, 44.0f, 2.0f, 1.0f);

        int activeBars = juce::roundToInt(std::clamp(grDb / 18.0f, 0.0f, 1.0f) * 8.0f);
        for (int b = 0; b < 8; ++b)
        {
            float barY = meterY + 44.0f - (b + 1) * 5.2f;
            bool on = b < activeBars;
            juce::Colour col = (b >= 6) ? juce::Colour(0xffff0055) : (b >= 3 ? juce::Colour(0xffffaa00) : juce::Colour(0xff00f0ff));
            g.setColour(on ? col : col.withAlpha(0.15f));
            g.fillRect(meterX + 1.0f, barY + 1.0f, 3.0f, 3.8f);
        }
    };

    drawMiniGrMeter(262.0f, 28.0f, currentCompGr);
    drawMiniGrMeter(522.0f, 28.0f, currentDeEssGr);

    // 4 Dynamic Resonance Activity Indicators (MUD, NSL, HRSH, FIZZ)
    const char* resLabels[4] = { "MUD", "NSL", "HRSH", "FIZZ" };
    float resStartX = 635.0f;
    for (int b = 0; b < 4; ++b)
    {
        float rx = resStartX + b * 22.0f;
        float ry = 30.0f;
        float redAmt = std::clamp(currentResReduction[b] / 6.0f, 0.0f, 1.0f);
        
        g.setColour(juce::Colour(0xff607286));
        g.setFont(juce::Font(7.0f, juce::Font::bold));
        g.drawText(resLabels[b], rx - 4.0f, ry, 20.0f, 8.0f, juce::Justification::centred);

        juce::Colour dotCol = redAmt > 0.08f ? juce::Colour(0xffffaa00).interpolatedWith(juce::Colour(0xffff0055), redAmt) : juce::Colour(0xff141822);
        g.setColour(dotCol);
        g.fillEllipse(rx + 3.5f, ry + 9.0f, 5.0f, 5.0f);
    }

    // Top-Right Version Stamp
    g.setColour(juce::Colour(0xff7f8b98));
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.drawText(UNDERGROUND_BUILD_ID, getLocalBounds().getWidth() - 140, 14, 120, 16, juce::Justification::right, true);



    // Glowing PluggedIN In-Plugin Cloud Auto-Updater Notification Badge
    if (audioProcessor.getAutoUpdater().isUpdateAvailable())
    {
        auto updateBadge = juce::Rectangle<float>(28, 52, 220, 18);
        g.setColour(juce::Colour(0xffff0055).withAlpha(0.25f));
        g.fillRoundedRectangle(updateBadge, 4.0f);
        g.setColour(juce::Colour(0xffff0055));
        g.drawRoundedRectangle(updateBadge, 4.0f, 1.0f);

        g.setFont(juce::Font(8.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xff00ff66));
        g.drawText("⚡ UPDATE AVAILABLE: v" + audioProcessor.getAutoUpdater().getLatestVersion(), updateBadge, juce::Justification::centred);
    }

    // 2. Upper Main Rack Card (Visual EQ & Flanking Meters)
    auto upperCard = juce::Rectangle<float>(28, 140, 724, 192);
    HardwareMaterials::drawRecessedPanel(g, upperCard, 6.0f, chassisTexture);

    HardwareMaterials::drawHexBolt(g, upperCard.getX() + 8, upperCard.getY() + 8);
    HardwareMaterials::drawHexBolt(g, upperCard.getRight() - 8, upperCard.getY() + 8);
    HardwareMaterials::drawHexBolt(g, upperCard.getX() + 8, upperCard.getBottom() - 8);
    HardwareMaterials::drawHexBolt(g, upperCard.getRight() - 8, upperCard.getBottom() - 8);

    // Render Multi-Segment Vertical Hardware LED Peak Meters (Left IN, Right OUT)
    auto drawLEDMeter = [&](float startX, float level, const juce::String& title) {
        g.setColour(juce::Colour(0xff808e9b));
        g.setFont(juce::Font(8.5f, juce::Font::bold));
        g.drawText(title, (int)startX - 15, (int)upperCard.getY() + 6, 85, 12, juce::Justification::centred);

        const int numSegments = 14;
        float meterH = 145.0f;
        float segmentH = meterH / numSegments;
        int activeSegments = juce::roundToInt(std::clamp(level * 1.2f, 0.0f, 1.0f) * numSegments);

        for (int i = 0; i < numSegments; ++i)
        {
            float segY = upperCard.getY() + 20.0f + (numSegments - 1 - i) * segmentH;
            bool isActive = i < activeSegments;

            juce::Colour segCol = (i >= 11) ? juce::Colour(0xffff0055) :
                                  (i >= 8)  ? juce::Colour(0xffffcc00) :
                                              juce::Colour(0xff00ff66);

            g.setColour(isActive ? segCol : segCol.withAlpha(0.12f));
            g.fillRect(startX, segY + 1.0f, 10.0f, segmentH - 2.0f);
            g.fillRect(startX + 13.0f, segY + 1.0f, 10.0f, segmentH - 2.0f);
        }

        // dB markings scale
        g.setColour(juce::Colour(0xff4a5d73));
        g.setFont(juce::Font(8.0f, juce::Font::bold));
        g.drawText("0",   (int)startX - 16, (int)upperCard.getY() + 20, 14, 10, juce::Justification::right);
        g.drawText("-10", (int)startX - 16, (int)upperCard.getY() + 50, 14, 10, juce::Justification::right);
        g.drawText("-20", (int)startX - 16, (int)upperCard.getY() + 80, 14, 10, juce::Justification::right);
        g.drawText("-40", (int)startX - 16, (int)upperCard.getY() + 120, 14, 10, juce::Justification::right);
        g.drawText("-60", (int)startX - 16, (int)upperCard.getY() + 155, 14, 10, juce::Justification::right);

        g.setColour(juce::Colour(0xff808e9b));
        g.drawText("IN  OUT", (int)startX - 4, (int)upperCard.getBottom() - 14, 34, 10, juce::Justification::centred);
    };

    drawLEDMeter(upperCard.getX() + 32.0f, currentInLevel, "STEREO PEAK METER (L & R)");
    drawLEDMeter(upperCard.getRight() - 60.0f, currentOutLevel, "STEREO PEAK METER (L & R)");

    // 3. Middle 5 Module Columns (5 Equal Metal Channel Strips Matching 1:1 Photo)
    float colW = 140.0f;
    float colGap = 6.0f;
    float startX = 28.0f;
    float colY = 338.0f;
    float colH = 172.0f;

    const char* colTitles[] = { "1. DEMON", "2. GRIT", "3. MODULATION", "4. DELAY", "5. REVERB" };
    const juce::Colour colColours[] = { juce::Colour(0xff00ff66), juce::Colour(0xffff0055), juce::Colour(0xff00f0ff), juce::Colour(0xffffa800), juce::Colour(0xffff00aa) };

    for (int i = 0; i < 5; ++i)
    {
        auto colRect = juce::Rectangle<float>(startX + i * (colW + colGap), colY, colW, colH);
        HardwareMaterials::drawRecessedPanel(g, colRect, 5.0f, chassisTexture);

        HardwareMaterials::drawHexBolt(g, colRect.getX() + 5, colRect.getY() + 5);
        HardwareMaterials::drawHexBolt(g, colRect.getRight() - 5, colRect.getY() + 5);
        HardwareMaterials::drawHexBolt(g, colRect.getX() + 5, colRect.getBottom() - 5);
        HardwareMaterials::drawHexBolt(g, colRect.getRight() - 5, colRect.getBottom() - 5);

        g.setColour(colColours[i]);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(colTitles[i], (int)colRect.getX() + 8, (int)colRect.getY() + 6, (int)colW - 40, 14, juce::Justification::left, true);

        // Ruby Red Indicator LED for Module Status (ON / BYPASS)
        bool isModuleOn = true;
        if (i == 0) isModuleOn = subEnableToggle.getToggleState();
        else if (i == 1) isModuleOn = gritEnableToggle.getToggleState();
        else if (i == 2) isModuleOn = modEnableToggle.getToggleState();
        else if (i == 3) isModuleOn = delayEnableToggle.getToggleState();
        else if (i == 4) isModuleOn = reverbEnableToggle.getToggleState();

        float ledX = colRect.getRight() - 28.0f;
        float ledY = colRect.getY() + 8.0f;

        if (isModuleOn)
        {
            // Glowing Ruby Red LED Halo & Core
            g.setColour(juce::Colour(0xffff0044).withAlpha(0.40f));
            g.fillEllipse(ledX - 3.0f, ledY - 3.0f, 14.0f, 14.0f);

            g.setColour(juce::Colour(0xffff1133));
            g.fillEllipse(ledX, ledY, 8.0f, 8.0f);

            g.setColour(juce::Colours::white);
            g.fillEllipse(ledX + 2.0f, ledY + 2.0f, 2.5f, 2.5f);
        }
        else
        {
            // Darkened Vintage Maroon Glass Lens (OFF)
            g.setColour(juce::Colour(0xff25050a));
            g.fillEllipse(ledX, ledY, 8.0f, 8.0f);
            g.setColour(juce::Colour(0xff120204));
            g.drawEllipse(ledX, ledY, 8.0f, 8.0f, 1.0f);
        }

        // Draw Vertical Mini LED Meter Bar on Right Side of Each Module Column
        float meterX = colRect.getRight() - 18.0f;
        float meterY = colRect.getY() + 24.0f;
        float meterW = 8.0f;
        float meterH = 125.0f;

        g.setColour(juce::Colour(0xff070a0f));
        g.fillRect(meterX, meterY, meterW, meterH);

        for (int m = 0; m < 10; ++m)
        {
            float segY = meterY + (9 - m) * (meterH / 10.0f);
            g.setColour(m > 8 ? juce::Colour(0xffff0055) : (m > 5 ? juce::Colour(0xffffcc00) : colColours[i]));
            g.fillRect(meterX + 1.0f, segY + 1.0f, meterW - 2.0f, (meterH / 10.0f) - 2.0f);
        }

        // Draw 3D Chamfered Metallic Seam Ribs between columns
        if (i < 4)
        {
            float seamX = colRect.getRight() + colGap * 0.5f;
            HardwareMaterials::drawChamferedSeam(g, seamX, colY - 2.0f, colRect.getBottom() + 2.0f);
        }
    }

    // 4. Bottom Master Deck
    auto deckCard = juce::Rectangle<float>(28, 514, 724, 40);
    HardwareMaterials::drawRecessedPanel(g, deckCard, 5.0f, chassisTexture);

    HardwareMaterials::drawHexBolt(g, deckCard.getX() + 5, deckCard.getY() + 5);
    HardwareMaterials::drawHexBolt(g, deckCard.getRight() - 5, deckCard.getY() + 5);

    g.setColour(juce::Colour(0xff606f7b));
    g.setFont(juce::Font(9.0f, juce::Font::plain));
    g.drawText("UNDERGROUND v4.2.4 | VST3 64-BIT", (int)deckCard.getRight() - 220, (int)deckCard.getY() + 14, 210, 14, juce::Justification::right);
}

void UndergroundAudioProcessorEditor::resized()
{
    // 1. Top Crown DEGENERATE Macro Knob & Flanking Dynamics Knobs
    degenerateKnob.setBounds(345, 10, 100, 100);

    compSqueezeSlider.setBounds(272, 22, 58, 58);
    compSqueezeLabel.setBounds(268, 80, 66, 12);

    deEssAmountSlider.setBounds(460, 22, 58, 58);
    deEssAmountLabel.setBounds(456, 80, 66, 12);

    // Preset Controls Bar & Top Action Buttons (Top-Right)

    prevPresetButton.setBounds(540, 12, 24, 24);
    presetModeBox.setBounds(568, 12, 140, 24);
    nextPresetButton.setBounds(712, 12, 24, 24);

    abButton.setBounds(568, 40, 50, 22);
    bypassButton.setBounds(623, 40, 60, 22);
    oversamplingButton.setBounds(688, 40, 85, 22);

    // 2. Upper Real-Time Visual EQ Window (Between LED meters)
    visualEQDisplay.setBounds(100, 150, 580, 172);

    // 3. Middle 5 Module Columns Positioning (3 Knobs Per Column + Bypass Toggle Switch)
    float colW = 140.0f;
    float colGap = 6.0f;
    float startX = 28.0f;
    float colY = 338.0f;

    subEnableToggle.setBounds((int)(startX + 0 * (colW + colGap) + colW - 32.0f), (int)colY + 4, 20, 20);
    gritEnableToggle.setBounds((int)(startX + 1 * (colW + colGap) + colW - 32.0f), (int)colY + 4, 20, 20);
    punishButton.setBounds((int)(startX + 1 * (colW + colGap) + 48.0f), (int)colY + 5, 54, 16);
    modEnableToggle.setBounds((int)(startX + 2 * (colW + colGap) + colW - 32.0f), (int)colY + 4, 20, 20);
    delayEnableToggle.setBounds((int)(startX + 3 * (colW + colGap) + colW - 32.0f), (int)colY + 4, 20, 20);
    reverbEnableToggle.setBounds((int)(startX + 4 * (colW + colGap) + colW - 32.0f), (int)colY + 4, 20, 20);

    // Column 1: DEMON / PITCH ENGINE (Dual-Row 4-Knob Matrix + Link + Mode)
    float col1X = startX;
    demonPitchSlider.setBounds((int)col1X + 8, (int)colY + 22, 46, 46);
    demonPitchLabel.setBounds((int)col1X + 4, (int)colY + 66, 54, 10);

    demonFormantSlider.setBounds((int)col1X + 62, (int)colY + 22, 46, 46);
    demonFormantLabel.setBounds((int)col1X + 58, (int)colY + 66, 54, 10);

    demonMixSlider.setBounds((int)col1X + 8, (int)colY + 76, 46, 46);
    demonMixLabel.setBounds((int)col1X + 4, (int)colY + 120, 54, 10);

    demonDriveSlider.setBounds((int)col1X + 62, (int)colY + 76, 46, 46);
    demonDriveLabel.setBounds((int)col1X + 58, (int)colY + 120, 54, 10);

    demonLinkButton.setBounds((int)col1X + 8, (int)colY + 138, 44, 22);
    demonModeBox.setBounds((int)col1X + 56, (int)colY + 138, 58, 22);

    auto layoutCol = [&](int colIdx, juce::Slider& s1, juce::Label& l1, juce::Slider& s2, juce::Label& l2, juce::Slider& s3, juce::Label& l3) {
        float x = startX + colIdx * (colW + colGap);
        float knobSize = 46.0f;
        float knobX = x + (116.0f - knobSize) * 0.5f;

        s1.setBounds((int)knobX, (int)colY + 20, (int)knobSize, (int)knobSize);
        l1.setBounds((int)x + 4, (int)colY + 66, 112, 10);

        s2.setBounds((int)knobX, (int)colY + 76, (int)knobSize, (int)knobSize);
        l2.setBounds((int)x + 4, (int)colY + 122, 112, 10);

        s3.setBounds((int)knobX, (int)colY + 130, (int)knobSize, (int)knobSize);
        l3.setBounds((int)x + 4, (int)colY + 160, 112, 10);
    };

    layoutCol(1, gritFuzzSlider, gritFuzzLabel, gritDustSlider, gritDustLabel, gritBitSlider, gritBitLabel);
    layoutCol(2, modChorusSlider, modChorusLabel, modPhaseSlider, modPhaseLabel, modVibeSlider, modVibeLabel);
    layoutCol(3, delayTimeSlider, delayTimeLabel, delayFbSlider, delayFbLabel, delayMixSlider, delayMixLabel);
    layoutCol(4, verbSizeSlider, verbSizeLabel, verbDecaySlider, verbDecayLabel, verbSpaceSlider, verbSpaceLabel);

    // 4. Bottom Master Deck Controls
    float deckY = 516.0f;
    inputGainSlider.setBounds(38, (int)deckY, 32, 32);
    inputGainLabel.setBounds(38, (int)deckY + 30, 32, 9);

    outputGainSlider.setBounds(85, (int)deckY, 32, 32);
    outputGainLabel.setBounds(85, (int)deckY + 30, 32, 9);

    mixGlobalSlider.setBounds(145, (int)deckY - 4, 40, 40);
    mixGlobalLabel.setBounds(145, (int)deckY + 34, 40, 9);

    airMidSlider.setBounds(210, (int)deckY - 4, 40, 40);
    airMidLabel.setBounds(205, (int)deckY + 34, 50, 9);

    airTopSlider.setBounds(275, (int)deckY - 4, 40, 40);
    airTopLabel.setBounds(270, (int)deckY + 34, 50, 9);
}


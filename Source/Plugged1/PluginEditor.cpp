#include "PluginEditor.h"

namespace Plugged1
{

// ==========================================
// Custom Look And Feel
// ==========================================
PluggedLookAndFeel::PluggedLookAndFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff0d0f14));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff00d4ff));
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffffffff));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff181b24));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff2a3042));
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffeceff4));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff141720));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffeceff4));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff00d4ff).withAlpha(0.3f));
}

void PluggedLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                          float sliderPosProportional, float rotaryStartAngle,
                                          float rotaryEndAngle, juce::Slider& slider)
{
    auto radius = (float) juce::jmin(width / 2, height / 2) - 4.0f;
    auto centreX = (float) x + (float) width * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Background track arc
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centreX, centreY, radius - 3.0f, radius - 3.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(0xff1e2230));
    g.strokePath(backgroundArc, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Active value arc
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius - 3.0f, radius - 3.0f, 0.0f, rotaryStartAngle, angle, true);
        
        bool isMacro = slider.getName().containsIgnoreCase("macro");
        juce::Colour activeColor = isMacro ? juce::Colour(0xff00e5ff) : juce::Colour(0xff00b4d8);
        g.setColour(activeColor);
        g.strokePath(valueArc, juce::PathStrokeType(isMacro ? 5.5f : 4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Dial Body (Dark circular knob)
    float dialRadius = radius - 8.0f;
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff222736), centreX, centreY - dialRadius,
                                           juce::Colour(0xff12151e), centreX, centreY + dialRadius, false));
    g.fillEllipse(centreX - dialRadius, centreY - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f);

    g.setColour(juce::Colour(0xff323a50));
    g.drawEllipse(centreX - dialRadius, centreY - dialRadius, dialRadius * 2.0f, dialRadius * 2.0f, 1.0f);

    // Indicator needle
    juce::Path p;
    auto pointerLength = dialRadius * 0.75f;
    p.addRoundedRectangle(-2.0f, -dialRadius, 4.0f, pointerLength, 1.5f);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(juce::Colour(0xffffffff));
    g.fillPath(p);
}

void PluggedLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                      int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                      juce::ComboBox& box)
{
    auto cornerSize = 5.0f;
    juce::Rectangle<float> r(0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f);

    g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(r, cornerSize);

    g.setColour(box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(r, cornerSize, 1.0f);

    // Arrow
    juce::Path path;
    path.startNewSubPath((float) width - 18.0f, (float) height * 0.45f);
    path.lineTo((float) width - 12.0f, (float) height * 0.60f);
    path.lineTo((float) width - 6.0f, (float) height * 0.45f);

    g.setColour(juce::Colour(0xff00d4ff));
    g.strokePath(path, juce::PathStrokeType(1.5f));
}

// ==========================================
// Oscilloscope Component
// ==========================================
OscilloscopeComponent::OscilloscopeComponent(Plugged1AudioProcessor& processor)
    : audioProcessor(processor)
{
    startTimerHz(30); // 30 FPS rendering
}

OscilloscopeComponent::~OscilloscopeComponent()
{
    stopTimer();
}

void OscilloscopeComponent::timerCallback()
{
    audioProcessor.getVisualizerData(waveData.data(), static_cast<int>(waveData.size()));
    repaint();
}

void OscilloscopeComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(juce::Colour(0xff12151e));
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(juce::Colour(0xff1e2333));
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Center grid line
    g.setColour(juce::Colour(0xff1c2130));
    g.drawHorizontalLine(static_cast<int>(bounds.getCentreY()), bounds.getX() + 4.0f, bounds.getRight() - 4.0f);

    // Waveform Path
    juce::Path wavePath;
    const int numSamples = static_cast<int>(waveData.size());
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float midY = bounds.getCentreY();

    wavePath.startNewSubPath(bounds.getX(), midY);

    for (int i = 0; i < numSamples; ++i)
    {
        float x = bounds.getX() + (static_cast<float>(i) / static_cast<float>(numSamples)) * w;
        float y = midY - (waveData[i] * (h * 0.42f));
        wavePath.lineTo(x, y);
    }

    // Glowing wave line
    g.setColour(juce::Colour(0xff00e5ff).withAlpha(0.2f));
    g.strokePath(wavePath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour(juce::Colour(0xff00e5ff));
    g.strokePath(wavePath, juce::PathStrokeType(1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

Plugged1AudioProcessorEditor::Plugged1AudioProcessorEditor(Plugged1AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), oscilloscope(p),
      keyboardComponent(audioProcessor.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel(&lookAndFeel);

    // Make plugin freely resizable and scalable across all display resolutions
    setResizable(true, true);
    setResizeLimits(650, 460, 1600, 1150);
    getConstrainer()->setFixedAspectRatio(880.0 / 620.0);
    setSize(840, 590); // Balanced default size for 1080p and laptop screens

    // 1. Setup Header & Preset Browser
    addAndMakeVisible(categoryBox);
    addAndMakeVisible(presetBox);
    addAndMakeVisible(prevPresetButton);
    addAndMakeVisible(nextPresetButton);
    addAndMakeVisible(voiceModeBox);

    voiceModeBox.addItem("Poly", 1);
    voiceModeBox.addItem("Mono", 2);
    voiceModeBox.addItem("Legato", 3);
    voiceModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), "voice_mode", voiceModeBox);

    auto& pm = audioProcessor.getPresetManager();
    auto categories = pm.getCategories();
    for (int i = 0; i < (int) categories.size(); ++i)
        categoryBox.addItem(categories[i], i + 1);

    categoryBox.setSelectedId(1, juce::dontSendNotification);
    updatePresetDropdown();

    categoryBox.onChange = [this]() {
        updatePresetDropdown();
    };

    presetBox.onChange = [this]() {
        int selectedId = presetBox.getSelectedId();
        if (selectedId > 0)
        {
            auto& mgr = audioProcessor.getPresetManager();
            mgr.loadPreset(selectedId - 1);
        }
    };

    prevPresetButton.onClick = [this]() {
        auto& mgr = audioProcessor.getPresetManager();
        int cur = mgr.getCurrentPresetIndex();
        if (cur > 0)
        {
            mgr.loadPreset(cur - 1);
            presetBox.setSelectedId(cur, juce::dontSendNotification);
        }
    };

    nextPresetButton.onClick = [this]() {
        auto& mgr = audioProcessor.getPresetManager();
        int cur = mgr.getCurrentPresetIndex();
        if (cur < (int) mgr.getFactoryPresets().size() - 1)
        {
            mgr.loadPreset(cur + 1);
            presetBox.setSelectedId(cur + 2, juce::dontSendNotification);
        }
    };

    // Quick Zoom / Scale Preset Buttons
    auto setupScaleBtn = [&](juce::TextButton& btn) {
        addAndMakeVisible(btn);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff181b24));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff8a99ad));
    };
    setupScaleBtn(scale75Button);
    setupScaleBtn(scale100Button);
    setupScaleBtn(scale125Button);

    scale75Button.onClick  = [this]() { setSize(670, 472); };
    scale100Button.onClick = [this]() { setSize(840, 590); };
    scale125Button.onClick = [this]() { setSize(1050, 740); };

    // 2. Oscilloscope
    addAndMakeVisible(oscilloscope);

    // 3. Macro Knobs
    macroPunchSlider.setName("macro_punch");
    macroDirtSlider.setName("macro_dirt");
    macroSpaceSlider.setName("macro_space");
    macroAirSlider.setName("macro_air");

    setupSlider(macroPunchSlider, macroPunchLabel, "PUNCH", "macro_punch", macroPunchAttach);
    setupSlider(macroDirtSlider, macroDirtLabel, "DIRT", "macro_dirt", macroDirtAttach);
    setupSlider(macroSpaceSlider, macroSpaceLabel, "SPACE", "macro_space", macroSpaceAttach);
    setupSlider(macroAirSlider, macroAirLabel, "AIR", "macro_air", macroAirAttach);

    // 4. Sub / 808 Machine
    setupSlider(glideSlider, glideLabel, "GLIDE", "glide_time", glideAttach);
    setupSlider(subPunchSlider, subPunchLabel, "PUNCH", "sub_punch_amount", subPunchAttach);
    setupSlider(subDriveSlider, subDriveLabel, "DRIVE", "sub_drive", subDriveAttach);
    setupSlider(subGainSlider, subGainLabel, "SUB GAIN", "sub_gain", subGainAttach);

    // 5. Synth Engine
    setupSlider(synthUnisonSlider, synthUnisonLabel, "UNISON", "synth_unison", synthUnisonAttach);
    setupSlider(synthDetuneSlider, synthDetuneLabel, "DETUNE", "synth_detune", synthDetuneAttach);
    setupSlider(synthSpreadSlider, synthSpreadLabel, "SPREAD", "synth_spread", synthSpreadAttach);
    setupSlider(synthGainSlider, synthGainLabel, "SYNTH GAIN", "synth_gain", synthGainAttach);

    // 6. Amp ADSR
    setupSlider(ampAttackSlider, ampAttackLabel, "ATTACK", "amp_attack", ampAttackAttach);
    setupSlider(ampDecaySlider, ampDecayLabel, "DECAY", "amp_decay", ampDecayAttach);
    setupSlider(ampSustainSlider, ampSustainLabel, "SUSTAIN", "amp_sustain", ampSustainAttach);
    setupSlider(ampReleaseSlider, ampReleaseLabel, "RELEASE", "amp_release", ampReleaseAttach);

    // 7. Filter & FX
    setupSlider(cutoffSlider, cutoffLabel, "CUTOFF", "filter_cutoff", cutoffAttach);
    setupSlider(resSlider, resLabel, "RES", "filter_resonance", resAttach);
    setupSlider(fxDriveSlider, fxDriveLabel, "SATURATION", "fx_drive_amount", fxDriveAttach);
    setupSlider(delayMixSlider, delayMixLabel, "DELAY", "fx_delay_mix", delayMixAttach);
    setupSlider(reverbMixSlider, reverbMixLabel, "REVERB", "fx_reverb_mix", reverbMixAttach);
    setupSlider(masterGainSlider, masterGainLabel, "MASTER", "master_gain", masterGainAttach);

    // 8. Interactive Virtual MIDI Piano Keyboard
    addAndMakeVisible(keyboardComponent);
    keyboardComponent.setAvailableRange(24, 96); // C1 (24) to C7 (96) - 6 full octaves
    keyboardComponent.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xffe8edf4));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff12151e));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xff00e5ff).withAlpha(0.75f));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0xff00e5ff).withAlpha(0.25f));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, juce::Colour(0xff181b24));
    keyboardComponent.setColour(juce::MidiKeyboardComponent::upDownButtonArrowColourId, juce::Colour(0xff00e5ff));
}

Plugged1AudioProcessorEditor::~Plugged1AudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void Plugged1AudioProcessorEditor::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text,
                                               const juce::String& paramId, std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setFont(juce::Font(11.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xff9aa5b8));
    addAndMakeVisible(label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), paramId, slider);
}

void Plugged1AudioProcessorEditor::updatePresetDropdown()
{
    presetBox.clear(juce::dontSendNotification);
    auto& pm = audioProcessor.getPresetManager();
    int catId = categoryBox.getSelectedId();
    if (catId > 0)
    {
        auto categories = pm.getCategories();
        if (catId - 1 < (int) categories.size())
        {
            auto catName = categories[catId - 1];
            auto indices = pm.getPresetIndicesForCategory(catName);
            for (int idx : indices)
            {
                presetBox.addItem(pm.getFactoryPresets()[idx].name, idx + 1);
            }
            if (!indices.empty())
                presetBox.setSelectedId(indices[0] + 1, juce::sendNotification);
        }
    }
}

void Plugged1AudioProcessorEditor::paint(juce::Graphics& g)
{
    float w = (float) getWidth();
    float h = (float) getHeight();
    float scale = h / 620.0f;

    // Background gradient
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff10131b), 0, 0,
                                           juce::Colour(0xff090a0f), 0, h, false));
    g.fillAll();

    // Top Header Bar
    float headerH = h * 0.095f;
    g.setColour(juce::Colour(0xff161a25));
    g.fillRect(0.0f, 0.0f, w, headerH);
    g.setColour(juce::Colour(0xff22283a));
    g.drawHorizontalLine((int)headerH, 0.0f, w);

    // Logo & Branding
    g.setColour(juce::Colour(0xff00e5ff));
    g.setFont(juce::Font(std::max(14.0f, 20.0f * scale), juce::Font::bold));
    g.drawText("PLUGGED 1", (int)(w * 0.025f), (int)(headerH * 0.15f), (int)(w * 0.18f), (int)(headerH * 0.45f), juce::Justification::left);

    g.setColour(juce::Colour(0xff6e7a91));
    g.setFont(juce::Font(std::max(9.0f, 10.5f * scale)));
    g.drawText("PLUGGED IN CENTRAL", (int)(w * 0.025f), (int)(headerH * 0.55f), (int)(w * 0.18f), (int)(headerH * 0.35f), juce::Justification::left);

    // Section Box Outlines
    auto drawSectionBox = [&](juce::Rectangle<int> area, const juce::String& title) {
        g.setColour(juce::Colour(0xff131620));
        g.fillRoundedRectangle(area.toFloat(), 5.0f * scale);
        g.setColour(juce::Colour(0xff202636));
        g.drawRoundedRectangle(area.toFloat(), 5.0f * scale, 1.0f);

        // Header tab
        g.setColour(juce::Colour(0xff00e5ff));
        g.setFont(juce::Font(std::max(9.5f, 11.0f * scale), juce::Font::bold));
        g.drawText(title, area.getX() + (int)(12 * scale), area.getY() + (int)(6 * scale), area.getWidth() - (int)(24 * scale), (int)(16 * scale), juce::Justification::left);
    };

    int padX = (int)(w * 0.02f);
    int rowW = (int)(w - 2 * padX);

    // Middle 3 Panels
    int pW = (rowW - (int)(20 * scale)) / 3;
    int p1X = padX;
    int p2X = p1X + pW + (int)(10 * scale);
    int p3X = p2X + pW + (int)(10 * scale);

    int row2Y = (int)(h * 0.355f);
    int row2H = (int)(h * 0.265f);

    drawSectionBox(juce::Rectangle<int>(p1X, row2Y, pW, row2H), "SUB & 808 MACHINE");
    drawSectionBox(juce::Rectangle<int>(p2X, row2Y, pW, row2H), "SYNTH & PLUCKS");
    drawSectionBox(juce::Rectangle<int>(p3X, row2Y, pW, row2H), "AMP ENVELOPE");

    // Studio DSP FX Rack
    int row3Y = (int)(h * 0.635f);
    int row3H = (int)(h * 0.215f);
    drawSectionBox(juce::Rectangle<int>(padX, row3Y, rowW, row3H), "STUDIO DSP & MASTER FX RACK");

    // Keyboard Section
    int row4Y = (int)(h * 0.865f);
    int row4H = (int)(h * 0.125f);
    drawSectionBox(juce::Rectangle<int>(padX, row4Y, rowW, row4H), "INTERACTIVE MIDI KEYBOARD / SOUND TEST");
}

void Plugged1AudioProcessorEditor::resized()
{
    float w = (float) getWidth();
    float h = (float) getHeight();
    float scale = h / 620.0f;

    int padX = (int)(w * 0.02f);
    int rowW = (int)(w - 2 * padX);
    float headerH = h * 0.095f;

    // Header Controls
    int headerY = (int)(headerH * 0.22f);
    int headerItemH = (int)(headerH * 0.58f);

    int catX = (int)(w * 0.20f);
    int catW = (int)(w * 0.15f);
    categoryBox.setBounds(catX, headerY, catW, headerItemH);

    int preX = catX + catW + (int)(6 * scale);
    int preW = (int)(w * 0.23f);
    presetBox.setBounds(preX, headerY, preW, headerItemH);

    int btnW = (int)(headerItemH * 1.0f);
    prevPresetButton.setBounds(preX + preW + (int)(5 * scale), headerY, btnW, headerItemH);
    nextPresetButton.setBounds(preX + preW + btnW + (int)(8 * scale), headerY, btnW, headerItemH);

    int modeX = preX + preW + btnW * 2 + (int)(14 * scale);
    int modeW = (int)(w * 0.10f);
    voiceModeBox.setBounds(modeX, headerY, modeW, headerItemH);

    int scaleBtnW = (int)(w * 0.045f);
    int scaleX = (int)(w - padX - 3 * scaleBtnW - (int)(6 * scale));
    scale75Button.setBounds(scaleX, headerY, scaleBtnW, headerItemH);
    scale100Button.setBounds(scaleX + scaleBtnW + 3, headerY, scaleBtnW, headerItemH);
    scale125Button.setBounds(scaleX + scaleBtnW * 2 + 6, headerY, scaleBtnW, headerItemH);

    // Hero Section: Oscilloscope (Left) & 4 Macros (Right)
    int heroY = (int)(h * 0.115f);
    int heroH = (int)(h * 0.225f);
    int oscW = (int)(w * 0.44f);
    oscilloscope.setBounds(padX, heroY, oscW, heroH);

    int macroStartX = padX + oscW + (int)(14 * scale);
    int macroAreaW = (int)(w - macroStartX - padX);
    int macroSpacing = macroAreaW / 4;
    int macroSize = std::min((int)(heroH * 0.65f), (int)(macroSpacing * 0.75f));

    auto placeMacro = [&](juce::Slider& s, juce::Label& l, int index) {
        int x = macroStartX + index * macroSpacing + (macroSpacing - macroSize) / 2;
        int y = heroY + (int)(heroH * 0.08f);
        s.setBounds(x, y, macroSize, macroSize);
        l.setBounds(x - (int)(10 * scale), y + macroSize + 2, macroSize + (int)(20 * scale), (int)(15 * scale));
        l.setFont(juce::Font(std::max(8.5f, 10.5f * scale), juce::Font::bold));
    };

    placeMacro(macroPunchSlider, macroPunchLabel, 0);
    placeMacro(macroDirtSlider, macroDirtLabel, 1);
    placeMacro(macroSpaceSlider, macroSpaceLabel, 2);
    placeMacro(macroAirSlider, macroAirLabel, 3);

    // Row 2: 3 Panels
    int row2Y = (int)(h * 0.355f);
    int row2H = (int)(h * 0.265f);
    int pW = (rowW - (int)(20 * scale)) / 3;
    int p1X = padX;
    int p2X = p1X + pW + (int)(10 * scale);
    int p3X = p2X + pW + (int)(10 * scale);

    int kSize = std::min((int)(row2H * 0.45f), (int)(pW * 0.22f));
    int knobY = row2Y + (int)(row2H * 0.28f);

    auto placeRowKnob = [&](juce::Slider& s, juce::Label& l, int panelX, int idx, int totalInPanel) {
        int spacing = pW / totalInPanel;
        int x = panelX + idx * spacing + (spacing - kSize) / 2;
        s.setBounds(x, knobY, kSize, kSize);
        l.setBounds(x - (int)(8 * scale), knobY + kSize + 2, kSize + (int)(16 * scale), (int)(14 * scale));
        l.setFont(juce::Font(std::max(8.0f, 9.5f * scale), juce::Font::bold));
    };

    // Panel 1: Sub / 808
    placeRowKnob(glideSlider, glideLabel, p1X, 0, 4);
    placeRowKnob(subPunchSlider, subPunchLabel, p1X, 1, 4);
    placeRowKnob(subDriveSlider, subDriveLabel, p1X, 2, 4);
    placeRowKnob(subGainSlider, subGainLabel, p1X, 3, 4);

    // Panel 2: Synth
    placeRowKnob(synthUnisonSlider, synthUnisonLabel, p2X, 0, 4);
    placeRowKnob(synthDetuneSlider, synthDetuneLabel, p2X, 1, 4);
    placeRowKnob(synthSpreadSlider, synthSpreadLabel, p2X, 2, 4);
    placeRowKnob(synthGainSlider, synthGainLabel, p2X, 3, 4);

    // Panel 3: Amp ADSR
    placeRowKnob(ampAttackSlider, ampAttackLabel, p3X, 0, 4);
    placeRowKnob(ampDecaySlider, ampDecayLabel, p3X, 1, 4);
    placeRowKnob(ampSustainSlider, ampSustainLabel, p3X, 2, 4);
    placeRowKnob(ampReleaseSlider, ampReleaseLabel, p3X, 3, 4);

    // Row 3: Studio DSP & Master FX Rack
    int row3Y = (int)(h * 0.635f);
    int row3H = (int)(h * 0.215f);
    int fxKnobSize = std::min((int)(row3H * 0.52f), (int)(rowW / 7.0f));
    int fxKnobY = row3Y + (int)(row3H * 0.24f);

    auto placeFXKnob = [&](juce::Slider& s, juce::Label& l, int idx) {
        int spacing = rowW / 6;
        int x = padX + idx * spacing + (spacing - fxKnobSize) / 2;
        s.setBounds(x, fxKnobY, fxKnobSize, fxKnobSize);
        l.setBounds(x - (int)(10 * scale), fxKnobY + fxKnobSize + 2, fxKnobSize + (int)(20 * scale), (int)(14 * scale));
        l.setFont(juce::Font(std::max(8.0f, 9.5f * scale), juce::Font::bold));
    };

    placeFXKnob(cutoffSlider, cutoffLabel, 0);
    placeFXKnob(resSlider, resLabel, 1);
    placeFXKnob(fxDriveSlider, fxDriveLabel, 2);
    placeFXKnob(delayMixSlider, delayMixLabel, 3);
    placeFXKnob(reverbMixSlider, reverbMixLabel, 4);
    placeFXKnob(masterGainSlider, masterGainLabel, 5);

    // Row 4: Virtual Keyboard
    int row4Y = (int)(h * 0.865f);
    int row4H = (int)(h * 0.125f);
    int kbY = row4Y + (int)(row4H * 0.24f);
    int kbH = row4H - (int)(row4H * 0.28f);
    int kbW = rowW - (int)(16 * scale);
    keyboardComponent.setBounds(padX + (int)(8 * scale), kbY, kbW, kbH);
    keyboardComponent.setKeyWidth(std::max(12.0f, (float)kbW / 44.0f));
}

} // namespace Plugged1

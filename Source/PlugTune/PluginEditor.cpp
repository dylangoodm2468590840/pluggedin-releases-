#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BuildInfo.h"
#include <cmath>

static const char* sRootNotes[] = {
    "C", "C# / Db", "D", "D# / Eb", "E", "F", "F# / Gb", "G", "G# / Ab", "A", "A# / Bb", "B"
};
static const char* sScales[] = {
    "Minor (m)", "Major (M)", "Chromatic (All)", "Harmonic Minor", "Pentatonic Minor", "Pentatonic Major", "Custom"
};
static const char* sRanges[] = { "Low (Male)", "Mid (Default)", "High (Female/Pop)" };
static const char* sGroups[] = { "Group: None", "Group: A", "Group: B", "Group: C", "Group: D" };

PlugTuneAudioProcessorEditor::PlugTuneAudioProcessorEditor(PlugTuneAudioProcessor& p)
    : AudioProcessorEditor(&p), mProcessor(p)
{
    setLookAndFeel(&mLookAndFeel);

    // 1. Top Bar Controls
    for (int i = 0; i < 5; ++i) mGroupCombo.addItem(sGroups[i], i + 1);
    mGroupCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mGroupCombo);
    mGroupAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        mProcessor.getAPVTS(), "group", mGroupCombo);

    for (int i = 0; i < 3; ++i) mVocalRangeCombo.addItem(sRanges[i], i + 1);
    mVocalRangeCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mVocalRangeCombo);
    mVocalRangeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        mProcessor.getAPVTS(), "vocalRange", mVocalRangeCombo);

    mLiveModeToggle.setButtonText("⚡ LIVE REC");
    mLiveModeToggle.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(mLiveModeToggle);
    mLiveModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        mProcessor.getAPVTS(), "liveMode", mLiveModeToggle);

    // 2. Drag & Drop Key Dropzone
    mDropZoneLabel.setText("📁 DRAG & DROP BEAT AUDIO HERE FOR INSTANT KEY DETECTION", juce::dontSendNotification);
    mDropZoneLabel.setJustificationType(juce::Justification::centred);
    mDropZoneLabel.setColour(juce::Label::textColourId, PlugTuneUI::PlugTuneLookFeel::getTextDim());
    mDropZoneLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    addAndMakeVisible(mDropZoneLabel);

    mApplyKeyButton.setButtonText("LOCK KEY");
    mApplyKeyButton.setVisible(false);
    mApplyKeyButton.onClick = [this]() {
        auto* rootParam = dynamic_cast<juce::AudioParameterChoice*>(mProcessor.getAPVTS().getParameter("rootKey"));
        auto* scaleParam = dynamic_cast<juce::AudioParameterChoice*>(mProcessor.getAPVTS().getParameter("scaleType"));
        if (rootParam && scaleParam)
        {
            *rootParam = mDetectedRootKey;
            *scaleParam = mDetectedScaleType;
        }
        mApplyKeyButton.setVisible(false);
        mDropZoneLabel.setText("KEY LOCKED: " + mDetectedKeyText + " ✓", juce::dontSendNotification);
    };
    addAndMakeVisible(mApplyKeyButton);

    // 3. Key & Scale Bar
    for (int i = 0; i < 12; ++i) mRootKeyCombo.addItem(sRootNotes[i], i + 1);
    mRootKeyCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mRootKeyCombo);
    mRootKeyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        mProcessor.getAPVTS(), "rootKey", mRootKeyCombo);

    for (int i = 0; i < 7; ++i) mScaleTypeCombo.addItem(sScales[i], i + 1);
    mScaleTypeCombo.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mScaleTypeCombo);
    mScaleTypeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        mProcessor.getAPVTS(), "scaleType", mScaleTypeCombo);

    mToneToggle.setButtonText("♫ TONE");
    mToneToggle.onClick = [this]() {
        mProcessor.getToneGenerator().setEnabled(mToneToggle.getToggleState());
    };
    addAndMakeVisible(mToneToggle);

    // 4. Interactive HeatMap Keyboard
    addAndMakeVisible(mHeatMapKeyboard);
    mHeatMapKeyboard.onNoteClicked = [this](int noteIdx) {
        if (mToneToggle.getToggleState())
        {
            mProcessor.getToneGenerator().playNote(noteIdx, 4);
        }
    };
    mHeatMapKeyboard.onNoteToggled = [this](int noteIdx, bool newState) {
        mProcessor.getQuantizer().setNoteEnabled(noteIdx, newState);
    };

    // 5. The 3 Clean Main Dials
    setupKnob(mTuneAmountSlider, mTuneAmountLabel, "TUNE AMOUNT", "%");
    mTuneAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        mProcessor.getAPVTS(), "tuneAmount", mTuneAmountSlider);

    setupKnob(mFormantSlider, mFormantLabel, "FORMANT SHIFT", " st");
    mFormantAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        mProcessor.getAPVTS(), "formant", mFormantSlider);

    setupKnob(mDoublerSlider, mDoublerLabel, "VOCAL DOUBLER", "%");
    mDoublerAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        mProcessor.getAPVTS(), "doubler", mDoublerSlider);

    setSize(660, 490);
    startTimerHz(30);
}

PlugTuneAudioProcessorEditor::~PlugTuneAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PlugTuneAudioProcessorEditor::setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxTextColourId, PlugTuneUI::PlugTuneLookFeel::getTextColor());
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(13.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, PlugTuneUI::PlugTuneLookFeel::getTextColor());
    addAndMakeVisible(label);
}

void PlugTuneAudioProcessorEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background Dark Gradient
    juce::ColourGradient bgGrad(PlugTuneUI::PlugTuneLookFeel::getDarkBg(), 0, 0,
                                PlugTuneUI::PlugTuneLookFeel::getDarkBg().darker(0.3f), 0, bounds.getHeight(), false);
    g.setGradientFill(bgGrad);
    g.fillRect(bounds);

    // Header Title
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.setColour(PlugTuneUI::PlugTuneLookFeel::getTextColor());
    g.drawText("PLUGTUNE", 20, 15, 120, 25, juce::Justification::centredLeft, false);

    // Subtitle & Build Badge
    g.setFont(juce::Font(11.0f, juce::Font::plain));
    g.setColour(PlugTuneUI::PlugTuneLookFeel::getTextDim());
    g.drawText("PluggedIN AUDIO | Real-Time Vocal Pitch & Formant Engine", 130, 19, 320, 20, juce::Justification::centredLeft, false);

    // Dev Build Badge
    auto badgeRect = juce::Rectangle<float>(bounds.getWidth() - 110, 16, 90, 22);
    g.setColour(PlugTuneUI::PlugTuneLookFeel::getCardBg());
    g.fillRoundedRectangle(badgeRect, 4.0f);
    g.setColour(PlugTuneUI::PlugTuneLookFeel::getCardBorder());
    g.drawRoundedRectangle(badgeRect.reduced(0.5f), 4.0f, 1.0f);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.setColour(PlugTuneUI::PlugTuneLookFeel::getElectricCyan());
    g.drawText(PLUGTUNE_BUILD_ID, badgeRect, juce::Justification::centred, false);

    // Dropzone Box
    auto dropBox = juce::Rectangle<float>(20, 48, bounds.getWidth() - 40, 36);
    juce::Colour dropBg = mIsFileDragOver ? PlugTuneUI::PlugTuneLookFeel::getElectricCyan().withAlpha(0.18f)
                                         : PlugTuneUI::PlugTuneLookFeel::getCardBg();
    g.setColour(dropBg);
    g.fillRoundedRectangle(dropBox, 6.0f);

    juce::Colour dropBorder = mIsFileDragOver ? PlugTuneUI::PlugTuneLookFeel::getNeonGreen()
                                             : PlugTuneUI::PlugTuneLookFeel::getCardBorder();
    g.setColour(dropBorder);
    g.drawRoundedRectangle(dropBox.reduced(0.5f), 6.0f, mIsFileDragOver ? 2.0f : 1.0f);

    // Section Divider
    g.setColour(PlugTuneUI::PlugTuneLookFeel::getCardBorder());
    g.drawLine(20, 94, bounds.getWidth() - 20, 94, 1.0f);
}

void PlugTuneAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Top Bar Right Options
    mGroupCombo.setBounds(bounds.getWidth() - 340, 16, 105, 24);
    mVocalRangeCombo.setBounds(bounds.getWidth() - 225, 16, 105, 24);
    mLiveModeToggle.setBounds(bounds.getWidth() - 440, 16, 90, 24);

    // Drop Zone
    mDropZoneLabel.setBounds(30, 52, bounds.getWidth() - 180, 28);
    mApplyKeyButton.setBounds(bounds.getWidth() - 140, 52, 110, 28);

    // Key & Scale Bar (Y: 104)
    mRootKeyCombo.setBounds(20, 104, 110, 28);
    mScaleTypeCombo.setBounds(140, 104, 160, 28);
    mToneToggle.setBounds(310, 104, 85, 28);

    // Keyboard (Y: 142, H: 95)
    mHeatMapKeyboard.setBounds(20, 142, bounds.getWidth() - 40, 95);

    // The 3 Main Controls Area (Y: 260, H: 200)
    int knobW = 140;
    int knobH = 140;
    int colW = (bounds.getWidth() - 40) / 3;
    int startX = 20;

    // Col 1: Main Tune Amount (Prominent First Dial)
    mTuneAmountLabel.setBounds(startX, 260, colW, 22);
    mTuneAmountSlider.setBounds(startX + (colW - knobW) / 2, 290, knobW, knobH);

    // Col 2: Formant Shift
    mFormantLabel.setBounds(startX + colW, 260, colW, 22);
    mFormantSlider.setBounds(startX + colW + (colW - knobW) / 2, 290, knobW, knobH);

    // Col 3: Vocal Doubler
    mDoublerLabel.setBounds(startX + colW * 2, 260, colW, 22);
    mDoublerSlider.setBounds(startX + colW * 2 + (colW - knobW) / 2, 290, knobW, knobH);
}

void PlugTuneAudioProcessorEditor::timerCallback()
{
    // Feed Live Pitch Telemetry to HeatMap Keyboard
    if (mProcessor.getLiveIsVoiced())
    {
        int note = mProcessor.getLiveDetectedNote();
        float clarity = mProcessor.getLiveClarity();
        mHeatMapKeyboard.registerPitchHit(note, clarity);
    }
}

bool PlugTuneAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (files.size() != 1) return false;
    juce::File f(files[0]);
    juce::String ext = f.getFileExtension().toLowerCase();
    return (ext == ".wav" || ext == ".mp3" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".ogg");
}

void PlugTuneAudioProcessorEditor::fileDragEnter(const juce::StringArray&, int, int)
{
    mIsFileDragOver = true;
    repaint();
}

void PlugTuneAudioProcessorEditor::fileDragExit(const juce::StringArray&)
{
    mIsFileDragOver = false;
    repaint();
}

void PlugTuneAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    mIsFileDragOver = false;
    if (files.size() > 0)
    {
        juce::File audioFile(files[0]);
        mDropZoneLabel.setText("ANALYZING: " + audioFile.getFileName() + " ...", juce::dontSendNotification);
        mApplyKeyButton.setVisible(false);

        mProcessor.getKeyDetector().analyzeFileAsync(audioFile, [this](PlugTuneDSP::KeyDetectionOutcome outcome) {
            if (outcome.success)
            {
                mDetectedRootKey = outcome.rootKey;
                mDetectedScaleType = outcome.isMinor ? 0 : 1; // 0 = Minor (m), 1 = Major (M)
                mDetectedKeyText = outcome.keyName;
                mDropZoneLabel.setText("DETECTED: " + mDetectedKeyText + " (" + juce::String(static_cast<int>(outcome.confidence * 100)) + "% Match)", juce::dontSendNotification);
                mApplyKeyButton.setVisible(true);
            }
            else
            {
                mDropZoneLabel.setText("COULD NOT DETECT KEY - TRY ANOTHER FILE", juce::dontSendNotification);
                mApplyKeyButton.setVisible(false);
            }
            resized();
        });
    }
    repaint();
}

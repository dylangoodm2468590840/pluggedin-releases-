#include "HeatMapKeyboard.h"
#include "PlugTuneLookFeel.h"
#include <algorithm>

namespace PlugTuneUI
{

static const char* sKeys12[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

bool HeatMapKeyboard::isBlackKey(int note0to11)
{
    int n = ((note0to11 % 12) + 12) % 12;
    return (n == 1 || n == 3 || n == 6 || n == 8 || n == 10);
}

const char* HeatMapKeyboard::getNoteName(int note0to11)
{
    int n = ((note0to11 % 12) + 12) % 12;
    return sKeys12[n];
}

HeatMapKeyboard::HeatMapKeyboard()
{
    mNoteActiveMask.fill(true);
    mHeatLevels.fill(0.0f);
    startTimerHz(30); // 30fps smooth decay animation
}

HeatMapKeyboard::~HeatMapKeyboard()
{
    stopTimer();
}

void HeatMapKeyboard::setNoteActive(int note0to11, bool isActive)
{
    int n = ((note0to11 % 12) + 12) % 12;
    if (mNoteActiveMask[n] != isActive)
    {
        mNoteActiveMask[n] = isActive;
        repaint();
    }
}

void HeatMapKeyboard::setAllNotes(const std::array<bool, 12>& mask)
{
    mNoteActiveMask = mask;
    repaint();
}

void HeatMapKeyboard::registerPitchHit(int note0to11, float intensity)
{
    int n = ((note0to11 % 12) + 12) % 12;
    mHeatLevels[n] = std::clamp(mHeatLevels[n] + intensity * 0.7f, 0.0f, 1.0f);
}

void HeatMapKeyboard::timerCallback()
{
    bool needRepaint = false;
    for (int i = 0; i < 12; ++i)
    {
        if (mHeatLevels[i] > 0.01f)
        {
            mHeatLevels[i] *= 0.88f; // Decay factor
            needRepaint = true;
        }
        else
        {
            mHeatLevels[i] = 0.0f;
        }
    }

    if (needRepaint)
    {
        repaint();
    }
}

void HeatMapKeyboard::resized()
{
}

void HeatMapKeyboard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float totalW = bounds.getWidth();
    float totalH = bounds.getHeight();
    float keyW = totalW / 12.0f;

    // Background container
    g.setColour(PlugTuneLookFeel::getCardBg());
    g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(PlugTuneLookFeel::getCardBorder());
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

    for (int i = 0; i < 12; ++i)
    {
        auto keyRect = juce::Rectangle<float>(i * keyW, 0.0f, keyW, totalH).reduced(2.0f, 2.0f);
        bool active = mNoteActiveMask[i];
        bool black = isBlackKey(i);
        float heat = mHeatLevels[i];

        // Base Key Color
        juce::Colour baseColor;
        if (black)
        {
            baseColor = active ? juce::Colour(0xff1e222b) : juce::Colour(0xff12141a);
        }
        else
        {
            baseColor = active ? juce::Colour(0xff2d3340) : juce::Colour(0xff181a22);
        }

        // Apply HeatMap Glow (Neon Green -> Electric Cyan blend)
        if (heat > 0.02f)
        {
            juce::Colour glowColor = PlugTuneLookFeel::getNeonGreen().interpolatedWith(PlugTuneLookFeel::getElectricCyan(), heat);
            baseColor = baseColor.interpolatedWith(glowColor, heat * 0.75f);
        }

        g.setColour(baseColor);
        g.fillRoundedRectangle(keyRect, 4.0f);

        // Key Border
        juce::Colour borderColor = active ? (black ? juce::Colour(0xff3b4354) : juce::Colour(0xff4a5568))
                                          : juce::Colour(0xff20242e);
        if (heat > 0.05f)
        {
            borderColor = PlugTuneLookFeel::getNeonGreen().withAlpha(heat);
        }
        g.setColour(borderColor);
        g.drawRoundedRectangle(keyRect, 4.0f, 1.0f);

        // Note Name Label
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        juce::Colour textCol = active ? PlugTuneLookFeel::getTextColor() : PlugTuneLookFeel::getTextDim().withAlpha(0.4f);
        if (heat > 0.1f) textCol = juce::Colours::white;

        g.setColour(textCol);
        g.drawText(getNoteName(i), keyRect.removeFromBottom(22.0f), juce::Justification::centred, false);

        // Status Indicator Dot on active notes
        if (active)
        {
            g.setColour(PlugTuneLookFeel::getElectricCyan().withAlpha(0.8f));
            g.fillEllipse(keyRect.getCentreX() - 2.5f, keyRect.getY() + 8.0f, 5.0f, 5.0f);
        }
    }
}

void HeatMapKeyboard::mouseUp(const juce::MouseEvent& e)
{
    float keyW = getWidth() / 12.0f;
    int noteIdx = std::clamp(static_cast<int>(e.position.x / keyW), 0, 11);

    if (onNoteClicked)
    {
        onNoteClicked(noteIdx);
    }

    bool newState = !mNoteActiveMask[noteIdx];
    setNoteActive(noteIdx, newState);

    if (onNoteToggled)
    {
        onNoteToggled(noteIdx, newState);
    }
}

} // namespace PlugTuneUI

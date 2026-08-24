#include "PlugTuneLookFeel.h"
#include <cmath>

namespace PlugTuneUI
{

PlugTuneLookFeel::PlugTuneLookFeel()
{
    setColour(juce::ResizableWindow::backgroundColourId, getDarkBg());
    setColour(juce::ComboBox::backgroundColourId, getCardBg());
    setColour(juce::ComboBox::textColourId, getTextColor());
    setColour(juce::ComboBox::outlineColourId, getCardBorder());
    setColour(juce::ComboBox::arrowColourId, getElectricCyan());
    setColour(juce::PopupMenu::backgroundColourId, getCardBg());
    setColour(juce::PopupMenu::textColourId, getTextColor());
    setColour(juce::PopupMenu::highlightedBackgroundColourId, getElectricCyan().withAlpha(0.25f));
}

void PlugTuneLookFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPosProportional, float rotaryStartAngle,
                                        float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(6.0f, radius * 0.16f);
    auto arcRadius = radius - lineW * 0.8f;
    auto center = bounds.getCentre();

    // 1. Background Track
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(center.x, center.y, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(getCardBorder());
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // 2. Active Value Arc (Neon Gradient)
    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(center.x, center.y, arcRadius, arcRadius, 0.0f,
                               rotaryStartAngle, toAngle, true);

        juce::ColourGradient grad(getElectricCyan(), center.x - radius, center.y,
                                  getNeonGreen(), center.x + radius, center.y, false);
        g.setGradientFill(grad);
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 3. Center Knob Disc
    auto knobRadius = arcRadius - lineW - 2.0f;
    if (knobRadius > 4.0f)
    {
        // Outer Shadow / Rim
        g.setColour(juce::Colour(0xff08090b));
        g.fillEllipse(center.x - knobRadius, center.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

        // Inner Face
        auto faceRadius = knobRadius - 2.0f;
        juce::ColourGradient faceGrad(getKnobBody().brighter(0.1f), center.x, center.y - faceRadius,
                                      getKnobBody().darker(0.2f), center.x, center.y + faceRadius, false);
        g.setGradientFill(faceGrad);
        g.fillEllipse(center.x - faceRadius, center.y - faceRadius, faceRadius * 2.0f, faceRadius * 2.0f);

        // Indicator Line
        juce::Path pointer;
        auto pLen = faceRadius * 0.75f;
        pointer.addRoundedRectangle(-1.5f, -faceRadius + 2.0f, 3.0f, pLen * 0.6f, 1.5f);
        pointer.applyTransform(juce::AffineTransform::rotation(toAngle).translated(center.x, center.y));

        g.setColour(slider.isEnabled() ? getNeonGreen() : getTextDim());
        g.fillPath(pointer);
    }
}

void PlugTuneLookFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                    int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                    juce::ComboBox& box)
{
    auto r = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(getCardBg());
    g.fillRoundedRectangle(r, 6.0f);

    g.setColour(box.hasKeyboardFocus(true) ? getElectricCyan() : getCardBorder());
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.2f);

    // Arrow
    auto arrowX = width - 18.0f;
    auto arrowY = height * 0.5f - 2.0f;
    juce::Path arrow;
    arrow.addTriangle(arrowX, arrowY, arrowX + 8.0f, arrowY, arrowX + 4.0f, arrowY + 5.0f);
    g.setColour(getElectricCyan());
    g.fillPath(arrow);
}

void PlugTuneLookFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                            const juce::Colour& /*backgroundColour*/,
                                            bool shouldDrawButtonAsHighlighted,
                                            bool shouldDrawButtonAsDown)
{
    auto r = button.getLocalBounds().toFloat();
    bool isToggled = button.getToggleState();

    juce::Colour bg = isToggled ? getElectricCyan().withAlpha(0.20f) : getCardBg();
    if (shouldDrawButtonAsHighlighted) bg = bg.brighter(0.15f);
    if (shouldDrawButtonAsDown) bg = bg.darker(0.1f);

    g.setColour(bg);
    g.fillRoundedRectangle(r, 6.0f);

    juce::Colour border = isToggled ? getElectricCyan() : getCardBorder();
    g.setColour(border);
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.2f);
}

} // namespace PlugTuneUI

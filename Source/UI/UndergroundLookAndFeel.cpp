#include "UndergroundLookAndFeel.h"
#include "HardwareMaterials.h"
#include <cmath>

UndergroundLookAndFeel::UndergroundLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, cyanAccent);
    setColour(juce::Slider::rotarySliderOutlineColourId, darkTrack);
    setColour(juce::Slider::thumbColourId, cyanAccent);

    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff0c0f17));
    setColour(juce::ComboBox::outlineColourId, cyanAccent.withAlpha(0.5f));
    setColour(juce::ComboBox::focusedOutlineColourId, cyanAccent);
    setColour(juce::ComboBox::textColourId, textColour);
    setColour(juce::ComboBox::arrowColourId, cyanAccent);

    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff0c0f17));
    setColour(juce::PopupMenu::headerTextColourId, cyanAccent);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, cyanAccent.withAlpha(0.25f));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

UndergroundLookAndFeel::~UndergroundLookAndFeel() = default;

void UndergroundLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPosProportional, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(6.0f);
    auto radius = std::min(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = 4.0f;
    auto arcRadius = radius - lineW * 0.5f - 5.0f;

    auto centre = bounds.getCentre();

    bool isDegenerate = slider.getComponentID() == "DEGENERATE";
    auto activeColour = isDegenerate ? degenerateGreen : cyanAccent;

    // 1. Cast Shadow Underneath Knob (Level 4 Depth)
    g.setColour(juce::Colour(0xff030406).withAlpha(0.7f));
    g.fillEllipse(centre.x - radius + 2.0f, centre.y - radius + 3.0f, radius * 2.0f, radius * 2.0f);

    // 2. Machined Metallic Outer Knurled Collar
    juce::ColourGradient collarGrad(
        juce::Colour(0xff2a364a), centre.x - radius, centre.y - radius,
        juce::Colour(0xff0e131d), centre.x + radius, centre.y + radius, false);
    g.setGradientFill(collarGrad);
    g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour(juce::Colour(0xff3f5270));
    g.drawEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, 1.2f);

    // Illuminated Outer Radial Ticks
    const int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        float tickAngle = rotaryStartAngle + (i / (float)(numTicks - 1)) * (rotaryEndAngle - rotaryStartAngle);
        float innerR = radius - 3.5f;
        float outerR = radius + 2.5f;
        float x1 = centre.x + innerR * std::sin(tickAngle);
        float y1 = centre.y - innerR * std::cos(tickAngle);
        float x2 = centre.x + outerR * std::sin(tickAngle);
        float y2 = centre.y - outerR * std::cos(tickAngle);

        bool isPassed = tickAngle <= toAngle;
        g.setColour(isPassed ? activeColour : juce::Colour(0xff2b384c));
        g.drawLine(x1, y1, x2, y2, isPassed ? 1.8f : 1.0f);
    }

    // 3. Draw Outer Track & Active Illuminated Arc
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(darkTrack);
    g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (slider.isEnabled())
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                               rotaryStartAngle, toAngle, true);

        // Hardware Emissive LED Bloom Glow
        g.setColour(activeColour.withAlpha(0.35f));
        g.strokePath(valueArc, juce::PathStrokeType(lineW + 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour(activeColour);
        g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // 4. Dark Machined Metal Inner Dial Cap
    auto innerCapRadius = radius * 0.65f;
    juce::ColourGradient capGrad(
        juce::Colour(0xff263244), centre.x - innerCapRadius * 0.7f, centre.y - innerCapRadius * 0.7f,
        juce::Colour(0xff090c12), centre.x + innerCapRadius * 0.7f, centre.y + innerCapRadius * 0.7f, false);
    g.setGradientFill(capGrad);
    g.fillEllipse(centre.x - innerCapRadius, centre.y - innerCapRadius, innerCapRadius * 2.0f, innerCapRadius * 2.0f);

    g.setColour(activeColour.withAlpha(0.65f));
    g.drawEllipse(centre.x - innerCapRadius, centre.y - innerCapRadius, innerCapRadius * 2.0f, innerCapRadius * 2.0f, 1.2f);

    // 5. Dial Indicator Notch Line
    juce::Path thumbTick;
    auto tickLen = innerCapRadius * 0.80f;
    thumbTick.startNewSubPath(centre);
    thumbTick.lineTo(centre.x + tickLen * std::sin(toAngle),
                     centre.y - tickLen * std::cos(toAngle));
    g.setColour(slider.isEnabled() ? activeColour : textColour.withAlpha(0.4f));
    g.strokePath(thumbTick, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void UndergroundLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                           int buttonX, int buttonY, int buttonW, int buttonH,
                                           juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);
    HardwareMaterials::drawRecessedPanel(g, bounds, 4.0f);

    g.setColour(cyanAccent.withAlpha(box.hasKeyboardFocus(true) ? 0.9f : 0.45f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.2f);

    // Draw arrow
    juce::Path arrow;
    auto arrowW = 8.0f;
    auto arrowH = 5.0f;
    auto arrowX = width - 16.0f;
    auto arrowY = height * 0.5f - arrowH * 0.5f;

    arrow.addTriangle(arrowX, arrowY,
                      arrowX + arrowW, arrowY,
                      arrowX + arrowW * 0.5f, arrowY + arrowH);
    g.setColour(cyanAccent);
    g.fillPath(arrow);
}

void UndergroundLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& backgroundColour,
                                                  bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

    // 3D Tactile Push Button
    juce::ColourGradient btnGrad(
        shouldDrawButtonAsDown ? juce::Colour(0xff090c12) : juce::Colour(0xff222c3d), bounds.getX(), bounds.getY(),
        shouldDrawButtonAsDown ? juce::Colour(0xff182230) : juce::Colour(0xff0c0f17), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(btnGrad);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(shouldDrawButtonAsHighlighted ? cyanAccent : juce::Colour(0xff34445c));
    g.drawRoundedRectangle(bounds, 4.0f, 1.2f);
}

void UndergroundLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.setColour(juce::Colour(0xff0c0f17));
    g.fillAll();
    g.setColour(cyanAccent.withAlpha(0.5f));
    g.drawRect(0, 0, width, height, 1);
}

juce::Font UndergroundLookAndFeel::getLabelFont(juce::Label& label)
{
    return juce::Font(12.0f, juce::Font::bold);
}

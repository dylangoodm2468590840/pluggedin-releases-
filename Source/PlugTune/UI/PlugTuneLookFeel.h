#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace PlugTuneUI
{

class PlugTuneLookFeel : public juce::LookAndFeel_V4
{
public:
    PlugTuneLookFeel();
    ~PlugTuneLookFeel() override = default;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    // Palette
    static juce::Colour getDarkBg()        { return juce::Colour(0xff0d0e12); }
    static juce::Colour getCardBg()        { return juce::Colour(0xff161820); }
    static juce::Colour getCardBorder()    { return juce::Colour(0xff252936); }
    static juce::Colour getNeonGreen()     { return juce::Colour(0xff00ff88); }
    static juce::Colour getElectricCyan()  { return juce::Colour(0xff00d4ff); }
    static juce::Colour getKnobBody()      { return juce::Colour(0xff222530); }
    static juce::Colour getTextColor()     { return juce::Colour(0xffe2e8f0); }
    static juce::Colour getTextDim()       { return juce::Colour(0xff8492a6); }
};

} // namespace PlugTuneUI

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class UndergroundLookAndFeel : public juce::LookAndFeel_V4
{
public:
    UndergroundLookAndFeel();
    ~UndergroundLookAndFeel() override;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    juce::Font getLabelFont(juce::Label& label) override;

private:
    juce::Colour bgColour       { 0xff0b0c10 };
    juce::Colour cardBgColour   { 0xff12151c };
    juce::Colour cyanAccent     { 0xff00f0ff };
    juce::Colour magentaAccent  { 0xffff0055 };
    juce::Colour degenerateGreen{ 0xff00ff66 };
    juce::Colour textColour     { 0xffe0e6ed };
    juce::Colour darkTrack      { 0xff1e2330 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndergroundLookAndFeel)
};

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
    RackUnitLookAndFeel
    ===================
    A custom JUCE LookAndFeel_V4 class designed for 3D dark industrial hardware rack interfaces.
*/
class RackUnitLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RackUnitLookAndFeel();
    ~RackUnitLookAndFeel() override;

    // 1. Custom Rotary Slider (3D Brushed Metal Knob with LED Glow Pointer)
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    // 2. Custom Toggle & Text Buttons (Dark Metallic States with Neon Green Glow)
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    // 3. Custom Dropdown ComboBox & Popup Menu
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    // 4. Custom Hardware Panel & Rivet Helper Drawing Methods
    static void drawRecessedPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius = 6.0f);
    static void drawMetallicRivet(juce::Graphics& g, float x, float y, float radius = 4.0f);
    static void drawTrapezoidHeader(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& titleText);
    static void drawSegmentedLEDMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float normalizedLevel, int numSegments = 14);
    static void drawEQDisplayBackground(juce::Graphics& g, juce::Rectangle<float> bounds);

private:
    juce::Colour darkCharcoal   { 0xff141619 };
    juce::Colour panelBevelLight{ 0xff3a3f47 };
    juce::Colour panelBevelDark { 0xff0a0b0c };
    juce::Colour neonGreen      { 0xff00ff66 };
    juce::Colour cyberCyan      { 0xff00f0ff };
    juce::Colour textLight      { 0xffe0e6ed };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RackUnitLookAndFeel)
};

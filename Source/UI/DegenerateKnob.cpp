#include "DegenerateKnob.h"

DegenerateKnob::DegenerateKnob()
{
    setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    setComponentID("DEGENERATE");
}

DegenerateKnob::~DegenerateKnob() = default;

void DegenerateKnob::paint(juce::Graphics& g)
{
    // 1. Draw ultra-high detail 3D hardware knob via LookAndFeel
    juce::Slider::paint(g);

    // 2. Render clean glowing neon green percentage text below dial center
    auto bounds = getLocalBounds().toFloat();
    int pct = juce::roundToInt(getValue() * 100.0);
    juce::String valStr = juce::String(pct) + "%";

    auto textBounds = bounds.removeFromBottom(20.0f);

    // Text Shadow for contrast against chassis
    g.setColour(juce::Colours::black.withAlpha(0.85f));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(valStr, textBounds.translated(1.0f, 1.0f), juce::Justification::centred, false);

    // Glowing Neon Green Percentage Text (#00FF66)
    g.setColour(juce::Colour(0xff00ff66));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(valStr, textBounds, juce::Justification::centred, false);
}


#include "RackUnitLookAndFeel.h"
#include <cmath>

RackUnitLookAndFeel::RackUnitLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId, neonGreen);
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1e232a));
    setColour(juce::ComboBox::backgroundColourId, darkCharcoal);
    setColour(juce::ComboBox::outlineColourId, cyberCyan);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff0d0f12));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff1d2430));
    setColour(juce::PopupMenu::highlightedTextColourId, neonGreen);
}

RackUnitLookAndFeel::~RackUnitLookAndFeel() = default;

// ==============================================================================
// 1. CUSTOM KNOB RENDERING (drawRotarySlider)
// ==============================================================================
void RackUnitLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPosProportional, float rotaryStartAngle,
                                            float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(3.0f);
    float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f - 2.0f;
    float centreX = bounds.getCentreX();
    float centreY = bounds.getCentreY();

    bool isDegenerate = (slider.getComponentID() == "DEGENERATE");
    juce::Colour neonGreenCol(0xff00ff66);
    juce::Colour activeAccent = isDegenerate ? neonGreenCol : juce::Colour(0xff00f0ff);

    float currentAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // ==============================================================================
    // A. DEEP CAST DROP SHADOW UNDERNEATH KNOB BODY
    // ==============================================================================
    float shadowOffset = std::max(2.0f, radius * 0.08f);
    g.setColour(juce::Colour(0xff000000).withAlpha(0.55f));
    g.fillEllipse(centreX - radius + shadowOffset * 0.6f, centreY - radius + shadowOffset * 1.2f, radius * 2.0f, radius * 2.0f);
    g.setColour(juce::Colour(0xff030406).withAlpha(0.75f));
    g.fillEllipse(centreX - radius + shadowOffset * 0.3f, centreY - radius + shadowOffset * 0.6f, radius * 2.0f, radius * 2.0f);

    // ==============================================================================
    // B. RADIAL SCALE TICKS & ACTIVE ILLUMINATED ARC
    // ==============================================================================
    const int numTicks = 11;
    for (int i = 0; i < numTicks; ++i)
    {
        float tickAngle = rotaryStartAngle + (float)i / (float)(numTicks - 1) * (rotaryEndAngle - rotaryStartAngle);
        float innerR = radius - 1.0f;
        float outerR = radius + 3.2f;

        float tx1 = centreX + innerR * std::sin(tickAngle);
        float ty1 = centreY - innerR * std::cos(tickAngle);
        float tx2 = centreX + outerR * std::sin(tickAngle);
        float ty2 = centreY - outerR * std::cos(tickAngle);

        bool isPassed = tickAngle <= currentAngle + 0.005f;
        g.setColour(isPassed ? activeAccent : juce::Colour(0xff3a485a));
        g.drawLine(tx1, ty1, tx2, ty2, isPassed ? 1.8f : 1.0f);
    }

    // Active Value Arc Glow around scale
    juce::Path activeArc;
    activeArc.addCentredArc(centreX, centreY, radius - 1.0f, radius - 1.0f, 0.0f,
                            rotaryStartAngle, currentAngle, true);
    g.setColour(activeAccent.withAlpha(0.25f));
    g.strokePath(activeArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(activeAccent);
    g.strokePath(activeArc, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // ==============================================================================
    // C. 24 RADIAL DIAMOND KNURLED TEETH AROUND OUTER PERIMETER COLLAR
    // ==============================================================================
    float collarRadius = radius * 0.94f;
    float collarInnerRadius = radius * 0.76f;
    const int numTeeth = 24;
    const float twoPi = juce::MathConstants<float>::twoPi;
    const float toothAngleStep = twoPi / (float)numTeeth;
    const float halfToothWidth = toothAngleStep * 0.42f;
    const float keyLightAngle = -juce::MathConstants<float>::pi * 0.25f; // Overhead top-left key light

    // Dark base collar disk
    juce::ColourGradient collarBaseGrad(
        juce::Colour(0xff2a3444), centreX - collarRadius, centreY - collarRadius,
        juce::Colour(0xff0b0e14), centreX + collarRadius, centreY + collarRadius, false);
    g.setGradientFill(collarBaseGrad);
    g.fillEllipse(centreX - collarRadius, centreY - collarRadius, collarRadius * 2.0f, collarRadius * 2.0f);

    for (int t = 0; t < numTeeth; ++t)
    {
        float toothAngle = t * toothAngleStep;

        float xOut = centreX + collarRadius * std::sin(toothAngle);
        float yOut = centreY - collarRadius * std::cos(toothAngle);

        float xLeft = centreX + collarInnerRadius * std::sin(toothAngle - halfToothWidth);
        float yLeft = centreY - collarInnerRadius * std::cos(toothAngle - halfToothWidth);

        float xRight = centreX + collarInnerRadius * std::sin(toothAngle + halfToothWidth);
        float yRight = centreY - collarInnerRadius * std::cos(toothAngle + halfToothWidth);

        float xValley = centreX + (collarInnerRadius - 1.2f) * std::sin(toothAngle);
        float yValley = centreY - (collarInnerRadius - 1.2f) * std::cos(toothAngle);

        // 3D Specular shading based on tooth orientation vs key light
        float normalLeft = toothAngle - halfToothWidth * 0.5f;
        float normalRight = toothAngle + halfToothWidth * 0.5f;

        float lightLeft = 0.5f + 0.5f * std::cos(normalLeft - keyLightAngle);
        float lightRight = 0.5f + 0.5f * std::cos(normalRight - keyLightAngle);

        lightLeft = std::clamp(lightLeft, 0.0f, 1.0f);
        lightRight = std::clamp(lightRight, 0.0f, 1.0f);

        juce::Colour leftCol = juce::Colour(0xff121820).interpolatedWith(juce::Colour(0xff9ab0c7), lightLeft);
        juce::Colour rightCol = juce::Colour(0xff080b0f).interpolatedWith(juce::Colour(0xff5c6e84), lightRight);

        // Left Face
        juce::Path leftFace;
        leftFace.startNewSubPath(xLeft, yLeft);
        leftFace.lineTo(xOut, yOut);
        leftFace.lineTo(xValley, yValley);
        leftFace.closeSubPath();
        g.setColour(leftCol);
        g.fillPath(leftFace);

        // Right Face
        juce::Path rightFace;
        rightFace.startNewSubPath(xOut, yOut);
        rightFace.lineTo(xRight, yRight);
        rightFace.lineTo(xValley, yValley);
        rightFace.closeSubPath();
        g.setColour(rightCol);
        g.fillPath(rightFace);

        // Diamond Ridge Highlight
        g.setColour(juce::Colour(0xffd0e2f5).withAlpha(0.15f + 0.50f * lightLeft));
        g.drawLine(xValley, yValley, xOut, yOut, 0.9f);
    }

    // Outer Beveled Edge of Knurled Collar
    g.setColour(juce::Colour(0xff7589a3).withAlpha(0.60f));
    g.drawEllipse(centreX - collarRadius, centreY - collarRadius, collarRadius * 2.0f, collarRadius * 2.0f, 1.2f);
    g.setColour(juce::Colour(0xff06080b).withAlpha(0.85f));
    g.drawEllipse(centreX - collarInnerRadius, centreY - collarInnerRadius, collarInnerRadius * 2.0f, collarInnerRadius * 2.0f, 1.0f);

    // ==============================================================================
    // D. MULTI-STOP ANISOTROPIC RADIAL BRUSHED METAL GRADIENT ON KNOB CAP
    // ==============================================================================
    float capRadius = collarInnerRadius * 0.88f;

    // Multi-stop radial base gradient (centered slightly top-left for 3D key light)
    float gradCX = centreX - capRadius * 0.25f;
    float gradCY = centreY - capRadius * 0.25f;

    juce::ColourGradient capGrad(
        juce::Colour(0xff627288), gradCX, gradCY,
        juce::Colour(0xff0c0f14), centreX + capRadius, centreY + capRadius, true);
    capGrad.addColour(0.20, juce::Colour(0xff404c5c));
    capGrad.addColour(0.50, juce::Colour(0xff222a36));
    capGrad.addColour(0.80, juce::Colour(0xff141a24));
    capGrad.addColour(0.95, juce::Colour(0xff0f131a));

    g.setGradientFill(capGrad);
    g.fillEllipse(centreX - capRadius, centreY - capRadius, capRadius * 2.0f, capRadius * 2.0f);

    // Anisotropic Specular Highlight Cones (Dual hourglass light reflection across cap)
    auto drawSpecularCone = [&](float centralAngle, float spreadAngle) {
        juce::Path cone;
        cone.startNewSubPath(centreX, centreY);
        cone.addArc(centreX - capRadius, centreY - capRadius,
                    capRadius * 2.0f, capRadius * 2.0f,
                    centralAngle - spreadAngle, centralAngle + spreadAngle, true);
        cone.closeSubPath();

        juce::ColourGradient specGrad(
            juce::Colours::white.withAlpha(0.26f), centreX, centreY,
            juce::Colours::white.withAlpha(0.0f), centreX + capRadius * std::cos(centralAngle), centreY + capRadius * std::sin(centralAngle), false);
        specGrad.addColour(0.6, juce::Colours::white.withAlpha(0.10f));

        g.setGradientFill(specGrad);
        g.fillPath(cone);
    };

    drawSpecularCone(-juce::MathConstants<float>::pi * 0.25f, juce::MathConstants<float>::pi * 0.22f);
    drawSpecularCone( juce::MathConstants<float>::pi * 0.75f, juce::MathConstants<float>::pi * 0.22f);

    // Concentric Turned Metal Micro Grooves
    const int numGrooves = 5;
    for (int gIdx = 1; gIdx <= numGrooves; ++gIdx)
    {
        float grooveR = capRadius * (0.18f + 0.76f * (gIdx / (float)numGrooves));
        g.setColour(juce::Colours::white.withAlpha(0.04f));
        g.drawEllipse(centreX - grooveR, centreY - grooveR, grooveR * 2.0f, grooveR * 2.0f, 0.6f);
        g.setColour(juce::Colours::black.withAlpha(0.06f));
        g.drawEllipse(centreX - grooveR + 0.5f, centreY - grooveR + 0.5f, grooveR * 2.0f, grooveR * 2.0f, 0.6f);
    }

    // Beveled Inner Rim & Specular Ring
    g.setColour(juce::Colour(0xff7a8d9e).withAlpha(0.70f));
    g.drawEllipse(centreX - capRadius, centreY - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.2f);

    g.setColour(juce::Colour(0xff06080b).withAlpha(0.85f));
    g.drawEllipse(centreX - capRadius + 1.0f, centreY - capRadius + 1.0f, (capRadius - 1.0f) * 2.0f, (capRadius - 1.0f) * 2.0f, 1.0f);

    // ==============================================================================
    // E. GLOWING NEON GREEN INDICATOR NEEDLE (#00FF66)
    // ==============================================================================
    float needleStartR = capRadius * 0.18f;
    float needleEndR = capRadius * 0.94f;

    float nx1 = centreX + needleStartR * std::sin(currentAngle);
    float ny1 = centreY - needleStartR * std::cos(currentAngle);
    float nx2 = centreX + needleEndR * std::sin(currentAngle);
    float ny2 = centreY - needleEndR * std::cos(currentAngle);

    // Outer Neon Green Emissive Bloom
    g.setColour(neonGreenCol.withAlpha(0.40f));
    g.drawLine(nx1, ny1, nx2, ny2, std::max(4.2f, radius * 0.11f));

    // Core Neon Green Line
    g.setColour(neonGreenCol);
    g.drawLine(nx1, ny1, nx2, ny2, std::max(2.2f, radius * 0.055f));

    // White Specular Core
    g.setColour(juce::Colours::white.withAlpha(0.92f));
    g.drawLine(nx1, ny1, nx2, ny2, 1.0f);

    // Tip Glowing Bead
    float dotR = std::max(2.0f, radius * 0.045f);
    g.setColour(neonGreenCol);
    g.fillEllipse(nx2 - dotR, ny2 - dotR, dotR * 2.0f, dotR * 2.0f);
    g.setColour(juce::Colours::white);
    g.fillEllipse(nx2 - dotR * 0.5f, ny2 - dotR * 0.5f, dotR, dotR);

    // Central Metallic Cap Rivet / Hub
    float hubR = std::max(3.5f, capRadius * 0.22f);
    juce::ColourGradient hubGrad(
        juce::Colour(0xff4a586c), centreX - hubR * 0.4f, centreY - hubR * 0.4f,
        juce::Colour(0xff0f141d), centreX + hubR * 0.5f, centreY + hubR * 0.5f, true);
    g.setGradientFill(hubGrad);
    g.fillEllipse(centreX - hubR, centreY - hubR, hubR * 2.0f, hubR * 2.0f);
    g.setColour(neonGreenCol.withAlpha(0.85f));
    g.drawEllipse(centreX - hubR, centreY - hubR, hubR * 2.0f, hubR * 2.0f, 1.0f);
}

// ==============================================================================
// 2. FRAME & PANEL INSETS + METALLIC RIVETS
// ==============================================================================
void RackUnitLookAndFeel::drawRecessedPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float cornerRadius)
{
    // Dark Charcoal Recessed Background (#141619)
    g.setColour(juce::Colour(0xff141619));
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Top/Left Bright Highlight Border (#3a3f47)
    g.setColour(juce::Colour(0xff3a3f47));
    g.drawRoundedRectangle(bounds, cornerRadius, 1.2f);

    // Bottom/Right Dark Shadow Border (#0a0b0c)
    g.setColour(juce::Colour(0xff0a0b0c));
    g.drawRoundedRectangle(bounds.reduced(1.0f), cornerRadius, 1.0f);
}

void RackUnitLookAndFeel::drawMetallicRivet(juce::Graphics& g, float x, float y, float radius)
{
    // Drop Shadow
    g.setColour(juce::Colour(0xff050608).withAlpha(0.80f));
    g.fillEllipse(x - radius + 1.0f, y - radius + 1.0f, radius * 2.0f, radius * 2.0f);

    // Circular Radial Gradient Metallic Head
    juce::ColourGradient rivetGrad(
        juce::Colour(0xff5e6b7c), x - radius * 0.4f, y - radius * 0.4f,
        juce::Colour(0xff12161e), x + radius * 0.5f, y + radius * 0.5f, true);
    g.setGradientFill(rivetGrad);
    g.fillEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f);

    g.setColour(juce::Colour(0xff7c8d9e));
    g.drawEllipse(x - radius, y - radius, radius * 2.0f, radius * 2.0f, 0.8f);

    // Center Slot Line
    g.setColour(juce::Colour(0xff080a0d));
    g.drawLine(x - radius * 0.5f, y, x + radius * 0.5f, y, 1.0f);
}

// ==============================================================================
// 3. TRAPEZOID HEADER & DEGENERATE SECTION
// ==============================================================================
void RackUnitLookAndFeel::drawTrapezoidHeader(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& titleText)
{
    juce::Path trapPath;
    float inset = bounds.getWidth() * 0.15f;
    trapPath.startNewSubPath(bounds.getX(), bounds.getBottom());
    trapPath.lineTo(bounds.getX() + inset, bounds.getY());
    trapPath.lineTo(bounds.getRight() - inset, bounds.getY());
    trapPath.lineTo(bounds.getRight(), bounds.getBottom());
    trapPath.closeSubPath();

    // Metallic Fill
    juce::ColourGradient trapGrad(
        juce::Colour(0xff222a36), bounds.getX(), bounds.getY(),
        juce::Colour(0xff0e131b), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(trapGrad);
    g.fillPath(trapPath);

    // Raised Metallic Border
    g.setColour(juce::Colour(0xff00ff66).withAlpha(0.70f));
    g.strokePath(trapPath, juce::PathStrokeType(1.8f));

    // Screws on top corners
    drawMetallicRivet(g, bounds.getX() + inset + 10.0f, bounds.getY() + 10.0f, 3.5f);
    drawMetallicRivet(g, bounds.getRight() - inset - 10.0f, bounds.getY() + 10.0f, 3.5f);

    // Header Title
    g.setColour(juce::Colour(0xff00ff66));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(titleText, bounds, juce::Justification::centred, true);
}

// ==============================================================================
// 4. LED METERS & EQ BACKGROUND
// ==============================================================================
void RackUnitLookAndFeel::drawSegmentedLEDMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float normalizedLevel, int numSegments)
{
    float segmentH = (bounds.getHeight() - (numSegments - 1) * 1.5f) / numSegments;
    int activeSegments = juce::roundToInt(std::clamp(normalizedLevel, 0.0f, 1.0f) * numSegments);

    for (int i = 0; i < numSegments; ++i)
    {
        float segY = bounds.getY() + (numSegments - 1 - i) * (segmentH + 1.5f);
        bool isActive = i < activeSegments;

        // Color Spectrum: Green -> Yellow -> Orange -> Red
        juce::Colour segColour = (i >= numSegments - 2) ? juce::Colour(0xffff0055) : // Red
                                 (i >= numSegments - 5) ? juce::Colour(0xffffa800) : // Orange
                                 (i >= numSegments - 8) ? juce::Colour(0xffffcc00) : // Yellow
                                                          juce::Colour(0xff00ff66);  // Green

        g.setColour(isActive ? segColour : segColour.withAlpha(0.12f));
        g.fillRect(bounds.getX(), segY, bounds.getWidth(), segmentH);
    }
}

void RackUnitLookAndFeel::drawEQDisplayBackground(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Dark Slate Grid Background
    g.setColour(juce::Colour(0xff0a0d12));
    g.fillRoundedRectangle(bounds, 6.0f);

    // 1px Glowing Cyan Border
    g.setColour(juce::Colour(0xff00f0ff).withAlpha(0.65f));
    g.drawRoundedRectangle(bounds, 6.0f, 1.2f);
}

// ==============================================================================
// 5. SLIDERS & BUTTONS
// ==============================================================================
void RackUnitLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& backgroundColour,
                                                bool shouldDrawButtonAsHighlighted,
                                                bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    float cornerRadius = 4.0f;

    bool isOn = button.getToggleState();

    // Dark Metal State
    juce::Colour fillCol = shouldDrawButtonAsDown ? juce::Colour(0xff090c10) :
                           isOn                  ? juce::Colour(0xff121b24) :
                           shouldDrawButtonAsHighlighted ? juce::Colour(0xff1e2633) : juce::Colour(0xff141820);

    g.setColour(fillCol);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Border
    juce::Colour borderCol = isOn ? neonGreen : (shouldDrawButtonAsHighlighted ? cyberCyan : juce::Colour(0xff2d3645));
    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.2f);
}

void RackUnitLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                          bool shouldDrawButtonAsHighlighted,
                                          bool shouldDrawButtonAsDown)
{
    bool isOn = button.getToggleState();
    juce::Colour textCol = isOn ? neonGreen : (shouldDrawButtonAsHighlighted ? cyberCyan : textLight);

    g.setColour(textCol);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred, true);
}

void RackUnitLookAndFeel::drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool)
{
    // Intentionally empty: hides default JUCE checkbox graphics ([x])
    // The glowing Ruby Red LED is custom-rendered in PluginEditor::paint!
}

void RackUnitLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                        int buttonX, int buttonY, int buttonW, int buttonH,
                                        juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)width, (float)height);
    drawRecessedPanel(g, bounds, 4.0f);

    // Dropdown Arrow
    juce::Path arrow;
    float arrowX = width - 16.0f;
    float arrowY = height * 0.5f - 2.0f;
    arrow.addTriangle(arrowX, arrowY, arrowX + 8.0f, arrowY, arrowX + 4.0f, arrowY + 5.0f);

    g.setColour(cyberCyan);
    g.fillPath(arrow);
}

void RackUnitLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    g.fillAll(juce::Colour(0xff0d0f12));
    g.setColour(cyberCyan.withAlpha(0.5f));
    g.drawRect(0, 0, width, height, 1);
}

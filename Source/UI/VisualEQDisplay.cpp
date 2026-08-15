#include "VisualEQDisplay.h"
#include "HardwareMaterials.h"
#include "../Utils/ParameterIDs.h"
#include <cmath>

VisualEQDisplay::VisualEQDisplay()
{
    std::fill(std::begin(spectrumBars), std::end(spectrumBars), 0.05f);

    // Initial default active nodes (2 starter nodes: Low Bell & Mid Bell)
    dynamicNodes.push_back({ 200.0f, 0.0f, 0.707f, 0, 0, true });
    dynamicNodes.push_back({ 2200.0f, 0.0f, 0.707f, 0, 0, true });
    dynamicNodes.push_back({ 8000.0f, 0.0f, 0.707f, 0, 0, true });
}

VisualEQDisplay::~VisualEQDisplay() = default;

void VisualEQDisplay::updateEQState(const float* realFFTSpectrum64,
                                     float lowCutFreq,
                                     float lowGainDb,
                                     float midGainDb,
                                     float highGainDb,
                                     float highCutFreq,
                                     float lowQVal,
                                     float midQVal,
                                     float highQVal,
                                     int lowCutSlopeVal,
                                     int highCutSlopeVal)
{
    if (realFFTSpectrum64 != nullptr)
    {
        for (int i = 0; i < 64; ++i)
            spectrumBars[i] = spectrumBars[i] * 0.70f + realFFTSpectrum64[i] * 0.30f;
    }

    if (activeDraggedNode < 0)
    {
        lowCut       = lowCutFreq;
        lowGain      = lowGainDb;
        midGain      = midGainDb;
        highGain     = highGainDb;
        highCut      = highCutFreq;
        lowQ         = lowQVal;
        midQ         = midQVal;
        highQ        = highQVal;
        lowCutSlope  = lowCutSlopeVal;
        highCutSlope = highCutSlopeVal;

        if (dynamicNodes.size() >= 3)
        {
            dynamicNodes[0].gainDb = lowGain;
            dynamicNodes[0].qFactor = lowQ;
            dynamicNodes[1].gainDb = midGain;
            dynamicNodes[1].qFactor = midQ;
            dynamicNodes[2].gainDb = highGain;
            dynamicNodes[2].qFactor = highQ;
        }
    }

    repaint();
}

void VisualEQDisplay::mouseDown(const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds().toFloat();

    // Check click on Floating Node HUD Widget Card (Filter Shape & Stereo Mode Toggles)
    if (selectedNode >= 0 && selectedNode < (int)dynamicNodes.size() && dynamicNodes[selectedNode].active)
    {
        auto hudCard = juce::Rectangle<float>(bounds.getRight() - 250.0f, bounds.getY() + 8.0f, 240.0f, 54.0f);
        if (hudCard.contains(e.position))
        {
            // Row 1: Filter Shape Buttons [ BELL ] [ LOW CUT ] [ HIGH CUT ] [ LOW SHELF ] [ HIGH SHELF ] [ NOTCH ]
            float shapeBtnY = hudCard.getY() + 20.0f;
            float shapeBtnW = 36.0f;
            float shapeBtnH = 13.0f;

            for (int shape = 0; shape < 6; ++shape)
            {
                auto sBtn = juce::Rectangle<float>(hudCard.getX() + 4.0f + shape * (shapeBtnW + 3.0f), shapeBtnY, shapeBtnW, shapeBtnH);
                if (sBtn.contains(e.position))
                {
                    dynamicNodes[selectedNode].filterType = shape;
                    repaint();
                    return;
                }
            }

            // Row 2: Stereo Mode Buttons [ STEREO ] [ MID ] [ SIDE ]
            float modeBtnY = hudCard.getY() + 36.0f;
            float modeBtnW = 42.0f;
            float modeBtnH = 13.0f;

            for (int mode = 0; mode < 3; ++mode)
            {
                auto mBtn = juce::Rectangle<float>(hudCard.getX() + 4.0f + mode * (modeBtnW + 4.0f), modeBtnY, modeBtnW, modeBtnH);
                if (mBtn.contains(e.position))
                {
                    dynamicNodes[selectedNode].stereoMode = mode;
                    repaint();
                    return;
                }
            }
        }
    }

    // Check click on bottom frequency ruler for timeline dragging/panning
    if (e.position.y >= bounds.getBottom() - 22.0f)
    {
        isDraggingRuler = true;
        lastRulerX = e.position.x;
        return;
    }

    auto freqToNormX = [&](float freqHz) {
        float minLog = std::log10(minFreqHz);
        float maxLog = std::log10(maxFreqHz);
        return (std::log10(std::clamp(freqHz, minFreqHz, maxFreqHz)) - minLog) / (maxLog - minLog);
    };
    auto dbToNormY = [&](float db) {
        return bounds.getCentreY() - (db / currentDbRange) * (bounds.getHeight() * 0.42f);
    };

    activeDraggedNode = -1;
    float closestDist = 28.0f;

    for (int n = 0; n < (int)dynamicNodes.size(); ++n)
    {
        if (!dynamicNodes[n].active) continue;

        float nx = bounds.getX() + freqToNormX(dynamicNodes[n].freqHz) * bounds.getWidth();
        float ny = dbToNormY(dynamicNodes[n].gainDb);

        float dist = std::hypot(e.position.x - nx, e.position.y - ny);
        if (dist < closestDist)
        {
            closestDist = dist;
            activeDraggedNode = n;
            selectedNode = n;
        }
    }

    if (activeDraggedNode >= 0 && activeDraggedNode < 3 && apvts != nullptr)
    {
        const char* paramIDs[] = { ParameterIDs::EQ_LOW_GAIN, ParameterIDs::EQ_MID_GAIN, ParameterIDs::EQ_HIGH_GAIN };
        if (auto* param = apvts->getParameter(paramIDs[activeDraggedNode]))
            param->beginChangeGesture();
    }
}

void VisualEQDisplay::mouseDrag(const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds().toFloat();

    // Timeline Frequency Ruler Drag Panning
    if (isDraggingRuler)
    {
        float dx = e.position.x - lastRulerX;
        lastRulerX = e.position.x;

        float rangeLog = std::log10(maxFreqHz) - std::log10(minFreqHz);
        float deltaLog = -(dx / bounds.getWidth()) * rangeLog;

        float newMinLog = std::clamp(std::log10(minFreqHz) + deltaLog, std::log10(20.0f), std::log10(18000.0f));
        float newMaxLog = std::clamp(newMinLog + rangeLog, std::log10(100.0f), std::log10(20000.0f));

        minFreqHz = std::pow(10.0f, newMinLog);
        maxFreqHz = std::pow(10.0f, newMaxLog);
        repaint();
        return;
    }

    if (activeDraggedNode < 0 || activeDraggedNode >= (int)dynamicNodes.size())
        return;

    auto normXToFreq = [&](float normX) {
        float minLog = std::log10(minFreqHz);
        float maxLog = std::log10(maxFreqHz);
        float logFreq = minLog + std::clamp(normX, 0.0f, 1.0f) * (maxLog - minLog);
        return std::pow(10.0f, logFreq);
    };

    auto normYToDb = [&](float normY) {
        float relY = (bounds.getCentreY() - normY) / (bounds.getHeight() * 0.42f);
        return std::clamp(relY * currentDbRange, -currentDbRange, currentDbRange);
    };

    float normX = (e.position.x - bounds.getX()) / bounds.getWidth();
    float normY = e.position.y;

    dynamicNodes[activeDraggedNode].freqHz = std::clamp(normXToFreq(normX), 20.0f, 20000.0f);
    dynamicNodes[activeDraggedNode].gainDb = normYToDb(normY);

    if (apvts != nullptr)
    {
        auto setParamVal = [&](const char* id, float val) {
            if (auto* param = apvts->getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(val));
        };

        if (activeDraggedNode == 0) setParamVal(ParameterIDs::EQ_LOW_GAIN, dynamicNodes[0].gainDb);
        if (activeDraggedNode == 1) setParamVal(ParameterIDs::EQ_MID_GAIN, dynamicNodes[1].gainDb);
        if (activeDraggedNode == 2) setParamVal(ParameterIDs::EQ_HIGH_GAIN, dynamicNodes[2].gainDb);
    }

    repaint();
}

void VisualEQDisplay::mouseUp(const juce::MouseEvent&)
{
    isDraggingRuler = false;

    if (activeDraggedNode >= 0 && activeDraggedNode < 3 && apvts != nullptr)
    {
        const char* paramIDs[] = { ParameterIDs::EQ_LOW_GAIN, ParameterIDs::EQ_MID_GAIN, ParameterIDs::EQ_HIGH_GAIN };
        if (auto* param = apvts->getParameter(paramIDs[activeDraggedNode]))
            param->endChangeGesture();
    }
    activeDraggedNode = -1;
}

void VisualEQDisplay::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds().toFloat();

    // Double click on bottom ruler: Reset Timeline Frequency Zoom to 20Hz - 20kHz!
    if (e.position.y >= bounds.getBottom() - 22.0f)
    {
        minFreqHz = 20.0f;
        maxFreqHz = 20000.0f;
        repaint();
        return;
    }

    auto freqToNormX = [&](float freqHz) {
        float minLog = std::log10(minFreqHz);
        float maxLog = std::log10(maxFreqHz);
        return (std::log10(std::clamp(freqHz, minFreqHz, maxFreqHz)) - minLog) / (maxLog - minLog);
    };
    auto dbToNormY = [&](float db) {
        return bounds.getCentreY() - (db / currentDbRange) * (bounds.getHeight() * 0.42f);
    };

    auto normXToFreq = [&](float normX) {
        float minLog = std::log10(minFreqHz);
        float maxLog = std::log10(maxFreqHz);
        float logFreq = minLog + std::clamp(normX, 0.0f, 1.0f) * (maxLog - minLog);
        return std::pow(10.0f, logFreq);
    };

    auto normYToDb = [&](float normY) {
        float relY = (bounds.getCentreY() - normY) / (bounds.getHeight() * 0.42f);
        return std::clamp(relY * currentDbRange, -currentDbRange, currentDbRange);
    };

    int clickedNode = -1;
    float closestDist = 25.0f;

    for (int n = 0; n < (int)dynamicNodes.size(); ++n)
    {
        if (!dynamicNodes[n].active) continue;
        float nx = bounds.getX() + freqToNormX(dynamicNodes[n].freqHz) * bounds.getWidth();
        float ny = dbToNormY(dynamicNodes[n].gainDb);

        float dist = std::hypot(e.position.x - nx, e.position.y - ny);
        if (dist < closestDist)
        {
            closestDist = dist;
            clickedNode = n;
        }
    }

    if (clickedNode >= 0)
    {
        if (dynamicNodes.size() > 1)
        {
            dynamicNodes.erase(dynamicNodes.begin() + clickedNode);
            selectedNode = std::clamp(clickedNode - 1, 0, (int)dynamicNodes.size() - 1);
        }
    }
    else
    {
        float normX = (e.position.x - bounds.getX()) / bounds.getWidth();
        float newFreq = normXToFreq(normX);
        float newGain = normYToDb(e.position.y);

        dynamicNodes.push_back({ newFreq, newGain, 1.0f, 0, 0, true });
        selectedNode = (int)dynamicNodes.size() - 1;
    }

    repaint();
}

void VisualEQDisplay::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Timeline Frequency Zoom: Hovering near bottom ruler OR holding Shift key
    bool isShiftDown = e.mods.isShiftDown();
    if (isShiftDown || e.position.y >= bounds.getBottom() - 32.0f)
    {
        float zoomStep = (wheel.deltaY > 0) ? 0.82f : 1.22f;
        float normHoverX = (e.position.x - bounds.getX()) / bounds.getWidth();
        float hoverLogFreq = std::log10(minFreqHz) + normHoverX * (std::log10(maxFreqHz) - std::log10(minFreqHz));

        float newRangeLog = (std::log10(maxFreqHz) - std::log10(minFreqHz)) * zoomStep;
        newRangeLog = std::clamp(newRangeLog, 0.18f, 3.0f);

        float newMinLog = hoverLogFreq - normHoverX * newRangeLog;
        float newMaxLog = newMinLog + newRangeLog;

        minFreqHz = std::clamp(std::pow(10.0f, newMinLog), 20.0f, 18000.0f);
        maxFreqHz = std::clamp(std::pow(10.0f, newMaxLog), 100.0f, 20000.0f);
        repaint();
        return;
    }

    // 2. Smooth Exponential Q Scaling for Target EQ Node
    auto freqToNormX = [&](float freqHz) {
        float minLog = std::log10(minFreqHz);
        float maxLog = std::log10(maxFreqHz);
        return (std::log10(std::clamp(freqHz, minFreqHz, maxFreqHz)) - minLog) / (maxLog - minLog);
    };
    auto dbToNormY = [&](float db) {
        return bounds.getCentreY() - (db / currentDbRange) * (bounds.getHeight() * 0.42f);
    };

    int targetNode = -1;
    float closestDist = 35.0f;

    for (int n = 0; n < (int)dynamicNodes.size(); ++n)
    {
        if (!dynamicNodes[n].active) continue;
        float nx = bounds.getX() + freqToNormX(dynamicNodes[n].freqHz) * bounds.getWidth();
        float ny = dbToNormY(dynamicNodes[n].gainDb);
        float dist = std::hypot(e.position.x - nx, e.position.y - ny);
        if (dist < closestDist)
        {
            closestDist = dist;
            targetNode = n;
        }
    }

    if (targetNode >= 0 && targetNode < (int)dynamicNodes.size())
    {
        selectedNode = targetNode;

        // Exponential Multiplicative Q Scaling (Silky smooth wide and ultra-thin cuts)
        float currentQ = dynamicNodes[targetNode].qFactor;
        float qScaleFactor = std::pow(1.20f, wheel.deltaY * 5.0f);
        dynamicNodes[targetNode].qFactor = std::clamp(currentQ * qScaleFactor, 0.10f, 50.0f);

        if (apvts != nullptr)
        {
            auto setQVal = [&](const char* id, float val) {
                if (auto* param = apvts->getParameter(id))
                    param->setValueNotifyingHost(param->convertTo0to1(val));
            };

            if (targetNode == 0) setQVal(ParameterIDs::EQ_LOW_Q, dynamicNodes[0].qFactor);
            if (targetNode == 1) setQVal(ParameterIDs::EQ_MID_Q, dynamicNodes[1].qFactor);
            if (targetNode == 2) setQVal(ParameterIDs::EQ_HIGH_Q, dynamicNodes[2].qFactor);
        }

        repaint();
    }
}

void VisualEQDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. CRT Glass Display Frame & Bezel
    HardwareMaterials::drawGlassDisplay(g, bounds, 6.0f);

    // 2. Logarithmic Frequency Gridlines & Timeline Navigation Labels
    g.setColour(juce::Colour(0xff182230));
    const float gridFreqsHz[] = { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    auto freqToNormX = [&](float freqHz) {
        float minLog = std::log10(minFreqHz);
        float maxLog = std::log10(maxFreqHz);
        return (std::log10(std::clamp(freqHz, minFreqHz, maxFreqHz)) - minLog) / (maxLog - minLog);
    };

    for (float fHz : gridFreqsHz)
    {
        if (fHz >= minFreqHz && fHz <= maxFreqHz)
        {
            float x = bounds.getX() + freqToNormX(fHz) * bounds.getWidth();
            g.drawVerticalLine((int)x, bounds.getY() + 10.0f, bounds.getBottom() - 10.0f);
        }
    }

    auto dbToNormY = [&](float db) {
        return bounds.getCentreY() - (db / currentDbRange) * (bounds.getHeight() * 0.42f);
    };

    float halfRange = currentDbRange * 0.5f;
    float dBGrid[] = { +currentDbRange, +halfRange, 0.0f, -halfRange, -currentDbRange };
    for (float db : dBGrid)
    {
        float y = dbToNormY(db);
        g.drawHorizontalLine((int)y, bounds.getX() + 10.0f, bounds.getRight() - 10.0f);
    }

    // Dynamic timeline scale labels
    g.setColour(juce::Colour(0xff4a5d73));
    g.setFont(juce::Font(8.5f, juce::Font::bold));
    if (100.0f >= minFreqHz && 100.0f <= maxFreqHz)
        g.drawText("100Hz", (int)(bounds.getX() + freqToNormX(100.0f) * bounds.getWidth()) - 15, (int)(bounds.getBottom() - 14), 30, 10, juce::Justification::centred);
    if (1000.0f >= minFreqHz && 1000.0f <= maxFreqHz)
        g.drawText("1kHz",  (int)(bounds.getX() + freqToNormX(1000.0f) * bounds.getWidth()) - 15, (int)(bounds.getBottom() - 14), 30, 10, juce::Justification::centred);
    if (10000.0f >= minFreqHz && 10000.0f <= maxFreqHz)
        g.drawText("10kHz", (int)(bounds.getX() + freqToNormX(10000.0f) * bounds.getWidth()) - 15, (int)(bounds.getBottom() - 14), 30, 10, juce::Justification::centred);

    g.drawText("+" + juce::String((int)currentDbRange) + "dB", 6, 8, 35, 10, juce::Justification::left);
    g.drawText("0dB",   6, (int)(bounds.getCentreY() - 5.0f), 24, 10, juce::Justification::left);
    g.drawText("-" + juce::String((int)currentDbRange) + "dB", 6, (int)(bounds.getBottom() - 14), 35, 10, juce::Justification::left);

    // 3. Render Real FFT Audio Spectrum Bars
    float barW = bounds.getWidth() / 64.0f;
    for (int i = 0; i < 64; ++i)
    {
        float x = bounds.getX() + i * barW;
        float h = bounds.getHeight() * 0.70f * spectrumBars[i];
        float y = bounds.getBottom() - 14.0f - h;

        g.setColour(juce::Colour(0xff00f0ff).withAlpha(0.22f));
        g.fillRect(x + 1.0f, y, barW - 1.5f, h);
    }

    // 4. Compute Combined Transfer Response ($H(f)$) across ALL Dynamic EQ Nodes with Shape Variations
    juce::Path curvePath;
    const int numPoints = 180;

    auto calcBandDb = [&](float freqHz) {
        float totalDb = 0.0f;

        for (const auto& node : dynamicNodes)
        {
            if (!node.active) continue;

            float qFactor = std::clamp(node.qFactor, 0.10f, 50.0f);

            // Filter Type: 0: Bell, 1: Low Cut (High Pass), 2: High Cut (Low Pass), 3: Low Shelf, 4: High Shelf, 5: Notch
            if (node.filterType == 1) // LOW CUT (High Pass)
            {
                if (freqHz < node.freqHz)
                {
                    float octaveDist = std::log2(node.freqHz / std::max(10.0f, freqHz));
                    totalDb -= octaveDist * 36.0f; // 36dB/oct slope cut!
                }
            }
            else if (node.filterType == 2) // HIGH CUT (Low Pass)
            {
                if (freqHz > node.freqHz)
                {
                    float octaveDist = std::log2(freqHz / node.freqHz);
                    totalDb -= octaveDist * 36.0f; // 36dB/oct slope cut!
                }
            }
            else if (node.filterType == 5) // NOTCH
            {
                float bandwidthOctaves = 1.0f / (qFactor * 0.707f);
                float bandwidthLog = bandwidthOctaves * 0.30103f;
                float logRatio = std::abs(std::log10(freqHz) - std::log10(std::clamp(node.freqHz, 20.0f, 20000.0f)));
                float normDist = logRatio / std::max(0.001f, bandwidthLog);
                totalDb -= 48.0f * std::exp(-normDist * normDist * 3.0f);
            }
            else // BELL / SHELF
            {
                float bandwidthOctaves = 1.0f / (qFactor * 0.707f);
                float bandwidthLog = bandwidthOctaves * 0.30103f;
                float logRatio = std::abs(std::log10(freqHz) - std::log10(std::clamp(node.freqHz, 20.0f, 20000.0f)));
                float normDist = logRatio / std::max(0.001f, bandwidthLog);
                totalDb += node.gainDb * std::exp(-normDist * normDist * 2.5f);
            }
        }

        return std::clamp(totalDb, -currentDbRange * 2.0f, currentDbRange * 2.0f);
    };

    for (int i = 0; i < numPoints; ++i)
    {
        float normX = (float)i / (float)(numPoints - 1);
        float logFreqHz = std::pow(10.0f, std::log10(minFreqHz) + normX * (std::log10(maxFreqHz) - std::log10(minFreqHz)));
        float x = bounds.getX() + normX * bounds.getWidth();
        float db = calcBandDb(logFreqHz);

        float y = dbToNormY(db);
        y = std::clamp(y, bounds.getY() + 6.0f, bounds.getBottom() - 6.0f);

        if (i == 0)
            curvePath.startNewSubPath(x, y);
        else
            curvePath.lineTo(x, y);
    }

    // 5. Render Filled Gradient under Curve
    juce::Path fillPath = curvePath;
    fillPath.lineTo(bounds.getRight(), bounds.getBottom() - 12.0f);
    fillPath.lineTo(bounds.getX(), bounds.getBottom() - 12.0f);
    fillPath.closeSubPath();

    juce::ColourGradient fillGrad(
        juce::Colour(0xff00f0ff).withAlpha(0.26f), bounds.getX(), bounds.getY(),
        juce::Colour(0xff00f0ff).withAlpha(0.02f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(fillGrad);
    g.fillPath(fillPath);

    // 6. Render Glowing Main Curve Line
    g.setColour(juce::Colour(0xff00f0ff));
    g.strokePath(curvePath, juce::PathStrokeType(2.5f));

    // 7. Render Interactive Frequency Node Handles (◯) with Filter Type Visual Indicators
    for (int n = 0; n < (int)dynamicNodes.size(); ++n)
    {
        if (!dynamicNodes[n].active) continue;

        float fHz = dynamicNodes[n].freqHz;
        if (fHz < minFreqHz || fHz > maxFreqHz) continue;

        float x = bounds.getX() + freqToNormX(fHz) * bounds.getWidth();
        float db = calcBandDb(fHz);
        float y = dbToNormY(db);

        bool isSelected = (n == selectedNode || n == activeDraggedNode);

        // Routing Color: 0: Stereo (Cyan/Green), 1: Mid (Amber Gold), 2: Side (Magenta Pink)
        juce::Colour nodeCoreCol = (dynamicNodes[n].stereoMode == 1) ? juce::Colour(0xffffa800) :
                                   (dynamicNodes[n].stereoMode == 2) ? juce::Colour(0xffff00aa) :
                                                                       juce::Colour(0xff00ff66);

        g.setColour(isSelected ? nodeCoreCol : juce::Colour(0xffff0055).withAlpha(0.40f));
        g.fillEllipse(x - (isSelected ? 9.0f : 7.0f), y - (isSelected ? 9.0f : 7.0f), isSelected ? 18.0f : 14.0f, isSelected ? 18.0f : 14.0f);

        g.setColour(nodeCoreCol);
        g.fillEllipse(x - 4.0f, y - 4.0f, 8.0f, 8.0f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(x - 2.0f, y - 2.0f, 4.0f, 4.0f);
    }

    // 8. Render Floating Node HUD Widget Card with Filter Shape Bar & Stereo Mode Toggles
    if (selectedNode >= 0 && selectedNode < (int)dynamicNodes.size() && dynamicNodes[selectedNode].active)
    {
        float selFreqHz = dynamicNodes[selectedNode].freqHz;
        float selGainDb = dynamicNodes[selectedNode].gainDb;
        float selQ      = dynamicNodes[selectedNode].qFactor;
        int shape       = dynamicNodes[selectedNode].filterType;
        int mode        = dynamicNodes[selectedNode].stereoMode;

        auto hudCard = juce::Rectangle<float>(bounds.getRight() - 250.0f, bounds.getY() + 8.0f, 240.0f, 54.0f);
        g.setColour(juce::Colour(0xef0a0f17));
        g.fillRoundedRectangle(hudCard, 5.0f);
        g.setColour(juce::Colour(0xff00f0ff).withAlpha(0.60f));
        g.drawRoundedRectangle(hudCard, 5.0f, 1.2f);

        g.setFont(juce::Font(8.5f, juce::Font::bold));
        g.setColour(juce::Colour(0xff00ff66));

        juce::String infoStr = "NODE #" + juce::String(selectedNode + 1) + " | F: " + juce::String((int)selFreqHz) + "Hz | G: " + juce::String(selGainDb, 1) + "dB | Q: " + juce::String(selQ, 2);
        g.drawText(infoStr, (int)hudCard.getX() + 4, (int)hudCard.getY() + 3, 230, 10, juce::Justification::left);

        // Row 1: Filter Shape Buttons [ BELL ] [ L-CUT ] [ H-CUT ] [ L-SHLF ] [ H-SHLF ] [ NOTCH ]
        float shapeBtnY = hudCard.getY() + 18.0f;
        float shapeBtnW = 36.0f;
        float shapeBtnH = 13.0f;
        juce::String shapeTitles[] = { "BELL", "L-CUT", "H-CUT", "L-SHLF", "H-SHLF", "NOTCH" };

        for (int s = 0; s < 6; ++s)
        {
            auto sBtn = juce::Rectangle<float>(hudCard.getX() + 4.0f + s * (shapeBtnW + 3.0f), shapeBtnY, shapeBtnW, shapeBtnH);
            bool isCurrent = (shape == s);

            g.setColour(isCurrent ? juce::Colour(0xff00f0ff) : juce::Colour(0xff121822));
            g.fillRoundedRectangle(sBtn, 2.0f);
            g.setColour(isCurrent ? juce::Colour(0xff00f0ff) : juce::Colour(0xff2d3b4d));
            g.drawRoundedRectangle(sBtn, 2.0f, 1.0f);

            g.setColour(isCurrent ? juce::Colour(0xff080b10) : juce::Colour(0xff808e9b));
            g.setFont(juce::Font(7.0f, juce::Font::bold));
            g.drawText(shapeTitles[s], sBtn, juce::Justification::centred);
        }

        // Row 2: Per-Band Stereo Channel Mode Bar [ STEREO ] [ MID ] [ SIDE ]
        float modeBtnY = hudCard.getY() + 34.0f;
        float modeBtnW = 42.0f;
        float modeBtnH = 13.0f;
        juce::String modeTitles[] = { "STEREO", "MID", "SIDE" };

        for (int m = 0; m < 3; ++m)
        {
            auto mBtn = juce::Rectangle<float>(hudCard.getX() + 4.0f + m * (modeBtnW + 4.0f), modeBtnY, modeBtnW, modeBtnH);
            bool isCurrent = (mode == m);

            juce::Colour mCol = (m == 1) ? juce::Colour(0xffffa800) : (m == 2 ? juce::Colour(0xffff00aa) : juce::Colour(0xff00ff66));
            g.setColour(isCurrent ? mCol : juce::Colour(0xff121822));
            g.fillRoundedRectangle(mBtn, 2.0f);
            g.setColour(isCurrent ? mCol : juce::Colour(0xff2d3b4d));
            g.drawRoundedRectangle(mBtn, 2.0f, 1.0f);

            g.setColour(isCurrent ? juce::Colour(0xff080b10) : juce::Colour(0xff808e9b));
            g.setFont(juce::Font(7.5f, juce::Font::bold));
            g.drawText(modeTitles[m], mBtn, juce::Justification::centred);
        }
    }
}

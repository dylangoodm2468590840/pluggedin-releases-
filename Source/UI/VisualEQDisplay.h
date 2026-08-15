#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>

struct DynamicEQNode
{
    float freqHz { 1000.0f };
    float gainDb { 0.0f };
    float qFactor { 0.707f };
    int filterType { 0 };  // 0: Bell, 1: Low Cut, 2: High Cut, 3: Low Shelf, 4: High Shelf, 5: Notch
    int stereoMode { 0 };  // 0: Stereo (L+R), 1: Mid (M), 2: Side (S), 3: Left, 4: Right
    bool active { true };
};

class VisualEQDisplay : public juce::Component
{
public:
    VisualEQDisplay();
    ~VisualEQDisplay() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    void setAPVTS(juce::AudioProcessorValueTreeState* stateToUse) { apvts = stateToUse; }
    const std::vector<DynamicEQNode>& getDynamicNodes() const { return dynamicNodes; }

    void updateEQState(const float* realFFTSpectrum64,
                       float lowCutFreq,
                       float lowGainDb,
                       float midGainDb,
                       float highGainDb,
                       float highCutFreq,
                       float lowQVal,
                       float midQVal,
                       float highQVal,
                       int lowCutSlopeVal,
                       int highCutSlopeVal);

private:
    juce::AudioProcessorValueTreeState* apvts { nullptr };

    float spectrumBars[64] { 0.0f };

    float lowCut  { 30.0f };
    float lowGain { 0.0f };
    float midGain { 0.0f };
    float highGain{ 0.0f };
    float highCut { 18000.0f };

    float lowQ    { 0.707f };
    float midQ    { 0.707f };
    float highQ   { 0.707f };

    int lowCutSlope  { 1 };
    int highCutSlope { 1 };

    int activeDraggedNode { -1 };
    int selectedNode      { 1 };

    // Dynamic dB Vertical Zoom Scale Mode (3.0dB, 6.0dB, 12.0dB, 30.0dB)
    float currentDbRange { 12.0f };

    // Horizontal Frequency Timeline Zoom & Pan Navigation Bounds
    float minFreqHz { 20.0f };
    float maxFreqHz { 20000.0f };
    bool isDraggingRuler { false };
    float lastRulerX { 0.0f };

    // Pro-Q 3 Style Dynamic Node Array (Click anywhere to add new nodes!)
    std::vector<DynamicEQNode> dynamicNodes;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualEQDisplay)
};

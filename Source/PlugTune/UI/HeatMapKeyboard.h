#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <functional>

namespace PlugTuneUI
{

class HeatMapKeyboard : public juce::Component, public juce::Timer
{
public:
    HeatMapKeyboard();
    ~HeatMapKeyboard() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent& e) override;

    void timerCallback() override;

    // Set scale note states (true = active in scale, false = disabled/bypassed)
    void setNoteActive(int note0to11, bool isActive);
    void setAllNotes(const std::array<bool, 12>& mask);

    // Feed real-time pitch hit to illuminate heat map
    void registerPitchHit(int note0to11, float intensity);

    // Callbacks
    std::function<void(int note0to11, bool newState)> onNoteToggled;
    std::function<void(int note0to11)> onNoteClicked; // For tone preview

private:
    std::array<bool, 12> mNoteActiveMask;
    std::array<float, 12> mHeatLevels;

    static bool isBlackKey(int note0to11);
    static const char* getNoteName(int note0to11);
};

} // namespace PlugTuneUI

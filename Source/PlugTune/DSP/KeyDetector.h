#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <functional>

namespace PlugTuneDSP
{

struct KeyDetectionOutcome
{
    int rootKey = 0; // 0 = C, 1 = C#, ... 11 = B
    bool isMinor = true;
    float confidence = 0.0f; // 0.0 to 1.0
    juce::String keyName; // e.g. "F# Minor", "C Major"
    bool success = false;
};

class KeyDetector : public juce::Thread
{
public:
    KeyDetector();
    ~KeyDetector() override;

    // Start asynchronous analysis on a dropped audio file
    void analyzeFileAsync(const juce::File& file, std::function<void(KeyDetectionOutcome)> onComplete);

    // Cancel any running analysis
    void cancel();

    static juce::String getNoteName(int note0to11);

private:
    void run() override;

    juce::AudioFormatManager mFormatManager;
    juce::File mTargetFile;
    std::function<void(KeyDetectionOutcome)> mCallback;
    std::atomic<bool> mIsAnalyzing { false };

    KeyDetectionOutcome analyzeAudio(juce::AudioFormatReader* reader);
};

} // namespace PlugTuneDSP

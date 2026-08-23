#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <string>
#include <functional>

namespace Plugged1
{

struct PresetData
{
    std::string name;
    std::string category;
    std::string description;
    std::vector<std::pair<std::string, float>> parameters;
};

class PresetManager
{
public:
    PresetManager(juce::AudioProcessorValueTreeState& apvts);
    ~PresetManager() = default;

    const std::vector<PresetData>& getFactoryPresets() const { return factoryPresets; }
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    void loadPreset(int index);
    void loadPresetByName(const std::string& name);

    std::vector<std::string> getCategories() const;
    std::vector<int> getPresetIndicesForCategory(const std::string& category) const;

private:
    void initFactoryPresets();
    juce::AudioProcessorValueTreeState& apvts;
    std::vector<PresetData> factoryPresets;
    int currentPresetIndex = 0;
};

} // namespace Plugged1

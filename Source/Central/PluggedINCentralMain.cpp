#include <juce_gui_basics/juce_gui_basics.h>
#include "CentralWindow.h"

class PluggedINCentralApplication : public juce::JUCEApplication
{
public:
    PluggedINCentralApplication() = default;

    const juce::String getApplicationName() override { return "PluggedIN Central"; }
    const juce::String getApplicationVersion() override { return "2.4.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<CentralWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

private:
    std::unique_ptr<CentralWindow> mainWindow;
};

START_JUCE_APPLICATION(PluggedINCentralApplication)

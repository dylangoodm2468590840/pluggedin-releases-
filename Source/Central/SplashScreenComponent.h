#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <vector>
#include <functional>

/**
 * @class SplashScreenComponent
 * @brief Ultra-sleek commercial splash & hardware boot sequence screen.
 * Displays dynamic background status tasks, pulsating LED meters, and smooth glow animations.
 */
class SplashScreenComponent : public juce::Component, private juce::Timer
{
public:
    std::function<void()> onSplashComplete;

    SplashScreenComponent()
    {
        bootSteps = {
            { "[INITIALIZING AUDIO CORE & DSP ENGINES...]", 0.20f },
            { "[SCANNING LOCAL VST3 / AU / CLAP DIRECTORIES...]", 0.45f },
            { "[QUERYING LIVE CLOUD UPDATE REPOSITORY...]", 0.70f },
            { "[VERIFYING PRODUCER LICENSES & PRESETS...]", 0.90f },
            { "[PLUGGEDIN AUDIO SUITE READY - LAUNCHING HUB]", 1.00f }
        };

        currentStatus = bootSteps[0].first;
        startTimerHz(30); // 30 FPS smooth animation
    }

    ~SplashScreenComponent() override
    {
        stopTimer();
    }

    void setStatus(const juce::String& statusText, float progressFraction)
    {
        currentStatus = statusText;
        targetProgress = juce::jlimit(0.0f, 1.0f, progressFraction);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        // 1. Dark Gunmetal Industrial Background
        juce::ColourGradient bgGrad(
            juce::Colour(0xff12161f), bounds.getCentreX(), 0,
            juce::Colour(0xff080a0e), bounds.getCentreX(), bounds.getHeight(), false);
        g.setGradientFill(bgGrad);
        g.fillRoundedRectangle(bounds, 12.0f);

        // 2. Neon Cyan Outer Glow Border
        g.setColour(juce::Colour(0xff00f0ff).withAlpha(0.35f + 0.15f * glowPhase));
        g.drawRoundedRectangle(bounds.reduced(1.0f), 12.0f, 1.5f);

        // 3. Precision Rack Corner Screws
        g.setColour(juce::Colour(0xff2a3240));
        float screwSize = 8.0f;
        g.fillEllipse(16, 16, screwSize, screwSize);
        g.fillEllipse(bounds.getWidth() - 24, 16, screwSize, screwSize);
        g.fillEllipse(16, bounds.getHeight() - 24, screwSize, screwSize);
        g.fillEllipse(bounds.getWidth() - 24, bounds.getHeight() - 24, screwSize, screwSize);

        // 4. Center Logo / Branding
        auto logoArea = juce::Rectangle<float>(0, 60, bounds.getWidth(), 70);
        g.setColour(juce::Colour(0xff00f0ff));
        g.setFont(juce::Font(32.0f, juce::Font::bold));
        g.drawText("PLUGGEDIN CENTRAL", logoArea, juce::Justification::centred);

        g.setColour(juce::Colour(0xff7f8b98));
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("PROFESSIONAL AUDIO ENGINE & PLUGIN SUITE", 0, 128, bounds.getWidth(), 20, juce::Justification::centred);

        // 5. Hardware LED VU Meter Animation (8 Stereo segments)
        float meterY = 175.0f;
        float meterStartX = (bounds.getWidth() - (12 * 20.0f)) / 2.0f;
        for (int i = 0; i < 12; ++i)
        {
            float segmentProgress = static_cast<float>(i) / 11.0f;
            bool active = segmentProgress <= currentProgress || (std::sin(animPhase * 4.0f + i * 0.5f) > 0.0f);

            juce::Colour ledColour = (i < 8) ? juce::Colour(0xff00ff66) : (i < 10 ? juce::Colour(0xffffaa00) : juce::Colour(0xffff0055));
            if (!active) ledColour = ledColour.withAlpha(0.15f);

            auto ledRect = juce::Rectangle<float>(meterStartX + i * 20.0f, meterY, 14.0f, 18.0f);
            g.setColour(ledColour);
            g.fillRoundedRectangle(ledRect, 3.0f);
            g.setColour(juce::Colour(0xff0c0e12));
            g.drawRoundedRectangle(ledRect, 3.0f, 1.0f);
        }

        // 6. Progress Bar Track & Bar
        auto barRect = juce::Rectangle<float>(48, 225, bounds.getWidth() - 96, 8);
        g.setColour(juce::Colour(0xff181d26));
        g.fillRoundedRectangle(barRect, 4.0f);
        g.setColour(juce::Colour(0xff283140));
        g.drawRoundedRectangle(barRect, 4.0f, 1.0f);

        auto fillRect = barRect.withWidth(barRect.getWidth() * currentProgress);
        juce::ColourGradient barGrad(
            juce::Colour(0xff00f0ff), fillRect.getX(), 0,
            juce::Colour(0xff00ff66), fillRect.getRight(), 0, false);
        g.setGradientFill(barGrad);
        g.fillRoundedRectangle(fillRect, 4.0f);

        // 7. Dynamic Status Message (Terminal / Console font)
        g.setColour(juce::Colour(0xff00f0ff).withAlpha(0.9f));
        g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.5f, juce::Font::bold));
        g.drawText(currentStatus, 48, 245, bounds.getWidth() - 96, 20, juce::Justification::centred);

        // 8. Footer Meta
        juce::String appVer = (juce::JUCEApplication::getInstance() != nullptr)
                                  ? juce::JUCEApplication::getInstance()->getApplicationVersion()
                                  : "2.4.0";
        g.setColour(juce::Colour(0xff4a5465));
        g.setFont(juce::Font(10.0f, juce::Font::plain));
        g.drawText("v" + appVer + " (x64 Native)  •  Cross-DAW Synchronizer", 0, (int)bounds.getHeight() - 28, bounds.getWidth(), 18, juce::Justification::centred);
    }

private:
    void timerCallback() override
    {
        animPhase += 0.05f;
        glowPhase = static_cast<float>((std::sin(animPhase * 3.0f) + 1.0) * 0.5);

        // Smooth progress interpolation
        currentProgress += (targetProgress - currentProgress) * 0.25f;

        // Progress automatic boot sequence if not externally set
        stepTimer += 1;
        if (stepTimer % 22 == 0 && currentStepIndex < (int)bootSteps.size() - 1)
        {
            currentStepIndex++;
            currentStatus = bootSteps[currentStepIndex].first;
            targetProgress = bootSteps[currentStepIndex].second;
        }

        if (currentStepIndex >= (int)bootSteps.size() - 1 && currentProgress > 0.98f)
        {
            finishDelay++;
            if (finishDelay > 12) // Brief hold on 100%
            {
                stopTimer();
                auto cb = onSplashComplete;
                juce::MessageManager::callAsync([cb]
                {
                    if (cb) cb();
                });
                return;
            }
        }

        repaint();
    }

    std::vector<std::pair<juce::String, float>> bootSteps;
    int currentStepIndex { 0 };
    int stepTimer { 0 };
    int finishDelay { 0 };

    juce::String currentStatus { "[INITIALIZING AUDIO CORE & DSP ENGINES...]" };
    float currentProgress { 0.05f };
    float targetProgress { 0.20f };
    float animPhase { 0.0f };
    float glowPhase { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashScreenComponent)
};

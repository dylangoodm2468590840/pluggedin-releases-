#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * @struct CentralDesignSystem
 * @brief Commercial-grade unified design tokens, typography, and drawing utilities for PluggedIN Central.
 */
struct CentralDesignSystem
{
    // --- Dark Palette Tokens ---
    static inline juce::Colour darkBgMain()       { return juce::Colour(0xff090c13); }
    static inline juce::Colour darkBgSidebar()    { return juce::Colour(0xff0d111a); }
    static inline juce::Colour darkBgHeader()     { return juce::Colour(0xff0e131d); }
    static inline juce::Colour darkBgCard()       { return juce::Colour(0xff121723); }
    static inline juce::Colour darkBgCardHover()  { return juce::Colour(0xff171e2c); }
    static inline juce::Colour darkBgCardActive() { return juce::Colour(0xff1c2436); }
    static inline juce::Colour darkBgInput()      { return juce::Colour(0xff0a0d14); }
    static inline juce::Colour darkBorderSubtle() { return juce::Colour(0xff1b2333); }
    static inline juce::Colour darkBorderMedium() { return juce::Colour(0xff253046); }
    static inline juce::Colour darkBorderGlow()   { return juce::Colour(0xff00f0ff).withAlpha(0.25f); }

    // --- Light Palette Tokens ---
    static inline juce::Colour lightBgMain()       { return juce::Colour(0xfff1f5f9); }
    static inline juce::Colour lightBgSidebar()    { return juce::Colour(0xffe2e8f0); }
    static inline juce::Colour lightBgHeader()     { return juce::Colour(0xfff8fafc); }
    static inline juce::Colour lightBgCard()       { return juce::Colour(0xffffffff); }
    static inline juce::Colour lightBgCardHover()  { return juce::Colour(0xfff8fafc); }
    static inline juce::Colour lightBgCardActive() { return juce::Colour(0xffe2e8f0); }
    static inline juce::Colour lightBgInput()      { return juce::Colour(0xffffffff); }
    static inline juce::Colour lightBorderSubtle() { return juce::Colour(0xffcbd5e1); }
    static inline juce::Colour lightBorderMedium() { return juce::Colour(0xff94a3b8); }
    static inline juce::Colour lightBorderGlow()   { return juce::Colour(0xff0284c7).withAlpha(0.25f); }

    // --- Semantic Accents ---
    static inline juce::Colour cyan(bool dark = true)    { return dark ? juce::Colour(0xff00f0ff) : juce::Colour(0xff0284c7); }
    static inline juce::Colour mint(bool dark = true)    { return dark ? juce::Colour(0xff00ff88) : juce::Colour(0xff16a34a); }
    static inline juce::Colour amber(bool dark = true)   { return dark ? juce::Colour(0xffffaa00) : juce::Colour(0xffd97706); }
    static inline juce::Colour crimson(bool dark = true) { return dark ? juce::Colour(0xffff3366) : juce::Colour(0xffdc2626); }
    static inline juce::Colour purple(bool dark = true)  { return dark ? juce::Colour(0xffa855f7) : juce::Colour(0xff7c3aed); }

    // --- Typography & Semantic Colors ---
    static inline juce::Colour textPrimary(bool dark = true)   { return dark ? juce::Colours::white : juce::Colour(0xff0f172a); }
    static inline juce::Colour textSecondary(bool dark = true) { return dark ? juce::Colour(0xff94a3b8) : juce::Colour(0xff334155); }
    static inline juce::Colour textMuted(bool dark = true)     { return dark ? juce::Colour(0xff64748b) : juce::Colour(0xff64748b); }
    static inline juce::Colour textDim(bool dark = true)       { return dark ? juce::Colour(0xff475569) : juce::Colour(0xff94a3b8); }

    // --- Helpers ---
    static inline juce::Colour bgMain(bool dark)       { return dark ? darkBgMain()       : lightBgMain(); }
    static inline juce::Colour bgSidebar(bool dark)    { return dark ? darkBgSidebar()    : lightBgSidebar(); }
    static inline juce::Colour bgHeader(bool dark)     { return dark ? darkBgHeader()     : lightBgHeader(); }
    static inline juce::Colour bgCard(bool dark)       { return dark ? darkBgCard()       : lightBgCard(); }
    static inline juce::Colour bgCardHover(bool dark)  { return dark ? darkBgCardHover()  : lightBgCardHover(); }
    static inline juce::Colour borderSubtle(bool dark) { return dark ? darkBorderSubtle() : lightBorderSubtle(); }
    static inline juce::Colour borderMedium(bool dark) { return dark ? darkBorderMedium() : lightBorderMedium(); }

    static void drawPillBadge(juce::Graphics& g, const juce::Rectangle<float>& rect, const juce::String& text,
                              juce::Colour bg, juce::Colour fg, float radius = 4.0f, float fontSize = 9.5f)
    {
        g.setColour(bg);
        g.fillRoundedRectangle(rect, radius);
        g.setColour(fg.withAlpha(0.6f));
        g.drawRoundedRectangle(rect, radius, 1.0f);

        g.setColour(fg);
        g.setFont(juce::Font(fontSize, juce::Font::bold));
        g.drawText(text, rect, juce::Justification::centred, true);
    }

    static void drawProgressBar(juce::Graphics& g, const juce::Rectangle<float>& rect, float progress,
                                const juce::String& label, juce::Colour fillCol, bool dark = true)
    {
        g.setColour(dark ? juce::Colour(0xff0d1117) : juce::Colour(0xffe2e8f0));
        g.fillRoundedRectangle(rect, 4.0f);
        g.setColour(borderSubtle(dark));
        g.drawRoundedRectangle(rect, 4.0f, 1.0f);

        float fillW = juce::jlimit(0.0f, rect.getWidth(), rect.getWidth() * progress);
        if (fillW > 0.0f)
        {
            auto fillRect = rect.withWidth(fillW);
            juce::ColourGradient grad(fillCol, fillRect.getX(), 0, fillCol.withAlpha(0.8f), fillRect.getRight(), 0, false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(fillRect, 4.0f);
        }

        if (label.isNotEmpty())
        {
            g.setColour(dark ? juce::Colours::white : juce::Colour(0xff0f172a));
            g.setFont(juce::Font(10.0f, juce::Font::bold));
            g.drawText(label, rect, juce::Justification::centred, true);
        }
    }
};

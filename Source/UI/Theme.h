// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Look of the Decu6000 editor, taken from the original GUI mockup:
// deep violet panels, orange machine voices, cyan/pink bio-neural voices,
// display type for titles and a terminal mono for labels and the LCD.
namespace Theme
{
    const juce::Colour bg      { 0xff12102b };
    const juce::Colour panel   { 0xff2a2150 };
    const juce::Colour panelLo { 0xff1e1840 };
    const juce::Colour text    { 0xffd8d2ff };
    const juce::Colour dim     { 0xff8a83b8 };
    const juce::Colour orange  { 0xffff7a1a };
    const juce::Colour pink    { 0xffff2e93 };
    const juce::Colour cyan    { 0xff3de8ff };
    const juce::Colour lcdBg   { 0xff0a0820 };
    const juce::Colour learn   { 0xffffe14d };   // MIDI learn highlight

    // Fixed design canvas; the editor scales it to the window size.
    constexpr int designWidth  = 1440;
    constexpr int designHeight = 820;

    struct Typefaces
    {
        Typefaces();   // loads the embedded fonts (Theme.cpp)
        juce::Typeface::Ptr bold, black, mono;
    };
    // Hold one of these (the editor does) to keep the embedded fonts loaded.
    using FontKeeper = juce::SharedResourcePointer<Typefaces>;

    juce::Font display (float height, bool black = false);   // titles (Orbitron-derived)
    juce::Font mono (float height);                           // labels, LCD (Share Tech Mono)

    // Knob body as drawn in the mockup. Used by the LookAndFeel and by placeholder knobs.
    void drawKnob (juce::Graphics&, juce::Rectangle<float> area, float angleRadians,
                   juce::Colour accent, bool enabled);

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
        juce::Font getTextButtonFont (juce::TextButton&, int) override;
        void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    };
}

// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "Theme.h"
#include "BinaryData.h"

namespace
{
    juce::Typeface::Ptr loadTypeface (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, static_cast<size_t> (size));
    }
}

Theme::Typefaces::Typefaces()
    : bold  (loadTypeface (BinaryData::DecuDisplayBold_ttf,  BinaryData::DecuDisplayBold_ttfSize)),
      black (loadTypeface (BinaryData::DecuDisplayBlack_ttf, BinaryData::DecuDisplayBlack_ttfSize)),
      mono  (loadTypeface (BinaryData::ShareTechMonoRegular_ttf, BinaryData::ShareTechMonoRegular_ttfSize))
{
}

namespace
{
    // Shared and reference-counted: lives while any editor (FontKeeper) or call holds it,
    // so nothing is left to destruct after JUCE shuts down when the plugin unloads.
    juce::SharedResourcePointer<Theme::Typefaces> typefaces() { return {}; }
}

namespace Theme
{
    juce::Font display (float height, bool black)
    {
        return juce::Font (juce::FontOptions().withTypeface (black ? typefaces()->black : typefaces()->bold)
                                              .withHeight (height).withKerningFactor (0.06f));
    }

    juce::Font mono (float height)
    {
        return juce::Font (juce::FontOptions().withTypeface (typefaces()->mono)
                                              .withHeight (height).withKerningFactor (0.06f));
    }

    void drawKnob (juce::Graphics& g, juce::Rectangle<float> area, float angle, juce::Colour accent, bool enabled)
    {
        const float d = juce::jmin (area.getWidth(), area.getHeight()) - 10.0f;   // room for glow + ring
        const auto c = area.getCentre();
        const auto body = juce::Rectangle<float> (d, d).withCentre (c);
        const float alpha = enabled ? 1.0f : 0.35f;

        // accent glow
        if (enabled)
            for (int i = 3; i >= 1; --i)
            {
                g.setColour (accent.withAlpha (0.10f * static_cast<float> (4 - i)));
                g.fillEllipse (body.expanded (2.0f + static_cast<float> (i) * 1.6f));
            }

        // dark separating ring, then the body with its light rim
        g.setColour (bg);
        g.fillEllipse (body.expanded (3.0f));

        juce::ColourGradient grad (juce::Colour (0xff4a3d8c).withMultipliedAlpha (alpha),
                                   body.getX() + body.getWidth() * 0.35f, body.getY() + body.getHeight() * 0.30f,
                                   juce::Colour (0xff17133a).withMultipliedAlpha (alpha),
                                   body.getRight(), body.getBottom(), true);
        g.setGradientFill (grad);
        g.fillEllipse (body);

        g.setColour (text.withAlpha (enabled ? 1.0f : 0.3f));
        g.drawEllipse (body, d > 50.0f ? 3.0f : 2.0f);

        // pointer, rotated with the knob
        const float r = d * 0.5f;
        const float pw = d > 50.0f ? 4.0f : 3.0f;
        const float ph = r * 0.62f;
        juce::Path p;
        p.addRoundedRectangle (-pw * 0.5f, -r + 2.5f, pw, ph, pw * 0.5f);
        g.setColour (accent.withAlpha (enabled ? 1.0f : 0.35f));
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    //==============================================================================
    LookAndFeel::LookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, lcdBg);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::outlineColourId, orange);
        setColour (juce::ComboBox::arrowColourId, orange);
        setColour (juce::PopupMenu::backgroundColourId, panelLo);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, orange.withAlpha (0.35f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::PopupMenu::headerTextColourId, cyan);
        setColour (juce::TextButton::buttonColourId, panel);
        setColour (juce::TextButton::buttonOnColourId, orange);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, bg);
        setColour (juce::Label::textColourId, text);
    }

    void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
    {
        const float angle = startAngle + pos * (endAngle - startAngle);
        drawKnob (g, juce::Rectangle<int> (x, y, w, h).toFloat(), angle,
                  s.findColour (juce::Slider::rotarySliderFillColourId), s.isEnabled());
    }

    void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        const auto accent = b.findColour (juce::TextButton::buttonOnColourId);

        g.setColour (on ? accent : (over ? panel.brighter (0.15f) : panelLo));
        if (down) g.setColour (accent.withAlpha (0.7f));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (accent.withAlpha (on ? 1.0f : 0.8f));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
    }

    juce::Font LookAndFeel::getTextButtonFont (juce::TextButton&, int) { return display (11.0f); }

    void LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
    {
        auto r = juce::Rectangle<int> (w, h).toFloat().reduced (0.5f);
        g.setColour (findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);

        juce::Path arrow;
        const float ax = static_cast<float> (w) - 16.0f, ay = static_cast<float> (h) * 0.5f;
        arrow.addTriangle (ax - 5.0f, ay - 2.5f, ax + 5.0f, ay - 2.5f, ax, ay + 3.5f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    juce::Font LookAndFeel::getComboBoxFont (juce::ComboBox&) { return mono (13.0f); }
    juce::Font LookAndFeel::getPopupMenuFont()                { return mono (14.0f); }

    void LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
    {
        label.setBounds (8, 1, box.getWidth() - 30, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }
}

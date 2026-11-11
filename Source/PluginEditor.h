// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/Theme.h"
#include "UI/Views.h"

// LCD in the header: shows the current patch ("PATCH 03 // RUST BELT *"),
// temporarily replaced by messages (knob values, MIDI learn). Click for the patch menu.
class LcdDisplay : public juce::Component
{
public:
    void setPatchText (const juce::String& t);
    // holdMs < 0: keep until the next message or clearMessage()
    void showMessage (const juce::String& t, juce::Colour c, int holdMs = 2500);
    void clearMessage();
    void tick();                                   // expires messages
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }

    std::function<void()> onClick;

private:
    juce::String patchText, message;
    juce::Colour messageColour { Theme::cyan };
    juce::uint32 messageUntil = 0;
    bool persistent = false;
    bool showingMessage() const;
};

//==============================================================================
class Decu6000AudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit Decu6000AudioProcessorEditor (Decu6000AudioProcessor&);
    ~Decu6000AudioProcessorEditor() override;

    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    // for tests / screenshots
    void showView (int view);
    void setLearnMode (bool on);
    void selectPad (int pad) { padView.selectPad (pad); }

private:
    // Everything is laid out on a fixed 1440x820 design canvas, scaled to the window.
    class Canvas : public juce::Component
    {
    public:
        void paint (juce::Graphics&) override;
    private:
        juce::Image background;
    };

    void timerCallback() override;
    void padClicked (int pad);
    void armKnobLearn (int param);
    void setLcd (const juce::String& t, juce::Colour c = Theme::cyan)
    {
        lcd.showMessage (t, c, ctx.learnMode && c == Theme::learn ? -1 : 2500);
    }
    void showPatchMenu();
    void saveAsDialog();
    void saveTo (const juce::String& name);
    void confirmDelete();
    void updatePatchText();
    DecuView* currentView();

    Decu6000AudioProcessor& decu;   // (AudioProcessorEditor already has a 'processor' member)
    Theme::FontKeeper fonts;
    Theme::LookAndFeel lnf;
    UiContext ctx;

    Canvas canvas;
    juce::TextButton rackViewButton { "RACK VIEW" }, padViewButton { "PAD VIEW" };
    juce::ComboBox layoutBox;
    juce::TextButton learnButton { "MIDI LEARN" };
    LcdDisplay lcd;
    juce::ArrowButton prevPatch { "prev", 0.5f, Theme::orange }, nextPatch { "next", 0.0f, Theme::orange };

    RackView machineView;
    PadView padView;
    LearnOverlay overlay;
    juce::TooltipWindow tooltips { this, 700 };   // full parameter names on hover

    std::array<int, PadMap::numPads> seenHits {};
    int seenDiag = 0;   // TEMP DIAGNOSTIC
    int seenLearnCount = 0, seenCcLearnCount = 0, tickCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Decu6000AudioProcessorEditor)
};

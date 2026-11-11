// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "Theme.h"
#include "../PluginProcessor.h"
#include <functional>
#include <memory>
#include <vector>

// Shared by every widget: the processor plus callbacks into the editor.
struct UiContext
{
    explicit UiContext (Decu6000AudioProcessor& p) : proc (p) {}

    Decu6000AudioProcessor& proc;
    bool learnMode = false;
    std::function<void (const juce::String&, juce::Colour)> setLcd;
    std::function<void (int pad)> padClicked;        // audition, or arm pad learn in learn mode
    std::function<void (int param)> armKnobLearn;    // from a knob's right-click menu
};

// Something MIDI learn can be pointed at: a pad (note) or a parameter (CC).
struct LearnTarget
{
    juce::Component* comp = nullptr;
    int pad = -1;
    int param = -1;
};

juce::String groupTitleFor (const juce::String& paramId);
juce::String padNoteText (const NoteMap&, int pad, bool withName);
juce::String noteName (int note);   // "C1 (36)" style, middle C = C3

//==============================================================================
// A parameter knob with its label underneath; right-click for MIDI CC learn.
class Knob : public juce::Component
{
public:
    Knob (UiContext&, int paramIndex, juce::Colour accent);
    ~Knob() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;   // via listener on the slider

    int getParamIndex() const { return paramIndex; }

private:
    void reportValue();

    UiContext& ctx;
    const int paramIndex;
    juce::String label, title;
    juce::Slider slider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Same look, no parameter yet ("coming soon" controls from the mockup).
class PlaceholderKnob : public juce::Component
{
public:
    PlaceholderKnob (juce::String labelText, float angleDegrees, bool large = false);
    void paint (juce::Graphics&) override;

private:
    juce::String label;
    float angle;
    bool isLarge;
};

//==============================================================================
// Base for anything that triggers a pad: click to audition (or arm learn),
// right-click to clear its notes, flashes when the pad is hit.
class PadControl : public juce::Component
{
public:
    PadControl (UiContext&, int pad, bool implemented);

    void mouseDown (const juce::MouseEvent&) override;
    void setNoteText (const juce::String&);
    void flash()       { flashLevel = 1.0f; repaint(); }
    void decayFlash();
    int  getPad() const { return pad; }

protected:
    UiContext& ctx;
    const int pad;
    const bool implemented;
    juce::String noteText;
    float flashLevel = 0.0f;
};

// The trigger button at the bottom of a rack-view voice strip.
class StripPad : public PadControl
{
public:
    StripPad (UiContext&, int pad, bool implemented, bool bioStyle);
    void paint (juce::Graphics&) override;

private:
    const bool bio;
};

// A large square tile in the 4x4 pad view.
class PadTile : public PadControl
{
public:
    PadTile (UiContext&, int pad, bool implemented, juce::Colour rowColour);
    void paint (juce::Graphics&) override;
    void setSelected (bool s) { if (s != selected) { selected = s; repaint(); } }

private:
    const juce::Colour colour;
    bool selected = false;
};

//==============================================================================
// One voice column of the rack view: title, 2-column knob grid, trigger pad.
class VoiceStrip : public juce::Component
{
public:
    // prefix != nullptr: real knobs for every parameter with that prefix.
    // Otherwise placeholder knobs with the given labels (voice not built yet).
    VoiceStrip (UiContext&, juce::String title, const char* prefix, juce::StringArray placeholderLabels,
                int pad, bool bioStyle);

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    StripPad& getPad() { return *padButton; }
    void collectLearnTargets (std::vector<LearnTarget>&);

    static constexpr int titleHeight = 30, rowHeight = 56, padHeight = 46;

private:
    juce::String title;
    const bool bio, implemented;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<PlaceholderKnob>> placeholders;
    std::unique_ptr<StripPad> padButton;
};

// Clickable "SEED 4096" display: click for a new random seed.
class SeedDisplay : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit SeedDisplay (UiContext&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    UiContext& ctx;
};

// "BIO-NEURAL ENGINE": LUCID / UNSTABLE plus six instability controls, the live
// instability curve (glitches marked in pink) and the seed.
class EnginePanel : public juce::Component
{
public:
    explicit EnginePanel (UiContext&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();                                    // repaint the live curve
    void collectLearnTargets (std::vector<LearnTarget>&);

private:
    UiContext& ctx;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::unique_ptr<Knob> lucid;
    SeedDisplay seed;
    juce::Rectangle<int> lcdArea;
};

class LevelMeter : public juce::Component
{
public:
    void setLevel (float linearPeak);
    void paint (juce::Graphics&) override;

private:
    float level = 0.0f;   // displayed (decaying) 0..1
};

// "MASTER // GATED VERB": level, gated-reverb return / gate time / predelay, meters.
class MasterPanel : public juce::Component
{
public:
    explicit MasterPanel (UiContext&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void updateMeters();
    void collectLearnTargets (std::vector<LearnTarget>&);

private:
    UiContext& ctx;
    Knob level, reverb, gate, predelay;
    LevelMeter meterL, meterR;
    juce::Rectangle<int> knobArea;
};

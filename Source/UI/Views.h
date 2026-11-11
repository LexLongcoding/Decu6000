// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "Widgets.h"

// Common interface for the two switchable editor views.
class DecuView : public juce::Component
{
public:
    virtual void collectLearnTargets (std::vector<LearnTarget>&) = 0;
    virtual void refreshNotes() = 0;      // pad note labels from the NoteMap
    virtual void flashPad (int pad) = 0;
    virtual void tick() {}                 // ~30 Hz: decay flashes, meters
};

//==============================================================================
// Rack layout (from the original GUI mockup): machine voice strips on top, bio-neural
// voices + engine + master along the bottom.
class RackView : public DecuView
{
public:
    explicit RackView (UiContext&);

    void paint (juce::Graphics&) override;
    void resized() override;

    void collectLearnTargets (std::vector<LearnTarget>&) override;
    void refreshNotes() override;
    void flashPad (int pad) override;
    void tick() override;

private:
    UiContext& ctx;
    std::vector<std::unique_ptr<VoiceStrip>> machine, bio;
    EnginePanel engine;
    MasterPanel master;
    int bottomRowY = 0;
};

//==============================================================================
// 4x4 pad grid like the hardware. Click a pad to hear it and edit it: its knobs
// appear large on the right, with the master section underneath.
class PadView : public DecuView
{
public:
    explicit PadView (UiContext&);

    void paint (juce::Graphics&) override;
    void resized() override;

    void collectLearnTargets (std::vector<LearnTarget>&) override;
    void refreshNotes() override;
    void flashPad (int pad) override;
    void tick() override;

    void selectPad (int pad);
    int  getSelectedPad() const { return selected; }

private:
    void layoutVoiceKnobs();

    UiContext& ctx;
    std::vector<std::unique_ptr<PadTile>> tiles;
    std::vector<std::unique_ptr<Knob>> voiceKnobs, masterKnobs, engineKnobs;
    juce::Rectangle<int> voiceArea, masterArea, engineArea;
    int selected = 0;
    bool engineWasAwake = false;
};

//==============================================================================
// Shown over the active view in MIDI LEARN mode. Outlines every pad and knob
// with its current note / CC; click one to arm it, right-click to clear it.
class LearnOverlay : public juce::Component
{
public:
    LearnOverlay (UiContext&, std::function<DecuView*()> currentView);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    std::function<void (const LearnTarget&)> onArmed;    // for LCD messages
    std::function<void()> onCancelled;

private:
    std::vector<std::pair<LearnTarget, juce::Rectangle<int>>> targetsWithBounds();

    UiContext& ctx;
    std::function<DecuView*()> view;
};

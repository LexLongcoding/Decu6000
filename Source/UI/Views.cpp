// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "Views.h"

using namespace Theme;

namespace
{
    bool isImplemented (int) { return true; }   // all 16 voices are built

    void drawSectionLabel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& t, juce::Colour c, juce::Colour line)
    {
        g.setColour (c);
        g.setFont (mono (12.0f));
        const int w = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), t);
        g.drawText (t, r, juce::Justification::centredLeft, false);
        g.setColour (line);
        g.fillRect (r.getX() + w + 12, r.getCentreY(), r.getRight() - (r.getX() + w + 12), 1);
    }
}

//==============================================================================
RackView::RackView (UiContext& c)
    : ctx (c), engine (c), master (c)
{
    struct Def { const char* title; const char* prefix; int pad; const char* placeholders; };
    const Def machineDefs[] = {
        { "KICK",      "kick",   PadMap::Kick,      "" },
        { "SNARE",     "snare",  PadMap::Snare,     "" },
        { "CLOSED HH", "hhc",    PadMap::ClosedHat, "" },
        { "OPEN HH",   "hho",    PadMap::OpenHat,   "" },
        { "CRASH",     "crash",  PadMap::Crash,     "" },
        { "RIDE",      "ride",   PadMap::Ride,      "" },
        { "LO TOM",    "lotom",  PadMap::LoTom,     "" },
        { "MID TOM",   "midtom", PadMap::MidTom,    "" },
        { "HI TOM",    "hitom",  PadMap::HiTom,     "" },
        { "PERC 1",    "perc1",  PadMap::Perc1,     "" },
        { "PERC 2",    "perc2",  PadMap::Perc2,     "" },
    };
    const Def bioDefs[] = {
        { "CRYSTAL IMPACT",   "crystal", PadMap::Crystal,   "" },
        { "LIQUID METAL",     "liquid",  PadMap::Liquid,    "" },
        { "SYNTHETIC BREATH", "breath",  PadMap::Breath,      "" },
        { "DIGITAL PERC",     "digital", PadMap::DigitalPerc, "" },
        { "BIO BURST",        "burst",   PadMap::BioBurst,    "" },
    };

    for (const auto& d : machineDefs)
    {
        machine.push_back (std::make_unique<VoiceStrip> (ctx, d.title, d.prefix,
                                                         juce::StringArray::fromTokens (d.placeholders, ",", ""), d.pad, false));
        addAndMakeVisible (*machine.back());
    }
    for (const auto& d : bioDefs)
    {
        bio.push_back (std::make_unique<VoiceStrip> (ctx, d.title, d.prefix,
                                                     juce::StringArray::fromTokens (d.placeholders, ",", ""), d.pad, true));
        addAndMakeVisible (*bio.back());
    }
    addAndMakeVisible (engine);
    addAndMakeVisible (master);
    refreshNotes();
}

void RackView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().reduced (20, 0);
    drawSectionLabel (g, r.withHeight (14), "MACHINE VOICES // DRUM CORE", text, text.withAlpha (0.3f));
    drawSectionLabel (g, r.withY (bottomRowY - 24).withHeight (14), "BIO-NEURAL VOICES // SYNTHETIC SOURCES",
                      cyan, cyan.withAlpha (0.4f));
}

void RackView::resized()
{
    auto r = getLocalBounds().reduced (20, 0);
    r.removeFromTop (24);
    auto top = r.removeFromTop (340);
    r.removeFromTop (12 + 14 + 10);
    bottomRowY = r.getY();
    auto bottom = r;

    const int gap = 8;
    const int n = static_cast<int> (machine.size());
    const int w = (top.getWidth() - gap * (n - 1)) / n;
    for (auto& s : machine)
    {
        s->setBounds (top.removeFromLeft (w));
        top.removeFromLeft (gap);
    }

    master.setBounds (bottom.removeFromRight (250));
    bottom.removeFromRight (gap);
    for (auto& s : bio)
    {
        s->setBounds (bottom.removeFromLeft (132));
        bottom.removeFromLeft (gap);
    }
    engine.setBounds (bottom);
}

void RackView::collectLearnTargets (std::vector<LearnTarget>& out)
{
    for (auto& s : machine) s->collectLearnTargets (out);
    for (auto& s : bio)     s->collectLearnTargets (out);
    engine.collectLearnTargets (out);
    master.collectLearnTargets (out);
}

void RackView::refreshNotes()
{
    for (auto* list : { &machine, &bio })
        for (auto& s : *list)
            s->getPad().setNoteText (padNoteText (ctx.proc.noteMap, s->getPad().getPad(), true));
}

void RackView::flashPad (int pad)
{
    for (auto* list : { &machine, &bio })
        for (auto& s : *list)
            if (s->getPad().getPad() == pad)
                s->getPad().flash();
}

void RackView::tick()
{
    for (auto* list : { &machine, &bio })
        for (auto& s : *list)
            s->getPad().decayFlash();
    master.updateMeters();
    engine.tick();
}

//==============================================================================
namespace
{
    // Parameter prefix for each pad (PadMap order).
    const char* prefixForPad (int pad)
    {
        static constexpr const char* prefixes[PadMap::numPads] = {
            "kick", "snare", "hhc", "hho", "lotom", "midtom", "hitom", "crash",
            "ride", "perc1", "perc2", "burst", "crystal", "liquid", "breath", "digital" };
        return prefixes[juce::jlimit (0, PadMap::numPads - 1, pad)];
    }

    bool isBioPad (int pad) { return pad >= PadMap::BioBurst; }
}

PadView::PadView (UiContext& c) : ctx (c)
{
    const juce::Colour rowColours[] = { orange, juce::Colour (0xffffa04d), pink, cyan };
    for (int i = 0; i < PadMap::numPads; ++i)
    {
        tiles.push_back (std::make_unique<PadTile> (ctx, i, isImplemented (i),
                                                    i == PadMap::BioBurst ? cyan : rowColours[i / 4]));
        addAndMakeVisible (*tiles.back());
    }

    for (int i = 0; i < Params::numParams; ++i)
    {
        const juce::String id (Params::specs[i].id);
        if (id.startsWith ("master_"))
        {
            masterKnobs.push_back (std::make_unique<Knob> (ctx, i, pink));
            addAndMakeVisible (*masterKnobs.back());
        }
        else if (id.startsWith ("engine_") && id != "engine_seed")
        {
            engineKnobs.push_back (std::make_unique<Knob> (ctx, i, id == "engine_lucid" || id == "engine_blend" ? pink : cyan));
            addAndMakeVisible (*engineKnobs.back());
        }
    }

    selectPad (PadMap::Kick);
    refreshNotes();
}

void PadView::selectPad (int pad)
{
    pad = juce::jlimit (0, PadMap::numPads - 1, pad);
    if (pad == selected && ! voiceKnobs.empty())
        return;

    selected = pad;
    for (auto& t : tiles)
        t->setSelected (t->getPad() == selected);

    voiceKnobs.clear();
    const juce::String prefix (prefixForPad (pad));
    const auto accent = isBioPad (pad) ? cyan : orange;
    for (int i = 0; i < Params::numParams; ++i)
        if (juce::String (Params::specs[i].id).upToFirstOccurrenceOf ("_", false, false) == prefix)
        {
            voiceKnobs.push_back (std::make_unique<Knob> (ctx, i, accent));
            addAndMakeVisible (*voiceKnobs.back());
        }

    layoutVoiceKnobs();
    repaint();
}

void PadView::paint (juce::Graphics& g)
{
    const bool bio = isBioPad (selected);
    const auto accent = bio ? cyan : orange;

    auto panelBox = [&] (juce::Rectangle<int> r, juce::Colour bar)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
        g.setColour (bar);
        g.fillRoundedRectangle (r.toFloat().withHeight (4.0f), 2.0f);
    };

    panelBox (voiceArea, accent);
    auto title = voiceArea.reduced (24, 0).withTrimmedTop (22).withHeight (34);
    g.setColour (text);
    g.setFont (display (24.0f, true));
    g.drawText (juce::String (PadMap::names[static_cast<size_t> (selected)]).toUpperCase(), title, juce::Justification::centredLeft);
    g.setColour (accent);
    g.setFont (mono (15.0f));
    g.drawText ("PAD " + juce::String (selected + 1).paddedLeft ('0', 2) + "  //  NOTE "
                    + padNoteText (ctx.proc.noteMap, selected, true),
                title, juce::Justification::centredRight);
    g.setColour (dim);
    g.setFont (mono (12.0f));
    g.drawText ("CLICK ANY PAD TO EDIT IT", voiceArea.reduced (24, 0).withTrimmedTop (58).withHeight (16),
                juce::Justification::centredLeft);

    panelBox (masterArea, pink);
    g.setColour (pink);
    g.setFont (display (12.0f));
    g.drawText ("MASTER // GATED VERB", masterArea.reduced (16, 0).withTrimmedTop (12).withHeight (16),
                juce::Justification::centredLeft);

    panelBox (engineArea, cyan);
    g.setColour (cyan);
    g.drawText ("BIO-NEURAL ENGINE", engineArea.reduced (16, 0).withTrimmedTop (12).withHeight (16),
                juce::Justification::centredLeft);
    g.setColour (ctx.proc.getEngine().isAwake() ? pink : dim);
    g.setFont (mono (11.0f));
    g.drawText (ctx.proc.apvts.getRawParameterValue ("engine_blend")->load() <= 0.0f
                    ? "BYPASSED" : (ctx.proc.getEngine().isAwake() ? "AWAKE" : "ASLEEP"),
                engineArea.reduced (16, 0).withTrimmedTop (12).withHeight (16), juce::Justification::centredRight);
}

void PadView::resized()
{
    auto r = getLocalBounds().reduced (20, 0);

    const int grid = juce::jmin (r.getHeight(), 680);
    auto gridArea = r.removeFromLeft (grid).withHeight (grid);
    const int cell = grid / 4;
    for (int i = 0; i < PadMap::numPads; ++i)
    {
        const int col = i % 4, row = i / 4;   // pad 1 bottom-left, like the hardware
        tiles[static_cast<size_t> (i)]->setBounds (gridArea.getX() + col * cell,
                                                   gridArea.getBottom() - (row + 1) * cell, cell, cell);
    }

    r.removeFromLeft (24);
    r = r.withHeight (grid).reduced (0, 5);
    auto bottom = r.removeFromBottom (150);
    r.removeFromBottom (12);
    voiceArea = r;
    masterArea = bottom.removeFromLeft (bottom.getWidth() * 4 / 11);
    bottom.removeFromLeft (10);
    engineArea = bottom;

    auto placeRow = [] (std::vector<std::unique_ptr<Knob>>& knobs, juce::Rectangle<int> area)
    {
        auto m = area.reduced (10, 0).withTrimmedTop (34).withTrimmedBottom (8);
        const int w = m.getWidth() / juce::jmax (1, static_cast<int> (knobs.size()));
        for (auto& k : knobs)
            k->setBounds (m.removeFromLeft (w));
    };
    placeRow (masterKnobs, masterArea);
    placeRow (engineKnobs, engineArea);

    layoutVoiceKnobs();
}

void PadView::layoutVoiceKnobs()
{
    if (voiceArea.isEmpty()) return;
    auto r = voiceArea.reduced (24, 0).withTrimmedTop (92).withTrimmedBottom (20);
    const int cols = 4;
    const int rows = (static_cast<int> (voiceKnobs.size()) + cols - 1) / cols;
    const int cw = r.getWidth() / cols;
    const int ch = juce::jmin (150, r.getHeight() / juce::jmax (1, rows));
    for (size_t i = 0; i < voiceKnobs.size(); ++i)
    {
        const int col = static_cast<int> (i) % cols, row = static_cast<int> (i) / cols;
        voiceKnobs[i]->setBounds (r.getX() + col * cw, r.getY() + row * ch, cw, ch);
    }
}

void PadView::collectLearnTargets (std::vector<LearnTarget>& out)
{
    for (auto& t : tiles)
        out.push_back ({ t.get(), t->getPad(), -1 });
    for (auto* list : { &voiceKnobs, &masterKnobs, &engineKnobs })
        for (auto& k : *list)
            out.push_back ({ k.get(), -1, k->getParamIndex() });
}

void PadView::refreshNotes()
{
    for (auto& t : tiles)
        t->setNoteText (padNoteText (ctx.proc.noteMap, t->getPad(), true));
}

void PadView::flashPad (int pad)
{
    tiles[static_cast<size_t> (pad)]->flash();
}

void PadView::tick()
{
    for (auto& t : tiles)
        t->decayFlash();
    if (const bool awake = ctx.proc.getEngine().isAwake(); awake != engineWasAwake)
    {
        engineWasAwake = awake;
        repaint (engineArea);
    }
}

//==============================================================================
LearnOverlay::LearnOverlay (UiContext& c, std::function<DecuView*()> currentView)
    : ctx (c), view (std::move (currentView))
{
}

std::vector<std::pair<LearnTarget, juce::Rectangle<int>>> LearnOverlay::targetsWithBounds()
{
    std::vector<LearnTarget> targets;
    if (auto* v = view())
        v->collectLearnTargets (targets);

    std::vector<std::pair<LearnTarget, juce::Rectangle<int>>> out;
    // visible within the editor (not isShowing(), which also depends on the host window)
    auto visibleInEditor = [this] (juce::Component* c)
    {
        for (; c != nullptr && c != getParentComponent(); c = c->getParentComponent())
            if (! c->isVisible()) return false;
        return c != nullptr;
    };
    for (const auto& t : targets)
        if (visibleInEditor (t.comp))
            out.emplace_back (t, getLocalArea (t.comp, t.comp->getLocalBounds()));
    return out;
}

void LearnOverlay::paint (juce::Graphics& g)
{
    g.fillAll (bg.withAlpha (0.45f));

    const int armedPad = ctx.proc.getArmedPad();
    const int armedParam = ctx.proc.getArmedCcParam();

    for (const auto& [t, r] : targetsWithBounds())
    {
        const bool armed = (t.pad >= 0 && t.pad == armedPad) || (t.param >= 0 && t.param == armedParam);
        juce::String badge;
        bool mapped = false;
        if (t.pad >= 0)
        {
            badge = padNoteText (ctx.proc.noteMap, t.pad, false);
            mapped = badge != "--";
            if (mapped) badge = "N " + badge;
        }
        else
        {
            const int cc = ctx.proc.ccForParam (t.param);
            mapped = cc >= 0;
            badge = mapped ? "CC " + juce::String (cc) : juce::String ("--");
        }

        const auto rf = r.toFloat().reduced (1.0f);
        if (armed)
        {
            g.setColour (learn.withAlpha (0.28f));
            g.fillRoundedRectangle (rf, 4.0f);
        }
        g.setColour (learn.withAlpha (armed ? 1.0f : (mapped ? 0.85f : 0.4f)));
        g.drawRoundedRectangle (rf, 4.0f, armed ? 2.5f : 1.2f);

        g.setFont (mono (10.0f));
        if (armed) badge = t.pad >= 0 ? "HIT KEY" : "MOVE CC";
        const int bw = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), badge) + 8;
        auto br = juce::Rectangle<int> (r.getRight() - bw - 2, r.getY() + 2, bw, 13).toFloat();
        g.setColour (mapped || armed ? learn : panelLo);
        g.fillRoundedRectangle (br, 3.0f);
        g.setColour (mapped || armed ? bg : dim);
        g.drawText (badge, br.toNearestInt(), juce::Justification::centred, false);
    }
}

void LearnOverlay::mouseDown (const juce::MouseEvent& e)
{
    for (const auto& [t, r] : targetsWithBounds())
    {
        if (! r.contains (e.getPosition()))
            continue;

        if (e.mods.isPopupMenu())   // right-click clears that mapping
        {
            if (t.pad >= 0) ctx.proc.noteMap.clearPad (t.pad);
            else            ctx.proc.clearCcForParam (t.param);
            repaint();
            return;
        }

        const bool alreadyArmed = (t.pad >= 0 && ctx.proc.getArmedPad() == t.pad)
                               || (t.param >= 0 && ctx.proc.getArmedCcParam() == t.param);
        if (alreadyArmed)
        {
            ctx.proc.cancelAllLearning();
            if (onCancelled) onCancelled();
        }
        else
        {
            if (t.pad >= 0) ctx.proc.armLearn (t.pad);
            else            ctx.proc.armCcLearn (t.param);
            if (onArmed) onArmed (t);
        }
        repaint();
        return;
    }

    ctx.proc.cancelAllLearning();   // click on empty space
    if (onCancelled) onCancelled();
    repaint();
}

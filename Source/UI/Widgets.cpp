// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "Widgets.h"

using namespace Theme;

juce::String groupTitleFor (const juce::String& paramId)
{
    const auto prefix = paramId.upToFirstOccurrenceOf ("_", false, false);
    for (const auto& g : Params::groups)
        if (prefix == g.prefix)
            return g.title;
    return prefix.toUpperCase();
}

juce::String noteName (int note)
{
    return juce::MidiMessage::getMidiNoteName (note, true, true, 3) + " (" + juce::String (note) + ")";
}

juce::String padNoteText (const NoteMap& map, int pad, bool withName)
{
    const auto n = map.notesForPad (pad);
    if (n.count == 0)
        return "--";
    juce::String t (n.firstNote);
    if (withName)
        t << "  " << juce::MidiMessage::getMidiNoteName (n.firstNote, true, true, 3);
    if (n.count > 1)
        t << " +" << (n.count - 1);
    return t;
}

//==============================================================================
Knob::Knob (UiContext& c, int index, juce::Colour accent)
    : ctx (c), paramIndex (index)
{
    const auto& spec = Params::specs[index];
    label = spec.knob;
    title = groupTitleFor (spec.id);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setTooltip (spec.name);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (ctx.proc.apvts, spec.id, slider);
    slider.setDoubleClickReturnValue (true, spec.def);
    slider.onDragStart   = [this] { reportValue(); };
    slider.onValueChange = [this] { if (slider.isMouseButtonDown()) reportValue(); };
    slider.addMouseListener (this, false);
    addAndMakeVisible (slider);
}

Knob::~Knob()
{
    slider.removeMouseListener (this);
}

void Knob::reportValue()
{
    if (ctx.setLcd == nullptr) return;
    if (auto* p = ctx.proc.apvts.getParameter (Params::specs[paramIndex].id))
        ctx.setLcd (title + " " + label + "  " + p->getCurrentValueAsText(), Theme::cyan);
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        return;

    const int cc = ctx.proc.ccForParam (paramIndex);
    juce::PopupMenu m;
    m.addSectionHeader (Params::specs[paramIndex].name);
    m.addItem (1, "Learn MIDI CC...");
    m.addItem (2, cc >= 0 ? "Clear MIDI CC (CC " + juce::String (cc) + ")" : juce::String ("Clear MIDI CC"), cc >= 0);
    m.addItem (3, "Reset to default");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&slider),
                     [safe = juce::Component::SafePointer<Knob> (this)] (int r)
                     {
                         if (safe == nullptr) return;
                         if (r == 1 && safe->ctx.armKnobLearn) safe->ctx.armKnobLearn (safe->paramIndex);
                         if (r == 2) safe->ctx.proc.clearCcForParam (safe->paramIndex);
                         if (r == 3) safe->slider.setValue (Params::specs[safe->paramIndex].def, juce::sendNotificationSync);
                     });
}

namespace { int knobLabelHeight (int w, int h) { return h > 100 && w > 90 ? 20 : 14; } }

void Knob::resized()
{
    auto r = getLocalBounds();
    r.removeFromBottom (knobLabelHeight (getWidth(), getHeight()));
    if (getHeight() > 100) r.reduce (0, 8);
    const int size = juce::jmin (r.getWidth(), r.getHeight());
    slider.setBounds (r.withSizeKeepingCentre (size, size));
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (text);
    const int lh = knobLabelHeight (getWidth(), getHeight());
    g.setFont (mono (lh > 14 ? 14.0f : 10.0f));
    g.drawFittedText (label, getLocalBounds().removeFromBottom (lh), juce::Justification::centred, 1, 0.55f);
}

//==============================================================================
PlaceholderKnob::PlaceholderKnob (juce::String labelText, float angleDegrees, bool large)
    : label (std::move (labelText)), angle (juce::degreesToRadians (angleDegrees)), isLarge (large)
{
    setInterceptsMouseClicks (false, false);
}

void PlaceholderKnob::paint (juce::Graphics& g)
{
    auto r = getLocalBounds();
    auto labelArea = r.removeFromBottom (14);
    const int size = juce::jmin (r.getWidth(), r.getHeight(), isLarge ? 84 : 44);
    drawKnob (g, r.withSizeKeepingCentre (size, size).toFloat(), angle, isLarge ? pink : orange, false);
    g.setColour (dim.withAlpha (0.8f));
    g.setFont (mono (10.0f));
    g.drawText (label, labelArea, juce::Justification::centred, false);
}

//==============================================================================
PadControl::PadControl (UiContext& c, int padIndex, bool isImplemented)
    : ctx (c), pad (padIndex), implemented (isImplemented)
{
    setRepaintsOnMouseActivity (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void PadControl::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        m.addSectionHeader (juce::String (PadMap::names[static_cast<size_t> (pad)]).toUpperCase());
        m.addItem (1, "Clear MIDI notes for this pad");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                         [safe = juce::Component::SafePointer<PadControl> (this)] (int r)
                         {
                             if (safe != nullptr && r == 1)
                                 safe->ctx.proc.noteMap.clearPad (safe->pad);
                         });
        return;
    }
    if (ctx.padClicked) ctx.padClicked (pad);
}

void PadControl::setNoteText (const juce::String& t)
{
    if (t != noteText) { noteText = t; repaint(); }
}

void PadControl::decayFlash()
{
    if (flashLevel > 0.0f)
    {
        flashLevel = flashLevel < 0.03f ? 0.0f : flashLevel * 0.8f;
        repaint();
    }
}

//==============================================================================
StripPad::StripPad (UiContext& c, int p, bool impl, bool bioStyle)
    : PadControl (c, p, impl), bio (bioStyle) {}

void StripPad::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    auto face = r.withTrimmedBottom (3.0f);
    const float alpha = implemented ? 1.0f : 0.4f;

    // glow
    g.setColour ((bio ? cyan : orange).withAlpha (0.18f * alpha + 0.25f * flashLevel));
    g.fillRoundedRectangle (r.expanded (2.0f), 6.0f);

    // shadow lip
    g.setColour ((bio ? juce::Colour (0xff7a1a52) : juce::Colour (0xff9c4a0c)).withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (r.withTrimmedTop (3.0f), 4.0f);

    juce::ColourGradient grad = bio
        ? juce::ColourGradient (cyan, face.getX(), face.getY(), pink, face.getRight(), face.getBottom(), false)
        : juce::ColourGradient (juce::Colour (0xffffa04d), 0.0f, face.getY(), orange, 0.0f, face.getBottom(), false);
    grad.multiplyOpacity (alpha);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (face, 4.0f);

    if (flashLevel > 0.0f || isMouseOver())
    {
        g.setColour (juce::Colours::white.withAlpha (0.45f * flashLevel + (isMouseOver() ? 0.08f : 0.0f)));
        g.fillRoundedRectangle (face, 4.0f);
    }

    g.setColour (bg.withAlpha (implemented ? 1.0f : 0.8f));
    g.setFont (mono (13.0f));
    g.drawText (noteText, face.toNearestInt(), juce::Justification::centred, false);
}

//==============================================================================
PadTile::PadTile (UiContext& c, int p, bool impl, juce::Colour rowColour)
    : PadControl (c, p, impl), colour (rowColour) {}

void PadTile::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (5.0f);
    const float corner = 6.0f;

    auto fill = panel;
    if (isMouseOver()) fill = fill.brighter (0.1f);
    fill = fill.interpolatedWith (colour, 0.6f * flashLevel);
    g.setColour (fill);
    g.fillRoundedRectangle (r, corner);

    if (selected)
    {
        g.setColour (colour.withAlpha (0.25f));
        g.drawRoundedRectangle (r.expanded (2.5f), corner + 2.0f, 4.0f);
    }
    g.setColour (colour.withAlpha (implemented ? 0.9f : 0.35f));
    g.drawRoundedRectangle (r, corner, selected ? 3.0f : 1.5f);
    g.fillRoundedRectangle (r.withHeight (5.0f).reduced (10.0f, 0.0f), 2.0f);

    auto content = getLocalBounds().reduced (18, 16);
    g.setColour (dim);
    g.setFont (mono (13.0f));
    g.drawText (juce::String (pad + 1).paddedLeft ('0', 2), content.removeFromTop (18), juce::Justification::topLeft);

    g.setColour (implemented ? text : dim);
    g.setFont (display (15.0f));
    g.drawFittedText (juce::String (PadMap::names[static_cast<size_t> (pad)]).toUpperCase(),
                      content.removeFromTop (48), juce::Justification::centredLeft, 2);

    g.setFont (mono (14.0f));
    g.setColour (noteText == "--" ? dim : colour);
    g.drawText (noteText, content, juce::Justification::bottomLeft);
    if (! implemented)
    {
        g.setColour (dim);
        g.drawText ("SOON", content, juce::Justification::bottomRight);
    }
}

//==============================================================================
VoiceStrip::VoiceStrip (UiContext& ctx, juce::String t, const char* prefix, juce::StringArray placeholderLabels,
                        int pad, bool bioStyle)
    : title (std::move (t)), bio (bioStyle), implemented (prefix != nullptr)
{
    const auto accent = bio ? cyan : orange;

    if (prefix != nullptr)
    {
        for (int i = 0; i < Params::numParams; ++i)
            if (juce::String (Params::specs[i].id).upToFirstOccurrenceOf ("_", false, false) == prefix)
            {
                knobs.push_back (std::make_unique<Knob> (ctx, i, accent));
                addAndMakeVisible (*knobs.back());
            }
    }
    else
    {
        const float angles[] = { 30.0f, 80.0f, 30.0f, 60.0f, 90.0f, -30.0f, 20.0f, -50.0f };
        int n = 0;
        for (const auto& l : placeholderLabels)
        {
            placeholders.push_back (std::make_unique<PlaceholderKnob> (l, angles[n++ % 8]));
            addAndMakeVisible (*placeholders.back());
        }
    }

    padButton = std::make_unique<StripPad> (ctx, pad, implemented, bio);
    addAndMakeVisible (*padButton);
}

void VoiceStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    if (bio)
    {
        g.setGradientFill (juce::ColourGradient (panel, 0.0f, 0.0f, panelLo, 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.0f);
        g.setGradientFill (juce::ColourGradient (cyan, 0.0f, 0.0f, pink, r.getRight(), 0.0f, false));
    }
    else
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (orange);
    }
    juce::Path topBar;
    topBar.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), 4.0f, 4.0f, 4.0f, true, true, false, false);
    g.fillPath (topBar);

    g.setColour (implemented ? text : dim);
    g.setFont (display (12.0f));
    g.drawFittedText (title, getLocalBounds().withTrimmedTop (bio ? 12 : 8).withHeight (titleHeight).reduced (4, 0),
                      juce::Justification::centred, 2);
}

void VoiceStrip::paintOverChildren (juce::Graphics& g)
{
    if (implemented) return;
    g.setColour (dim);
    g.setFont (mono (10.0f));
    g.drawText ("COMING SOON", padButton->getBounds().translated (0, -18).withHeight (14), juce::Justification::centred);
}

void VoiceStrip::resized()
{
    auto r = getLocalBounds().reduced (6, 0);
    r.removeFromTop (bio ? 12 : 8);
    r.removeFromBottom (10);
    r.removeFromTop (titleHeight + 8);

    padButton->setBounds (r.removeFromBottom (padHeight));

    const int cellW = (r.getWidth() - 4) / 2;
    auto place = [&] (juce::Component& c, int i)
    {
        c.setBounds (r.getX() + (i % 2) * (cellW + 4), r.getY() + (i / 2) * (rowHeight + 4), cellW, rowHeight);
    };
    for (size_t i = 0; i < knobs.size(); ++i)        place (*knobs[i], static_cast<int> (i));
    for (size_t i = 0; i < placeholders.size(); ++i) place (*placeholders[i], static_cast<int> (i));
}

void VoiceStrip::collectLearnTargets (std::vector<LearnTarget>& out)
{
    for (auto& k : knobs)
        out.push_back ({ k.get(), -1, k->getParamIndex() });
    out.push_back ({ padButton.get(), padButton->getPad(), -1 });
}

//==============================================================================
SeedDisplay::SeedDisplay (UiContext& c) : ctx (c)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTooltip ("Click for a new random seed. The same seed gives the same glitches.");
}

void SeedDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (lcdBg);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (cyan.withAlpha (isMouseOver() ? 1.0f : 0.7f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    const int value = juce::roundToInt (ctx.proc.apvts.getRawParameterValue ("engine_seed")->load());
    g.setFont (mono (13.0f));
    g.drawText ("SEED " + juce::String (value), getLocalBounds(), juce::Justification::centred);
}

void SeedDisplay::mouseDown (const juce::MouseEvent&)
{
    if (auto* p = ctx.proc.apvts.getParameter ("engine_seed"))
    {
        const int v = juce::Random::getSystemRandom().nextInt (10000);
        p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (v)));
        if (ctx.setLcd) ctx.setLcd ("ENGINE SEED  " + juce::String (v), cyan);
    }
    repaint();
}

//==============================================================================
EnginePanel::EnginePanel (UiContext& c) : ctx (c), seed (c)
{
    for (const char* id : { "engine_grain", "engine_crush", "engine_bits", "engine_rate", "engine_microt",
                            "engine_prob", "engine_blend" })
    {
        const juce::String sid (id);
        knobs.push_back (std::make_unique<Knob> (ctx, Decu6000AudioProcessor::paramIndexForId (id),
                                                 sid == "engine_blend" ? pink : cyan));
        addAndMakeVisible (*knobs.back());
    }
    lucid = std::make_unique<Knob> (ctx, Decu6000AudioProcessor::paramIndexForId ("engine_lucid"), pink);
    addAndMakeVisible (*lucid);
    addAndMakeVisible (seed);
}

void EnginePanel::resized()
{
    auto r = getLocalBounds().reduced (14, 12);
    r.removeFromTop (16 + 10);
    auto right = r.removeFromRight (130);
    r.removeFromRight (18);

    auto knobRow = r.removeFromBottom (62);
    r.removeFromBottom (10);
    lcdArea = r;
    const int w = knobRow.getWidth() / static_cast<int> (knobs.size());
    for (auto& k : knobs)
        k->setBounds (knobRow.removeFromLeft (w));

    seed.setBounds (right.removeFromBottom (30));
    right.removeFromBottom (10);
    lucid->setBounds (right);
}

void EnginePanel::tick()
{
    repaint (lcdArea);
    repaint (getLocalBounds().withHeight (32));   // AWAKE / ASLEEP / OFF status
    seed.repaint();
}

void EnginePanel::collectLearnTargets (std::vector<LearnTarget>& out)
{
    out.push_back ({ lucid.get(), -1, lucid->getParamIndex() });
    for (auto& k : knobs)
        out.push_back ({ k.get(), -1, k->getParamIndex() });
}

void EnginePanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (cyan);
    g.fillRoundedRectangle (r.withHeight (4.0f), 2.0f);

    const auto& engine = ctx.proc.getEngine();
    const bool awake = engine.isAwake();
    const bool on = ctx.proc.apvts.getRawParameterValue ("engine_blend")->load() > 0.0f;

    auto content = getLocalBounds().reduced (14, 12);
    g.setFont (display (12.0f));
    g.drawText ("BIO-NEURAL ENGINE", content.removeFromTop (16), juce::Justification::centredLeft);
    g.setColour (awake ? pink : dim);
    g.setFont (mono (11.0f));
    g.drawText (! on ? "BYPASSED" : (awake ? "AWAKE" : "ASLEEP"),
                getLocalBounds().reduced (14, 12).removeFromTop (16), juce::Justification::centredRight);

    // LCD with the live instability curve
    g.setColour (lcdBg);
    g.fillRoundedRectangle (lcdArea.toFloat(), 3.0f);
    g.setColour (cyan.withAlpha (0.6f));
    g.drawRoundedRectangle (lcdArea.toFloat(), 3.0f, 1.0f);

    auto inner = lcdArea.reduced (8, 6);
    g.setColour (text.withAlpha (0.75f));
    g.setFont (mono (10.0f));
    g.drawText (! on ? "BLEND 0% // ENGINE BYPASSED" : (awake ? "AWAKENING // INSTABILITY OVER TIME" : "ASLEEP // TURN UP LUCID / UNSTABLE"),
                inner.removeFromTop (14), juce::Justification::topLeft);

    juce::Path curve;
    const float x0 = static_cast<float> (inner.getX()), w = static_cast<float> (inner.getWidth());
    const float yBottom = static_cast<float> (inner.getBottom()), h = static_cast<float> (inner.getHeight());
    for (int i = 0; i < BioEngine::historySize; ++i)
    {
        bool glitch = false;
        const float v = engine.getHistory (i, glitch);
        const float x = x0 + w * static_cast<float> (i) / static_cast<float> (BioEngine::historySize - 1);
        const float y = yBottom - 2.0f - v * (h - 4.0f);
        if (i == 0) curve.startNewSubPath (x, y); else curve.lineTo (x, y);
        if (glitch)
        {
            g.setColour (pink.withAlpha (0.8f));
            g.drawLine (x, yBottom - 2.0f, x, juce::jmax (static_cast<float> (inner.getY()), y - h * 0.35f), 1.5f);
        }
    }
    g.setColour (cyan.withAlpha (awake ? 1.0f : 0.35f));
    g.strokePath (curve, juce::PathStrokeType (2.0f));
}

//==============================================================================
void LevelMeter::setLevel (float peak)
{
    const float db = juce::Decibels::gainToDecibels (peak, -60.0f);
    const float target = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
    const float next = juce::jmax (target, level * 0.86f);
    if (std::abs (next - level) > 0.002f) { level = next; repaint(); }
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    juce::ColourGradient grad (cyan, 0.0f, r.getBottom(), pink, 0.0f, r.getY(), false);
    grad.addColour (0.75, orange);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (bg);
    g.fillRect (r.withHeight (r.getHeight() * (1.0f - level)));
}

//==============================================================================
MasterPanel::MasterPanel (UiContext& c)
    : ctx (c),
      level    (c, Decu6000AudioProcessor::paramIndexForId ("master_level"), pink),
      reverb   (c, Decu6000AudioProcessor::paramIndexForId ("master_reverb"), pink),
      gate     (c, Decu6000AudioProcessor::paramIndexForId ("master_gate"), pink),
      predelay (c, Decu6000AudioProcessor::paramIndexForId ("master_predelay"), pink)
{
    for (juce::Component* comp : std::initializer_list<juce::Component*> { &level, &reverb, &gate, &predelay, &meterL, &meterR })
        addAndMakeVisible (comp);
}

void MasterPanel::resized()
{
    auto r = getLocalBounds().reduced (14, 12);
    r.removeFromTop (16 + 10);
    auto meters = r.removeFromRight (28);
    auto mh = meters.withSizeKeepingCentre (28, juce::jmin (130, meters.getHeight()));
    meterL.setBounds (mh.removeFromLeft (12));
    meterR.setBounds (mh.removeFromRight (12));

    r.removeFromRight (14);
    knobArea = r.withSizeKeepingCentre (r.getWidth(), juce::jmin (r.getHeight(), 140));
    const int cw = knobArea.getWidth() / 2, ch = knobArea.getHeight() / 2;
    juce::Component* cells[] = { &level, &reverb, &gate, &predelay };
    for (int i = 0; i < 4; ++i)
        cells[i]->setBounds (knobArea.getX() + (i % 2) * cw, knobArea.getY() + (i / 2) * ch, cw, ch);
}

void MasterPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (pink);
    g.fillRoundedRectangle (r.withHeight (4.0f), 2.0f);
    g.setFont (display (12.0f));
    g.drawText ("MASTER // GATED VERB", getLocalBounds().reduced (14, 12).removeFromTop (16), juce::Justification::centredLeft);
}

void MasterPanel::updateMeters()
{
    meterL.setLevel (ctx.proc.getMeter (0));
    meterR.setLevel (ctx.proc.getMeter (1));
}

void MasterPanel::collectLearnTargets (std::vector<LearnTarget>& out)
{
    for (auto* k : { &level, &reverb, &gate, &predelay })
        out.push_back ({ k, -1, k->getParamIndex() });
}

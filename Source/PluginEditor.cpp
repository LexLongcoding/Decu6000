// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PluginEditor.h"

using namespace Theme;

namespace
{
    constexpr int viewTop = 97;   // below header + tri-colour bar
    constexpr int viewHeight = designHeight - viewTop - 18;

    juce::String padTitle (int pad) { return juce::String (PadMap::names[static_cast<size_t> (pad)]).toUpperCase(); }
    juce::String paramTitle (int param)
    {
        const auto& s = Params::specs[param];
        return groupTitleFor (s.id) + " " + s.knob;
    }
}

//==============================================================================
void LcdDisplay::setPatchText (const juce::String& t)
{
    if (t != patchText) { patchText = t; if (! showingMessage()) repaint(); }
}

void LcdDisplay::showMessage (const juce::String& t, juce::Colour c, int holdMs)
{
    message = t;
    messageColour = c;
    persistent = holdMs < 0;
    messageUntil = juce::Time::getMillisecondCounter() + static_cast<juce::uint32> (juce::jmax (0, holdMs));
    repaint();
}

void LcdDisplay::clearMessage()
{
    message.clear();
    persistent = false;
    repaint();
}

bool LcdDisplay::showingMessage() const
{
    return message.isNotEmpty() && (persistent || juce::Time::getMillisecondCounter() < messageUntil);
}

void LcdDisplay::tick()
{
    if (message.isNotEmpty() && ! showingMessage())
        clearMessage();
}

void LcdDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (lcdBg);
    g.fillRoundedRectangle (r, 3.0f);
    for (int i = 1; i <= 3; ++i)   // inner glow
    {
        g.setColour (cyan.withAlpha (0.06f));
        g.drawRoundedRectangle (r.reduced (static_cast<float> (i) * 1.5f), 3.0f, 2.0f);
    }
    g.setColour (cyan);
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
    const bool msg = showingMessage();
    g.setColour (msg ? messageColour : cyan);
    g.setFont (mono (13.0f));
    g.drawFittedText ((msg ? message : patchText).toUpperCase(), getLocalBounds().reduced (12, 0),
                      juce::Justification::centredLeft, 1, 0.8f);
    if (! msg)
    {
        juce::Path arrow;   // menu hint
        const float ax = static_cast<float> (getWidth()) - 14.0f, ay = static_cast<float> (getHeight()) * 0.5f;
        arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
        g.setColour (cyan.withAlpha (0.6f));
        g.fillPath (arrow);
    }
}

//==============================================================================
void Decu6000AudioProcessorEditor::Canvas::paint (juce::Graphics& g)
{
    if (background.isNull())
    {
        background = juce::Image (juce::Image::RGB, designWidth, designHeight, true);
        juce::Graphics bgG (background);
        bgG.fillAll (bg);
        bgG.setColour (cyan.withAlpha (0.035f));   // scanlines
        for (int y = 0; y < designHeight; y += 3)
            bgG.fillRect (0, y, designWidth, 1);

        // logo
        auto logo = juce::Rectangle<int> (20, 20, 400, 40);
        bgG.setFont (display (32.0f, true));
        const int w = juce::GlyphArrangement::getStringWidthInt (bgG.getCurrentFont(), "DECU");
        bgG.setColour (pink.withAlpha (0.35f));
        bgG.drawText ("DECU", logo.translated (0, 1), juce::Justification::centredLeft, false);
        bgG.setColour (pink);
        bgG.drawText ("DECU", logo, juce::Justification::centredLeft, false);
        bgG.setColour (orange);
        bgG.drawText ("6000", logo.withTrimmedLeft (w), juce::Justification::centredLeft, false);
        bgG.setColour (text);
        bgG.setFont (mono (11.0f));
        bgG.drawText ("BIO-NEURAL PERCUSSION SYNTHESIZER", 21, 60, 400, 14, juce::Justification::centredLeft, false);

        // tri-colour bar
        const int barY = 82, x0 = 20, bw = designWidth - 40;
        bgG.setColour (orange); bgG.fillRect (x0, barY, bw * 3 / 6, 3);
        bgG.setColour (pink);   bgG.fillRect (x0 + bw * 3 / 6, barY, bw * 2 / 6, 3);
        bgG.setColour (cyan);   bgG.fillRect (x0 + bw * 5 / 6, barY, bw - bw * 5 / 6, 3);
    }
    g.drawImageAt (background, 0, 0);
}

//==============================================================================
Decu6000AudioProcessorEditor::Decu6000AudioProcessorEditor (Decu6000AudioProcessor& p)
    : AudioProcessorEditor (p), decu (p), ctx (p),
      machineView (ctx), padView (ctx),
      overlay (ctx, [this] { return currentView(); })
{
    setLookAndFeel (&lnf);
    setWantsKeyboardFocus (true);

    ctx.setLcd       = [this] (const juce::String& t, juce::Colour c) { setLcd (t, c); };
    ctx.padClicked   = [this] (int pad) { padClicked (pad); };
    ctx.armKnobLearn = [this] (int param) { armKnobLearn (param); };

    addAndMakeVisible (canvas);
    canvas.setBounds (0, 0, designWidth, designHeight);

    // header: view tabs
    for (auto* b : { &rackViewButton, &padViewButton })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (1);
        b->setColour (juce::TextButton::buttonOnColourId, cyan);
        canvas.addAndMakeVisible (b);
    }
    rackViewButton.onClick = [this] { if (rackViewButton.getToggleState()) showView (0); };
    padViewButton.onClick = [this] { if (padViewButton.getToggleState()) showView (1); };

    // header: pad layout presets (acts as an action menu)
    layoutBox.setTextWhenNothingSelected ("PAD LAYOUT...");
    for (int i = 0; i < static_cast<int> (NoteMap::Layout::numLayouts); ++i)
        layoutBox.addItem (NoteMap::layoutName (static_cast<NoteMap::Layout> (i)), i + 1);
    layoutBox.onChange = [this]
    {
        const int id = layoutBox.getSelectedId();
        if (id <= 0) return;
        const auto layout = static_cast<NoteMap::Layout> (id - 1);
        decu.noteMap.applyLayout (layout);
        setLcd (juce::String ("LAYOUT: ") + NoteMap::layoutName (layout), orange);
        layoutBox.setSelectedId (0, juce::dontSendNotification);
    };
    canvas.addAndMakeVisible (layoutBox);

    learnButton.setClickingTogglesState (true);
    learnButton.setColour (juce::TextButton::buttonOnColourId, learn);
    learnButton.onClick = [this] { setLearnMode (learnButton.getToggleState()); };
    canvas.addAndMakeVisible (learnButton);
    canvas.addAndMakeVisible (lcd);
    lcd.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    lcd.onClick = [this] { showPatchMenu(); };
    prevPatch.onClick = [this] { decu.patches.loadNext (-1); updatePatchText(); lcd.clearMessage(); };
    nextPatch.onClick = [this] { decu.patches.loadNext (+1); updatePatchText(); lcd.clearMessage(); };
    canvas.addAndMakeVisible (prevPatch);
    canvas.addAndMakeVisible (nextPatch);
    updatePatchText();

    canvas.addChildComponent (machineView);
    canvas.addChildComponent (padView);
    canvas.addChildComponent (overlay);
    overlay.onArmed = [this] (const LearnTarget& t)
    {
        if (t.pad >= 0) setLcd ("LEARN: HIT A KEY OR PAD FOR " + padTitle (t.pad), learn);
        else            setLcd ("LEARN: MOVE A KNOB OR FADER FOR " + paramTitle (t.param), learn);
    };
    overlay.onCancelled = [this] { setLcd ("LEARN: CLICK A PAD OR KNOB  //  ESC TO EXIT", learn); };

    for (int i = 0; i < PadMap::numPads; ++i)
        seenHits[static_cast<size_t> (i)] = decu.getHitCount (i);
    seenLearnCount = decu.getLearnCount();
    seenCcLearnCount = decu.getCcLearnCount();

    showView (decu.uiView);
    sendLookAndFeelChange();

    const float savedScale = juce::jlimit (0.5f, 1.25f, decu.uiScale);   // read before any resized() call
    setResizable (true, true);
    setResizeLimits (designWidth / 2, designHeight / 2, designWidth * 5 / 4, designHeight * 5 / 4);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio (static_cast<double> (designWidth) / designHeight);
    setSize (juce::roundToInt (designWidth * savedScale), juce::roundToInt (designHeight * savedScale));
    startTimerHz (30);
}

Decu6000AudioProcessorEditor::~Decu6000AudioProcessorEditor()
{
    decu.cancelAllLearning();
    setLookAndFeel (nullptr);
}

DecuView* Decu6000AudioProcessorEditor::currentView()
{
    return decu.uiView == 1 ? static_cast<DecuView*> (&padView) : static_cast<DecuView*> (&machineView);
}

void Decu6000AudioProcessorEditor::showView (int view)
{
    decu.uiView = juce::jlimit (0, 1, view);
    machineView.setVisible (decu.uiView == 0);
    padView.setVisible (decu.uiView == 1);
    rackViewButton.setToggleState (decu.uiView == 0, juce::dontSendNotification);
    padViewButton.setToggleState (decu.uiView == 1, juce::dontSendNotification);
    currentView()->refreshNotes();
    overlay.repaint();
}

void Decu6000AudioProcessorEditor::setLearnMode (bool on)
{
    ctx.learnMode = on;
    learnButton.setToggleState (on, juce::dontSendNotification);
    decu.cancelAllLearning();
    overlay.setVisible (on);
    overlay.toFront (false);
    if (on) setLcd ("LEARN: CLICK A PAD OR KNOB  //  ESC TO EXIT", learn);
    else    lcd.clearMessage();
}

void Decu6000AudioProcessorEditor::padClicked (int pad)
{
    decu.auditionPad (pad);
    padView.selectPad (pad);
    setLcd (padTitle (pad) + "  //  " + padNoteText (decu.noteMap, pad, true), orange);
}

void Decu6000AudioProcessorEditor::armKnobLearn (int param)
{
    decu.armCcLearn (param);
    setLcd ("LEARN: MOVE A KNOB OR FADER FOR " + paramTitle (param) + "  //  ESC CANCELS", learn);
}

bool Decu6000AudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if (key != juce::KeyPress::escapeKey)
        return false;

    if (decu.getArmedPad() >= 0 || decu.getArmedCcParam() >= 0)
    {
        decu.cancelAllLearning();
        setLcd (ctx.learnMode ? "LEARN: CANCELLED  //  CLICK A PAD OR KNOB" : "LEARN: CANCELLED", learn);
        overlay.repaint();
        return true;
    }
    if (ctx.learnMode)
    {
        setLearnMode (false);
        return true;
    }
    return false;
}

void Decu6000AudioProcessorEditor::timerCallback()
{
    auto* view = currentView();
    updatePatchText();
    lcd.tick();

    for (int i = 0; i < PadMap::numPads; ++i)
    {
        const int hits = decu.getHitCount (i);
        if (hits != seenHits[static_cast<size_t> (i)])
        {
            seenHits[static_cast<size_t> (i)] = hits;
            view->flashPad (i);
        }
    }
    view->tick();
    view->refreshNotes();

    bool learnChanged = false;
    if (const int n = decu.getLearnCount(); n != seenLearnCount)
    {
        seenLearnCount = n;
        learnChanged = true;
        const int note = decu.getLastLearnedNote();
        const int pad = decu.noteMap.padForNote (note);
        if (pad >= 0)
            setLcd (padTitle (pad) + " <- NOTE " + noteName (note), learn);
    }
    if (const int n = decu.getCcLearnCount(); n != seenCcLearnCount)
    {
        seenCcLearnCount = n;
        learnChanged = true;
        const int cc = decu.getLastLearnedCc();
        for (int i = 0; i < Params::numParams; ++i)
            if (decu.ccForParam (i) == cc)
                setLcd (paramTitle (i) + " <- CC " + juce::String (cc), learn);
    }

    if (overlay.isVisible() && (learnChanged || ++tickCount % 3 == 0))
        overlay.repaint();
}

void Decu6000AudioProcessorEditor::resized()
{
    if (getWidth() <= 0)
        return;
    const float scale = static_cast<float> (getWidth()) / static_cast<float> (designWidth);
    canvas.setTransform (juce::AffineTransform::scale (scale));
    decu.uiScale = scale;

    // header controls, right-aligned, in design coordinates
    auto header = juce::Rectangle<int> (20, 28, designWidth - 40, 34);
    learnButton.setBounds (header.removeFromRight (130));
    header.removeFromRight (10);
    layoutBox.setBounds (header.removeFromRight (230));
    header.removeFromRight (10);
    nextPatch.setBounds (header.removeFromRight (26).reduced (4, 8));
    header.removeFromRight (2);
    lcd.setBounds (header.removeFromRight (330));
    header.removeFromRight (2);
    prevPatch.setBounds (header.removeFromRight (26).reduced (4, 8));
    header.removeFromRight (16);
    padViewButton.setBounds (header.removeFromRight (100));
    header.removeFromRight (6);
    rackViewButton.setBounds (header.removeFromRight (100));

    const juce::Rectangle<int> viewArea (0, viewTop, designWidth, viewHeight);
    machineView.setBounds (viewArea);
    padView.setBounds (viewArea);
    overlay.setBounds (viewArea);
}

//==============================================================================
// Patches
void Decu6000AudioProcessorEditor::updatePatchText()
{
    auto& pm = decu.patches;
    const int n = pm.getDisplayNumber();
    lcd.setPatchText ("PATCH " + (n > 0 ? juce::String (n).paddedLeft ('0', 2) : juce::String ("--"))
                      + " // " + pm.getCurrentName() + (pm.isModified() ? " *" : ""));
}

void Decu6000AudioProcessorEditor::showPatchMenu()
{
    auto& pm = decu.patches;
    pm.refresh();   // pick up patches saved from other instances
    const auto& entries = pm.getEntries();

    enum { saveId = 1, saveAsId, deleteId, folderId, loadBase = 1000 };
    juce::PopupMenu m;
    m.addSectionHeader ("FACTORY");
    for (int i = 0; i < static_cast<int> (entries.size()); ++i)
        if (entries[static_cast<size_t> (i)].factory)
            m.addItem (loadBase + i, entries[static_cast<size_t> (i)].name, true, i == pm.getCurrentIndex());
    m.addSectionHeader ("USER");
    if (static_cast<int> (entries.size()) == pm.getNumFactory())
        m.addItem (-1, "(no user patches yet - use Save As)", false);
    for (int i = 0; i < static_cast<int> (entries.size()); ++i)
        if (! entries[static_cast<size_t> (i)].factory)
            m.addItem (loadBase + i, entries[static_cast<size_t> (i)].name, true, i == pm.getCurrentIndex());
    m.addSeparator();
    m.addItem (saveId, "Save \"" + pm.getCurrentName() + "\"", pm.isCurrentUserPatch());
    m.addItem (saveAsId, "Save As...");
    m.addItem (deleteId, "Delete \"" + pm.getCurrentName() + "\"...", pm.isCurrentUserPatch());
    m.addSeparator();
    m.addItem (folderId, "Open patch folder");

    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&lcd),
                     [safe = juce::Component::SafePointer<Decu6000AudioProcessorEditor> (this)] (int r)
                     {
                         if (safe == nullptr || r == 0) return;
                         auto& p = safe->decu.patches;
                         if (r >= loadBase)
                         {
                             if (! p.load (r - loadBase))
                                 safe->setLcd ("COULDN'T READ THAT PATCH FILE", pink);
                             else
                                 safe->lcd.clearMessage();
                         }
                         else if (r == saveId)   safe->saveTo (p.getCurrentName());
                         else if (r == saveAsId) safe->saveAsDialog();
                         else if (r == deleteId) safe->confirmDelete();
                         else if (r == folderId)
                         {
                             const auto folder = PatchManager::getUserFolder();
                             folder.createDirectory();
                             folder.startAsProcess();
                         }
                         safe->updatePatchText();
                     });
}

void Decu6000AudioProcessorEditor::saveAsDialog()
{
    auto* w = new juce::AlertWindow ("SAVE PATCH", "Name for this patch:", juce::MessageBoxIconType::NoIcon, this);
    const auto& pm = decu.patches;
    w->addTextEditor ("name", pm.isCurrentUserPatch() || pm.getCurrentName() != "INIT" ? pm.getCurrentName() : "MY PATCH");
    w->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    w->enterModalState (true, juce::ModalCallbackFunction::create (
        [safe = juce::Component::SafePointer<Decu6000AudioProcessorEditor> (this), w] (int result)
        {
            if (safe == nullptr || result != 1) return;
            const auto name = PatchManager::sanitiseName (w->getTextEditorContents ("name"));
            if (name.isEmpty()) { safe->setLcd ("PATCH NAME CAN'T BE EMPTY", pink); return; }

            if (safe->decu.patches.userPatchExists (name) && name != safe->decu.patches.getCurrentName())
            {
                juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                                  .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                                  .withTitle ("REPLACE PATCH?")
                                                  .withMessage ("A patch called \"" + name + "\" already exists. Replace it?")
                                                  .withButton ("REPLACE").withButton ("CANCEL")
                                                  .withAssociatedComponent (safe.getComponent()),
                                              [safe, name] (int r) { if (safe != nullptr && r == 1) safe->saveTo (name); });
                return;
            }
            safe->saveTo (name);
        }), true);
}

void Decu6000AudioProcessorEditor::saveTo (const juce::String& name)
{
    if (decu.patches.saveUser (name))
        setLcd ("SAVED: " + PatchManager::sanitiseName (name), orange);
    else
        setLcd ("COULDN'T SAVE - CHECK THE PATCH FOLDER", pink);
    updatePatchText();
}

void Decu6000AudioProcessorEditor::confirmDelete()
{
    const auto name = decu.patches.getCurrentName();
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::WarningIcon)
                                      .withTitle ("DELETE PATCH?")
                                      .withMessage ("Delete the patch \"" + name + "\"? The current sound stays loaded.")
                                      .withButton ("DELETE").withButton ("CANCEL")
                                      .withAssociatedComponent (this),
                                  [safe = juce::Component::SafePointer<Decu6000AudioProcessorEditor> (this), name] (int r)
                                  {
                                      if (safe == nullptr || r != 1) return;
                                      auto& pm = safe->decu.patches;
                                      if (pm.deleteUser (pm.getCurrentIndex()))
                                          safe->setLcd ("DELETED: " + name, orange);
                                      safe->updatePatchText();
                                  });
}

// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PatchManager.h"
#include "FactoryPatches.h"
#include "Parameters.h"

PatchManager::PatchManager (juce::AudioProcessorValueTreeState& s) : state (s)
{
    for (const auto& spec : Params::specs)
        state.addParameterListener (spec.id, this);
    refresh();
}

PatchManager::~PatchManager()
{
    for (const auto& spec : Params::specs)
        state.removeParameterListener (spec.id, this);
}

juce::File PatchManager::getUserFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Decu6000").getChildFile ("Patches");
}

juce::String PatchManager::sanitiseName (const juce::String& name)
{
    auto n = juce::File::createLegalFileName (name.trim()).trim();
    return n.substring (0, 40).toUpperCase();
}

void PatchManager::refresh()
{
    const auto keepName = currentName;
    const bool keepUser = isCurrentUserPatch();

    entries.clear();
    const auto& factory = FactoryPatches::all();
    for (int i = 0; i < static_cast<int> (factory.size()); ++i)
        entries.push_back ({ factory[static_cast<size_t> (i)].name, true, i, {} });

    auto files = getUserFolder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + fileExtension);
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0; });
    for (const auto& f : files)
        entries.push_back ({ f.getFileNameWithoutExtension().toUpperCase(), false, -1, f });

    selectByName (keepName, ! keepUser);
}

int PatchManager::getNumFactory() const
{
    return static_cast<int> (FactoryPatches::all().size());
}

bool PatchManager::isCurrentUserPatch() const
{
    return currentIndex >= 0 && currentIndex < static_cast<int> (entries.size())
        && ! entries[static_cast<size_t> (currentIndex)].factory
        && entries[static_cast<size_t> (currentIndex)].name == currentName;
}

bool PatchManager::userPatchExists (const juce::String& name) const
{
    return getUserFolder().getChildFile (sanitiseName (name) + fileExtension).existsAsFile();
}

void PatchManager::selectByName (const juce::String& name, bool factoryFirst)
{
    int found = -1;
    for (int pass = 0; pass < 2 && found < 0; ++pass)
        for (int i = 0; i < static_cast<int> (entries.size()); ++i)
        {
            const auto& e = entries[static_cast<size_t> (i)];
            const bool wanted = (pass == 0) == factoryFirst ? e.factory : ! e.factory;
            if (wanted && e.name == name) { found = i; break; }
        }
    currentIndex = found;   // -1: the name isn't in the list (e.g. patch file deleted)
}

void PatchManager::applyValues (const std::vector<std::pair<juce::String, float>>& values)
{
    applying = true;
    for (const auto& spec : Params::specs)
    {
        float v = spec.def;
        for (const auto& [id, value] : values)
            if (id == spec.id) { v = value; break; }

        if (auto* p = state.getParameter (spec.id))
            p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (spec.min, spec.max, v)));
    }
    applying = false;
    modified.store (false);
}

bool PatchManager::load (int index)
{
    if (index < 0 || index >= static_cast<int> (entries.size()))
        return false;

    const auto& e = entries[static_cast<size_t> (index)];
    std::vector<std::pair<juce::String, float>> values;

    if (e.factory)
    {
        for (const auto& [id, v] : FactoryPatches::all()[static_cast<size_t> (e.factoryIndex)].values)
            values.emplace_back (id, v);
    }
    else
    {
        auto xml = juce::XmlDocument::parse (e.file);
        if (xml == nullptr || ! xml->hasTagName ("DecuPatch"))
            return false;
        for (auto* p : xml->getChildWithTagNameIterator ("Param"))
            values.emplace_back (p->getStringAttribute ("id"), static_cast<float> (p->getDoubleAttribute ("value")));
    }

    applyValues (values);
    currentIndex = index;
    currentName = e.name;
    notifyCurrentChanged();
    return true;
}

void PatchManager::loadNext (int direction)
{
    const int n = static_cast<int> (entries.size());
    if (n == 0) return;
    const int start = currentIndex < 0 ? (direction > 0 ? -1 : 0) : currentIndex;
    load (((start + direction) % n + n) % n);
}

bool PatchManager::saveUser (const juce::String& rawName)
{
    const auto name = sanitiseName (rawName);
    if (name.isEmpty())
        return false;

    juce::XmlElement xml ("DecuPatch");
    xml.setAttribute ("name", name);
    xml.setAttribute ("version", 1);
    for (const auto& spec : Params::specs)
    {
        auto* p = xml.createNewChildElement ("Param");
        p->setAttribute ("id", spec.id);
        p->setAttribute ("value", static_cast<double> (state.getRawParameterValue (spec.id)->load()));
    }

    const auto folder = getUserFolder();
    if (! folder.createDirectory())
        return false;
    if (! xml.writeTo (folder.getChildFile (name + fileExtension)))
        return false;

    currentName = name;
    modified.store (false);
    refresh();
    selectByName (name, false);
    notifyCurrentChanged();
    return true;
}

bool PatchManager::deleteUser (int index)
{
    if (index < 0 || index >= static_cast<int> (entries.size()) || entries[static_cast<size_t> (index)].factory)
        return false;
    if (! entries[static_cast<size_t> (index)].file.deleteFile())
        return false;
    const bool wasCurrent = index == currentIndex;
    refresh();
    if (wasCurrent) { currentIndex = -1; modified.store (true); }   // sound stays; it's now unsaved
    notifyCurrentChanged();
    return true;
}

void PatchManager::restoreSession (const juce::String& name, bool wasModified)
{
    if (name.isNotEmpty())
    {
        currentName = name;
        selectByName (name, true);
    }
    modified.store (wasModified);
    notifyCurrentChanged();
}

void PatchManager::parameterChanged (const juce::String&, float)
{
    if (! applying)
        modified.store (true);
}

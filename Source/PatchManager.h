// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>

// Patches = every sound parameter, saved by name. Factory patches are built in;
// user patches are .decupatch files (small XML) in the user's patch folder:
//   Windows  %APPDATA%\Decu6000\Patches
//   macOS    ~/Library/Decu6000/Patches
//   Linux    ~/.config/Decu6000/Patches
// Pad/key mappings and MIDI CC assignments are NOT part of a patch - they
// describe your controller, so they stay put when you change sounds.
//
// Values are stored in real units (Hz, seconds...) by parameter ID, so patches
// survive range changes, and parameters a patch doesn't mention (added in a
// later version) load at their defaults.
class PatchManager : private juce::AudioProcessorValueTreeState::Listener
{
public:
    struct Entry
    {
        juce::String name;
        bool factory = false;
        int factoryIndex = -1;
        juce::File file;
    };

    explicit PatchManager (juce::AudioProcessorValueTreeState&);
    ~PatchManager() override;

    static juce::File getUserFolder();
    static constexpr const char* fileExtension = ".decupatch";

    void refresh();                                   // rescan factory + user patches
    const std::vector<Entry>& getEntries() const      { return entries; }
    int  getNumFactory() const;

    bool load (int index);                            // returns false if the file couldn't be read
    void loadNext (int direction);                    // +1 / -1, wraps
    bool saveUser (const juce::String& name);         // overwrites a user patch of the same name
    bool deleteUser (int index);
    bool userPatchExists (const juce::String& name) const;

    int  getCurrentIndex() const                      { return currentIndex; }
    juce::String getCurrentName() const               { return currentName; }
    bool isModified() const                           { return modified.load(); }
    bool isCurrentUserPatch() const;

    // Patch number shown on the LCD ("PATCH 07"), 1-based; 0 if not in the list.
    int  getDisplayNumber() const                     { return currentIndex + 1; }

    // Session state (saved with the DAW project).
    void restoreSession (const juce::String& name, bool wasModified);

    static juce::String sanitiseName (const juce::String&);

private:
    void parameterChanged (const juce::String&, float) override;
    void applyValues (const std::vector<std::pair<juce::String, float>>& values);
    void selectByName (const juce::String& name, bool factoryFirst);

    juce::AudioProcessorValueTreeState& state;
    std::vector<Entry> entries;
    int currentIndex = 0;
    juce::String currentName { "INIT" };
    std::atomic<bool> modified { false };
    std::atomic<bool> applying { false };   // CC moves can notify from the audio thread
};

// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KickVoice.h"
#include "SnareVoice.h"
#include "HatVoice.h"
#include "TomVoice.h"
#include "CymbalVoice.h"
#include "PercVoice.h"
#include "CrystalVoice.h"
#include "LiquidVoice.h"
#include "BreathVoice.h"
#include "DigitalVoice.h"
#include "BurstVoice.h"
#include "GatedReverb.h"
#include "BioEngine.h"
#include <vector>
#include "PadMap.h"
#include "NoteMap.h"
#include "Parameters.h"
#include "PatchManager.h"
#include <array>
#include <atomic>

class Decu6000AudioProcessor : public juce::AudioProcessor
{
public:
    Decu6000AudioProcessor();
    ~Decu6000AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Decu6000"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    // Not exposed to the host - see PluginProcessor.cpp.
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    PatchManager patches { apvts };

    //==============================================================================
    // Pad (note) MIDI learn - used by the editor on the message thread. Lock-free.
    NoteMap noteMap;

    void armLearn (int pad)          { learnCcParam.store (-1); learnPad.store (pad); }   // -1 cancels
    int  getArmedPad() const         { return learnPad.load(); }
    int  getLearnCount() const       { return learnCount.load(); }
    int  getLastLearnedNote() const  { return lastLearnedNote.load(); }
    int  getHitCount (int pad) const { return hitCount[static_cast<size_t> (pad)].load (std::memory_order_relaxed); }
    void auditionPad (int pad);

    //==============================================================================
    // Knob (CC) MIDI learn. Params are addressed by their index in Params::specs.
    static int paramIndexForId (const juce::String& id);

    void armCcLearn (int paramIndex)    { learnPad.store (-1); learnCcParam.store (paramIndex); }   // -1 cancels
    int  getArmedCcParam() const        { return learnCcParam.load(); }
    int  getCcLearnCount() const        { return ccLearnCount.load(); }
    int  getLastLearnedCc() const       { return lastLearnedCc.load(); }
    int  ccForParam (int paramIndex) const;          // -1 if none
    void clearCcForParam (int paramIndex);
    void clearAllCcs();

    void cancelAllLearning()            { learnPad.store (-1); learnCcParam.store (-1); }

    //==============================================================================
    // Output meters (peak, linear) for the editor.
    const BioEngine& getEngine() const { return engine; }
    float getMeter (int channel) const  { return meters[static_cast<size_t> (channel & 1)].load (std::memory_order_relaxed); }

    // Editor state saved with the session (message thread only).
    int   uiView  = 0;       // 0 = rack view, 1 = pad view
    float uiScale = 0.8f;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateVoiceParams();
    void handleMidi (const juce::MidiMessage& m, int pos, int numSamples);
    void addEvent (int pos, int pad, float velocity);
    void resetAllVoices();
    void handleController (int cc, int value);
    void triggerPad (int pad, float velocity);
    float pv (int index) const { return raw[static_cast<size_t> (index)]->load(); }
    static int indexFor (const char* prefix, const char* suffix);
    void buildIndexTable();

    juce::String ccMapToString() const;
    void ccMapFromString (const juce::String&);

    static constexpr const char* noteMapProperty = "padNoteMap";
    static constexpr const char* ccMapProperty   = "ccMap";
    static constexpr const char* uiViewProperty  = "uiView";
    static constexpr const char* uiScaleProperty = "uiScale";
    static constexpr const char* patchNameProperty = "patchName";
    static constexpr const char* patchModifiedProperty = "patchModified";
    static constexpr int numCcs = 120;    // CC 120-127 are channel-mode messages, never mapped

    std::array<juce::RangedAudioParameter*, static_cast<size_t> (Params::numParams)> paramList {};
    std::array<std::atomic<float>*, static_cast<size_t> (Params::numParams)> raw {};

    struct HatIx    { int tune, decay, level, tone, metal, crush; };
    struct CymbalIx { int tune, decay, level, tone, ring, crush; };
    struct TomIx    { int tune, decay, level, bend, noise, drive, crush; };
    struct PercIx   { int pitch, decay, level, ratio, index, crush; };
    struct BioIx    { int pitch, decay, level, a, b, send; };   // a/b: the voice's two character controls
    struct IndexTable
    {
        int kickPitch = 0, kickDecay = 0, kickLevel = 0, kickDrive = 0, kickDrone = 0;
        int snareTune = 0, snareDecay = 0, snareLevel = 0, snareSnappy = 0, snareTone = 0, snareDrive = 0, snareCrush = 0;
        int masterLevel = 0;
        std::array<HatIx, 2> hats {};
        std::array<CymbalIx, 2> cymbals {};
        std::array<TomIx, 3> toms {};
        std::array<PercIx, 2> percs {};
        BioIx crystal {}, liquid {}, breath {}, digital {}, burst {};
        int reverbReturn = 0, reverbGate = 0, reverbPredelay = 0;
        struct { int blend, lucid, grain, crush, bits, rate, microT, prob, seed; } engine {};
    } ix;

    std::atomic<int> learnPad { -1 }, learnCount { 0 }, lastLearnedNote { -1 };
    std::atomic<int> learnCcParam { -1 }, ccLearnCount { 0 }, lastLearnedCc { -1 };
    std::array<std::atomic<int>, numCcs> ccToParam;
    std::array<std::atomic<int>, PadMap::numPads> hitCount {};
    std::array<std::atomic<bool>, PadMap::numPads> pendingAudition {};
    std::array<std::atomic<float>, 2> meters {};

    KickVoice   kick;
    SnareVoice  snare;
    HatVoice    closedHat { 0xC105EDu };
    HatVoice    openHat   { 0x0BE7AAu };
    CymbalVoice crash     { 0xC4A5Bu };
    CymbalVoice ride      { 0x0010E5u };
    TomVoice    loTom     { 0x10A1u };
    TomVoice    midTom    { 0x10A2u };
    TomVoice    hiTom     { 0x10A3u };
    PercVoice   perc1, perc2;
    CrystalVoice crystal;
    LiquidVoice  liquid;
    BreathVoice  breath;
    DigitalVoice digital;
    BurstVoice   burst;

    GatedReverb reverb;
    BioEngine engine;

    // hits for the current block, and hits the engine delayed past its end
    struct Event   { int pos; int pad; float velocity; };
    struct Pending { juce::int64 time; int pad; float velocity; };
    std::array<Event, 256> events {};
    std::array<Pending, 64> pending {};
    int numEvents = 0, numPending = 0;
    juce::int64 sampleClock = 0;
    std::vector<float> sendBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Decu6000AudioProcessor)
};

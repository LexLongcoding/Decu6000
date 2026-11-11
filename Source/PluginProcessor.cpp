// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::AudioParameterFloatAttributes attributesFor (Params::Unit unit)
    {
        using U = Params::Unit;
        auto a = juce::AudioParameterFloatAttributes();
        switch (unit)
        {
            case U::Time:
                return a.withLabel ("s")
                    .withStringFromValueFunction ([] (float v, int)
                    {
                        return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms"
                                        : juce::String (v, 2) + " s";
                    })
                    .withValueFromStringFunction ([] (const juce::String& t)
                    {
                        const float v = t.getFloatValue();
                        return t.containsIgnoreCase ("ms") ? v / 1000.0f : v;
                    });
            case U::Hz:
                return a.withLabel ("Hz")
                    .withStringFromValueFunction ([] (float v, int)
                    {
                        return v < 20.0f ? juce::String (v, 1) + " Hz" : juce::String (juce::roundToInt (v)) + " Hz";
                    })
                    .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });
            case U::Semitones:   // stored 0..1, shown as -12 .. +12 semitones
                return a.withLabel ("st")
                    .withStringFromValueFunction ([] (float v, int)
                    {
                        const int st = juce::roundToInt (v * 24.0f - 12.0f);
                        return (st > 0 ? "+" : "") + juce::String (st) + " st";
                    })
                    .withValueFromStringFunction ([] (const juce::String& t) { return (t.getFloatValue() + 12.0f) / 24.0f; });
            case U::Toggle:
                return a.withLabel ("")
                    .withStringFromValueFunction ([] (float v, int) { return juce::String (v >= 0.5f ? "ON" : "OFF"); })
                    .withValueFromStringFunction ([] (const juce::String& t)
                    {
                        return t.trim().equalsIgnoreCase ("on") || t.getFloatValue() >= 0.5f ? 1.0f : 0.0f;
                    });
            case U::Integer:
                return a.withLabel ("")
                    .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)); })
                    .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });
            case U::Ratio:
                return a.withLabel (":1")
                    .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2) + ":1"; })
                    .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });
            case U::Percent:
                break;
        }
        return a.withLabel ("%")
            .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; })
            .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue() / 100.0f; });
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout Decu6000AudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& s : Params::specs)
    {
        juce::NormalisableRange<float> range (s.min, s.max);
        if (s.centre > 0.0f)
            range.setSkewForCentre (s.centre);
        if (s.unit == Params::Unit::Integer || s.unit == Params::Unit::Toggle)
            range.interval = 1.0f;
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { s.id, 1 }, s.name,
                                                                 range, s.def, attributesFor (s.unit)));
    }
    return layout;
}

int Decu6000AudioProcessor::paramIndexForId (const juce::String& id)
{
    for (int i = 0; i < Params::numParams; ++i)
        if (id == Params::specs[i].id)
            return i;
    return -1;
}

Decu6000AudioProcessor::Decu6000AudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "DECU6000", createLayout())
{
    for (int i = 0; i < Params::numParams; ++i)
    {
        paramList[static_cast<size_t> (i)] = apvts.getParameter (Params::specs[i].id);
        raw[static_cast<size_t> (i)] = apvts.getRawParameterValue (Params::specs[i].id);
    }
    buildIndexTable();
    for (auto& c : ccToParam)
        c.store (-1);
}

void Decu6000AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sendBuffer.assign (static_cast<size_t> (juce::jmax (1, samplesPerBlock)), 0.0f);
    reverb.prepare (sampleRate, samplesPerBlock);
    crystal.prepare (sampleRate);
    liquid.prepare (sampleRate);
    breath.prepare (sampleRate);
    digital.prepare (sampleRate);
    burst.prepare (sampleRate);
    engine.prepare (sampleRate, samplesPerBlock);
    numPending = 0;
    kick.prepare (sampleRate);
    snare.prepare (sampleRate);
    closedHat.prepare (sampleRate);
    openHat.prepare (sampleRate);
    crash.prepare (sampleRate);
    ride.prepare (sampleRate);
    loTom.prepare (sampleRate);
    midTom.prepare (sampleRate);
    hiTom.prepare (sampleRate);
    perc1.prepare (sampleRate);
    perc2.prepare (sampleRate);
}

bool Decu6000AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet().isDisabled();
}

void Decu6000AudioProcessor::updateVoiceParams()
{
    // raw[] holds each parameter's atomic value, looked up once in the constructor,
    // so nothing here allocates or searches on the audio thread.
    KickVoice::Params k;
    k.pitchHz  = pv (ix.kickPitch);
    k.decaySec = pv (ix.kickDecay);
    k.level    = pv (ix.kickLevel);
    k.drive    = pv (ix.kickDrive);
    k.drone    = pv (ix.kickDrone);
    kick.setParams (k);

    SnareVoice::Params s;
    s.tuneHz   = pv (ix.snareTune);
    s.decaySec = pv (ix.snareDecay);
    s.snappy   = pv (ix.snareSnappy);
    s.tone     = pv (ix.snareTone);
    s.drive    = pv (ix.snareDrive);
    s.crush    = pv (ix.snareCrush);
    s.level    = pv (ix.snareLevel);
    snare.setParams (s);

    for (size_t i = 0; i < 2; ++i)
    {
        const auto& h = ix.hats[i];
        HatVoice::Params p;
        p.tune = pv (h.tune); p.decaySec = pv (h.decay); p.level = pv (h.level);
        p.tone = pv (h.tone); p.metal = pv (h.metal);    p.crush = pv (h.crush);
        (i == 0 ? closedHat : openHat).setParams (p);
    }

    for (size_t i = 0; i < 2; ++i)
    {
        const auto& c = ix.cymbals[i];
        CymbalVoice::Params p;
        p.tune = pv (c.tune); p.decaySec = pv (c.decay); p.level = pv (c.level);
        p.tone = pv (c.tone); p.ring = pv (c.ring);      p.crush = pv (c.crush);
        (i == 0 ? crash : ride).setParams (p);
    }

    TomVoice* toms[] = { &loTom, &midTom, &hiTom };
    for (size_t i = 0; i < 3; ++i)
    {
        const auto& t = ix.toms[i];
        TomVoice::Params p;
        p.tuneHz = pv (t.tune); p.decaySec = pv (t.decay); p.level = pv (t.level);
        p.bend = pv (t.bend);   p.noise = pv (t.noise);    p.drive = pv (t.drive); p.crush = pv (t.crush);
        toms[i]->setParams (p);
    }

    PercVoice* percs[] = { &perc1, &perc2 };
    for (size_t i = 0; i < 2; ++i)
    {
        const auto& c = ix.percs[i];
        PercVoice::Params p;
        p.pitchHz = pv (c.pitch); p.decaySec = pv (c.decay); p.level = pv (c.level);
        p.ratio = pv (c.ratio);   p.index = pv (c.index);    p.crush = pv (c.crush);
        percs[i]->setParams (p);
    }

    CrystalVoice::Params c;
    c.pitchHz = pv (ix.crystal.pitch); c.decaySec = pv (ix.crystal.decay); c.level = pv (ix.crystal.level);
    c.inharm  = pv (ix.crystal.a);     c.shimmer  = pv (ix.crystal.b);
    crystal.setParams (c);

    LiquidVoice::Params l;
    l.pitchHz = pv (ix.liquid.pitch); l.decaySec = pv (ix.liquid.decay); l.level = pv (ix.liquid.level);
    l.drip    = pv (ix.liquid.a);     l.modRateHz = pv (ix.liquid.b);
    liquid.setParams (l);

    BreathVoice::Params b;
    b.pitchHz = pv (ix.breath.pitch); b.decaySec = pv (ix.breath.decay); b.level = pv (ix.breath.level);
    b.vowel   = pv (ix.breath.a);     b.breath   = pv (ix.breath.b);
    breath.setParams (b);

    DigitalVoice::Params d;
    d.pitchHz = pv (ix.digital.pitch); d.decaySec = pv (ix.digital.decay); d.level = pv (ix.digital.level);
    d.chaos   = pv (ix.digital.a);     d.fold     = pv (ix.digital.b);
    digital.setParams (d);

    BurstVoice::Params u;
    u.pitchHz = pv (ix.burst.pitch); u.decaySec = pv (ix.burst.decay); u.level = pv (ix.burst.level);
    u.swarm   = pv (ix.burst.a);     u.grain    = pv (ix.burst.b);
    burst.setParams (u);

    BioEngine::Params e;
    e.lucid  = pv (ix.engine.lucid);  e.blend = pv (ix.engine.blend);  e.grain = pv (ix.engine.grain); e.crush  = pv (ix.engine.crush);
    e.bits   = pv (ix.engine.bits);   e.rateHz = pv (ix.engine.rate); e.microT = pv (ix.engine.microT);
    e.prob   = pv (ix.engine.prob);   e.seed  = juce::roundToInt (pv (ix.engine.seed));
    engine.setParams (e);
}

int Decu6000AudioProcessor::indexFor (const char* prefix, const char* suffix)
{
    const int i = paramIndexForId (juce::String (prefix) + "_" + suffix);
    jassert (i >= 0);   // every voice parameter must exist in Params::specs
    return juce::jmax (0, i);
}

void Decu6000AudioProcessor::buildIndexTable()
{
    ix.kickPitch = indexFor ("kick", "pitch");   ix.kickDecay = indexFor ("kick", "decay");
    ix.kickLevel = indexFor ("kick", "level");   ix.kickDrive = indexFor ("kick", "drive");
    ix.kickDrone = indexFor ("kick", "drone");
    ix.snareTune = indexFor ("snare", "tune");   ix.snareDecay = indexFor ("snare", "decay");
    ix.snareLevel = indexFor ("snare", "level"); ix.snareSnappy = indexFor ("snare", "snappy");
    ix.snareTone = indexFor ("snare", "tone");   ix.snareDrive = indexFor ("snare", "drive");
    ix.snareCrush = indexFor ("snare", "crush");
    ix.masterLevel = indexFor ("master", "level");

    const char* hatPre[] = { "hhc", "hho" };
    for (size_t i = 0; i < 2; ++i)
        ix.hats[i] = { indexFor (hatPre[i], "tune"), indexFor (hatPre[i], "decay"), indexFor (hatPre[i], "level"),
                       indexFor (hatPre[i], "tone"), indexFor (hatPre[i], "metal"), indexFor (hatPre[i], "crush") };

    const char* cymPre[] = { "crash", "ride" };
    for (size_t i = 0; i < 2; ++i)
        ix.cymbals[i] = { indexFor (cymPre[i], "tune"), indexFor (cymPre[i], "decay"), indexFor (cymPre[i], "level"),
                          indexFor (cymPre[i], "tone"), indexFor (cymPre[i], "ring"), indexFor (cymPre[i], "crush") };

    const char* tomPre[] = { "lotom", "midtom", "hitom" };
    for (size_t i = 0; i < 3; ++i)
        ix.toms[i] = { indexFor (tomPre[i], "tune"), indexFor (tomPre[i], "decay"), indexFor (tomPre[i], "level"),
                       indexFor (tomPre[i], "bend"), indexFor (tomPre[i], "noise"), indexFor (tomPre[i], "drive"),
                       indexFor (tomPre[i], "crush") };

    const char* percPre[] = { "perc1", "perc2" };
    for (size_t i = 0; i < 2; ++i)
        ix.percs[i] = { indexFor (percPre[i], "pitch"), indexFor (percPre[i], "decay"), indexFor (percPre[i], "level"),
                        indexFor (percPre[i], "ratio"), indexFor (percPre[i], "index"), indexFor (percPre[i], "crush") };

    ix.crystal = { indexFor ("crystal", "pitch"), indexFor ("crystal", "decay"), indexFor ("crystal", "level"),
                   indexFor ("crystal", "inharm"), indexFor ("crystal", "shimmer"), indexFor ("crystal", "send") };
    ix.liquid  = { indexFor ("liquid", "pitch"), indexFor ("liquid", "decay"), indexFor ("liquid", "level"),
                   indexFor ("liquid", "drip"), indexFor ("liquid", "modrate"), indexFor ("liquid", "send") };
    ix.breath  = { indexFor ("breath", "pitch"), indexFor ("breath", "decay"), indexFor ("breath", "level"),
                   indexFor ("breath", "vowel"), indexFor ("breath", "breath"), indexFor ("breath", "send") };
    ix.digital = { indexFor ("digital", "pitch"), indexFor ("digital", "decay"), indexFor ("digital", "level"),
                   indexFor ("digital", "chaos"), indexFor ("digital", "fold"), indexFor ("digital", "send") };
    ix.burst   = { indexFor ("burst", "pitch"), indexFor ("burst", "decay"), indexFor ("burst", "level"),
                   indexFor ("burst", "swarm"), indexFor ("burst", "grain"), indexFor ("burst", "send") };
    ix.engine = { indexFor ("engine", "blend"), indexFor ("engine", "lucid"), indexFor ("engine", "grain"), indexFor ("engine", "crush"),
                  indexFor ("engine", "bits"), indexFor ("engine", "rate"), indexFor ("engine", "microt"),
                  indexFor ("engine", "prob"), indexFor ("engine", "seed") };
    ix.reverbReturn = indexFor ("master", "reverb");
    ix.reverbGate   = indexFor ("master", "gate");
    ix.reverbPredelay = indexFor ("master", "predelay");
}

//==============================================================================
void Decu6000AudioProcessor::resetAllVoices()
{
    for (DrumVoice* v : std::initializer_list<DrumVoice*> { &kick, &snare, &closedHat, &openHat, &crash,
                                                            &ride, &loTom, &midTom, &hiTom, &perc1, &perc2,
                                                            &crystal, &liquid, &breath, &digital, &burst })
        v->reset();
    reverb.reset();
    engine.reset();
    numPending = 0;
}

// Handles one MIDI message at sample `pos` of the current block. Note-ons become
// trigger events (possibly dropped or delayed by the Bio-Neural Engine).
void Decu6000AudioProcessor::handleMidi (const juce::MidiMessage& m, int pos, int numSamples)
{
    if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        resetAllVoices();
        numEvents = 0;
        return;
    }

    if (m.isController())
    {
        handleController (m.getControllerNumber(), m.getControllerValue());
        return;
    }

    if (! m.isNoteOn())
        return;

    const int note = m.getNoteNumber();

    // Pad learn: the first note after a pad is armed gets assigned to it.
    const int armed = learnPad.exchange (-1);
    if (armed >= 0)
    {
        noteMap.learn (note, armed);
        lastLearnedNote.store (note);
        learnCount.fetch_add (1);
    }

    const int pad = noteMap.padForNote (note);
    if (pad < 0)
        return;

    int delay = 0;
    if (! engine.onHit (delay))      // PROB dropped this hit
        return;

    const int t = pos + delay;
    if (t < numSamples || numPending >= static_cast<int> (pending.size()))
        addEvent (juce::jmin (t, numSamples - 1), pad, m.getFloatVelocity());
    else
        pending[static_cast<size_t> (numPending++)] = { sampleClock + t, pad, m.getFloatVelocity() };
}

void Decu6000AudioProcessor::addEvent (int pos, int pad, float vel)
{
    if (numEvents < static_cast<int> (events.size()))
        events[static_cast<size_t> (numEvents++)] = { pos, pad, vel };
}

void Decu6000AudioProcessor::handleController (int cc, int value)
{
    if (cc < 0 || cc >= numCcs)
        return;

    // Knob learn: the first CC after a knob is armed gets assigned to it.
    const int armed = learnCcParam.exchange (-1);
    if (armed >= 0 && armed < Params::numParams)
    {
        for (auto& c : ccToParam)                  // a knob listens to one CC
            if (c.load() == armed) c.store (-1);
        ccToParam[static_cast<size_t> (cc)].store (armed);
        lastLearnedCc.store (cc);
        ccLearnCount.fetch_add (1);
    }

    const int idx = ccToParam[static_cast<size_t> (cc)].load();
    if (idx >= 0 && idx < Params::numParams)
        paramList[static_cast<size_t> (idx)]->setValueNotifyingHost (static_cast<float> (value) / 127.0f);
}

int Decu6000AudioProcessor::ccForParam (int paramIndex) const
{
    for (int cc = 0; cc < numCcs; ++cc)
        if (ccToParam[static_cast<size_t> (cc)].load() == paramIndex)
            return cc;
    return -1;
}

void Decu6000AudioProcessor::clearCcForParam (int paramIndex)
{
    for (auto& c : ccToParam)
        if (c.load() == paramIndex) c.store (-1);
}

void Decu6000AudioProcessor::clearAllCcs()
{
    for (auto& c : ccToParam) c.store (-1);
}

void Decu6000AudioProcessor::triggerPad (int pad, float vel)
{
    if (pad < 0 || pad >= PadMap::numPads)
        return;

    hitCount[static_cast<size_t> (pad)].fetch_add (1, std::memory_order_relaxed);

    switch (pad)
    {
        case PadMap::Kick:      kick.trigger (vel);  break;
        case PadMap::Snare:     snare.trigger (vel); break;
        case PadMap::ClosedHat: openHat.choke(); closedHat.trigger (vel); break;   // closed chokes open
        case PadMap::OpenHat:   openHat.trigger (vel); break;
        case PadMap::LoTom:     loTom.trigger (vel);  break;
        case PadMap::MidTom:    midTom.trigger (vel); break;
        case PadMap::HiTom:     hiTom.trigger (vel);  break;
        case PadMap::Crash:     crash.trigger (vel);  break;
        case PadMap::Ride:      ride.trigger (vel);   break;
        case PadMap::Perc1:     perc1.trigger (vel);  break;
        case PadMap::Perc2:     perc2.trigger (vel);  break;
        case PadMap::Crystal:   crystal.trigger (vel); break;
        case PadMap::Liquid:    liquid.trigger (vel);  break;
        case PadMap::Breath:    breath.trigger (vel);  break;
        case PadMap::DigitalPerc: digital.trigger (vel); break;
        case PadMap::BioBurst:  burst.trigger (vel);   break;
        default: break;
    }
}

void Decu6000AudioProcessor::auditionPad (int pad)
{
    if (pad >= 0 && pad < PadMap::numPads)
        pendingAudition[static_cast<size_t> (pad)].store (true);
}

//==============================================================================
void Decu6000AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    updateVoiceParams();

    // pads clicked in the editor play at the start of the block
    for (size_t p = 0; p < pendingAudition.size(); ++p)
        if (pendingAudition[p].exchange (false))
            triggerPad (static_cast<int> (p), 0.85f);

    const int numSamples = buffer.getNumSamples();
    float* left  = buffer.getWritePointer (0);
    float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    if (static_cast<int> (sendBuffer.size()) < numSamples)        // host sent more than it promised
        sendBuffer.resize (static_cast<size_t> (numSamples), 0.0f);
    const float crystalSend = pv (ix.crystal.send), liquidSend = pv (ix.liquid.send), breathSend = pv (ix.breath.send),
                digitalSend = pv (ix.digital.send), burstSend = pv (ix.burst.send);

    auto render = [&] (int start, int end)
    {
        for (int i = start; i < end; ++i)
        {
            const float c = crystal.process(), l = liquid.process(), br = breath.process(),
                        dg = digital.process(), bu = burst.process();
            left[i] = kick.process() + snare.process()
                    + closedHat.process() + openHat.process()
                    + crash.process() + ride.process()
                    + loTom.process() + midTom.process() + hiTom.process()
                    + perc1.process() + perc2.process() + c + l + br + dg + bu;
            sendBuffer[static_cast<size_t> (i)] = c * crystalSend + l * liquidSend + br * breathSend
                                                + dg * digitalSend + bu * burstSend;
        }
    };

    // 1) gather this block's hits: MIDI note-ons (via the engine) + delayed hits now due
    numEvents = 0;
    for (const auto meta : midi)
        handleMidi (meta.getMessage(), juce::jlimit (0, juce::jmax (0, numSamples - 1), meta.samplePosition), numSamples);

    for (int i = 0; i < numPending;)
    {
        auto& p = pending[static_cast<size_t> (i)];
        if (p.time < sampleClock + numSamples)
        {
            addEvent (static_cast<int> (juce::jmax<juce::int64> (0, p.time - sampleClock)), p.pad, p.velocity);
            p = pending[static_cast<size_t> (--numPending)];
        }
        else ++i;
    }

    // 2) sort by position (small list - insertion sort)
    for (int i = 1; i < numEvents; ++i)
        for (int j = i; j > 0 && events[static_cast<size_t> (j - 1)].pos > events[static_cast<size_t> (j)].pos; --j)
            std::swap (events[static_cast<size_t> (j - 1)], events[static_cast<size_t> (j)]);

    // 3) render, triggering each hit at its exact sample
    int pos = 0;
    for (int e = 0; e < numEvents; ++e)
    {
        const auto& ev = events[static_cast<size_t> (e)];
        render (pos, ev.pos);
        triggerPad (ev.pad, ev.velocity);
        pos = ev.pos;
    }
    render (pos, numSamples);
    sampleClock += numSamples;

    // dry is mono so far; the gated reverb adds a stereo wet signal
    if (right != nullptr)
        std::copy (left, left + numSamples, right);
    reverb.process (sendBuffer.data(), left, right, numSamples,
                    pv (ix.reverbPredelay), pv (ix.reverbGate), pv (ix.reverbReturn));

    // the Bio-Neural Engine works on the whole mix (bypassed while LUCID is 0)
    engine.process (left, right, numSamples);

    const float master = pv (ix.masterLevel);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* d = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            d[i] = Dsp::softLimit (d[i] * master);
    }

    meters[0].store (buffer.getMagnitude (0, 0, numSamples), std::memory_order_relaxed);
    meters[1].store (buffer.getMagnitude (buffer.getNumChannels() > 1 ? 1 : 0, 0, numSamples), std::memory_order_relaxed);
}

int Decu6000AudioProcessor::getNumPrograms()
{
    return juce::jmax (1, static_cast<int> (patches.getEntries().size()));
}

int Decu6000AudioProcessor::getCurrentProgram()
{
    return juce::jmax (0, patches.getCurrentIndex());
}

void Decu6000AudioProcessor::setCurrentProgram (int index)
{
    patches.load (index);
}

const juce::String Decu6000AudioProcessor::getProgramName (int index)
{
    const auto& e = patches.getEntries();
    return index >= 0 && index < static_cast<int> (e.size()) ? e[static_cast<size_t> (index)].name : juce::String();
}

juce::AudioProcessorEditor* Decu6000AudioProcessor::createEditor()
{
    return new Decu6000AudioProcessorEditor (*this);
}

//==============================================================================
juce::String Decu6000AudioProcessor::ccMapToString() const
{
    juce::StringArray items;
    for (int cc = 0; cc < numCcs; ++cc)
    {
        const int idx = ccToParam[static_cast<size_t> (cc)].load();
        if (idx >= 0 && idx < Params::numParams)
            items.add (juce::String (cc) + "=" + Params::specs[idx].id);
    }
    return items.joinIntoString (",");
}

void Decu6000AudioProcessor::ccMapFromString (const juce::String& text)
{
    clearAllCcs();
    for (const auto& item : juce::StringArray::fromTokens (text, ",", ""))
    {
        const int cc = item.upToFirstOccurrenceOf ("=", false, false).getIntValue();
        const int idx = paramIndexForId (item.fromFirstOccurrenceOf ("=", false, false));
        if (cc >= 0 && cc < numCcs && idx >= 0 && item.containsChar ('='))
            ccToParam[static_cast<size_t> (cc)].store (idx);   // unknown IDs (renamed params) are dropped
    }
}

void Decu6000AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty (noteMapProperty, juce::String (noteMap.toString()), nullptr);
    state.setProperty (ccMapProperty, ccMapToString(), nullptr);
    state.setProperty (uiViewProperty, uiView, nullptr);
    state.setProperty (uiScaleProperty, uiScale, nullptr);
    state.setProperty (patchNameProperty, patches.getCurrentName(), nullptr);
    state.setProperty (patchModifiedProperty, patches.isModified(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Decu6000AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);

            const auto mapText = state.getProperty (noteMapProperty).toString();
            if (mapText.isEmpty() || ! noteMap.fromString (mapText.toStdString()))
                noteMap.applyLayout (NoteMap::Layout::PadsFrom36);   // older sessions / bad data
            ccMapFromString (state.getProperty (ccMapProperty).toString());
            uiView  = juce::jlimit (0, 1, static_cast<int> (state.getProperty (uiViewProperty, 0)));
            uiScale = juce::jlimit (0.5f, 1.25f, static_cast<float> (state.getProperty (uiScaleProperty, 0.8f)));

            const auto patchName = state.getProperty (patchNameProperty).toString();
            const bool patchModified = state.getProperty (patchModifiedProperty, patchName.isEmpty());

            for (auto* p : { noteMapProperty, ccMapProperty, uiViewProperty, uiScaleProperty,
                             patchNameProperty, patchModifiedProperty })
                state.removeProperty (p, nullptr);
            apvts.replaceState (state);
            patches.restoreSession (patchName, patchModified);   // after replaceState, which marks "modified"
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Decu6000AudioProcessor();
}

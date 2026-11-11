// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include "DspUtils.h"

// Simmons-flavoured tom: sine body with an adjustable pitch "Bend" (the
// classic 80s/90s electronic zap when pushed), a short filtered-noise stick
// transient, then Drive and Crush. Lo / Mid / Hi are three instances, each
// with its own full set of controls.
class TomVoice : public DrumVoice
{
public:
    explicit TomVoice (uint32_t noiseSeed = 0x70A1u) : noise (noiseSeed) {}

    struct Params
    {
        float tuneHz   = 120.0f;  // settled pitch after the bend
        float decaySec = 0.4f;    // to -60 dB
        float bend     = 0.4f;    // 0..1, pitch-drop depth (0 = flat, 1 = ~3 octaves down-sweep)
        float noise    = 0.3f;    // 0..1, stick transient
        float drive    = 0.2f;    // 0..1
        float crush    = 0.0f;    // 0..1
        float level    = 0.8f;    // 0..1
    };

    void setParams (const Params& p)
    {
        params = p;
        noiseFilter.setCoefficients (params.tuneHz * 12.0f, 0.8f, sampleRate);
        crusher.setAmount (params.crush, sampleRate);
    }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        clickCoeff = Dsp::decayCoeff (0.025f, sr);
        setParams (params);
        reset();
    }

    void reset() override
    {
        noise.reset();
        active = false;
        ampEnv = pitchEnv = clickEnv = 0.0f;
        phase = 0.0f;
        quietCount = 0;
        noiseFilter.reset(); crusher.reset();
    }

    void trigger (float vel) override
    {
        velocity = vel;
        phase = 0.0f;
        ampEnv = pitchEnv = clickEnv = 1.0f;
        quietCount = 0;
        active = true;

        const float d = std::max (params.decaySec, 0.05f);
        ampCoeff = Dsp::decayCoeff (d, sampleRate);
        // the sweep runs over roughly half the decay, so long toms keep zapping
        pitchCoeff = Dsp::decayCoeff (0.04f + 0.5f * d * params.bend, sampleRate);
    }

    float process() override
    {
        if (! active)
            return 0.0f;

        const float sr = static_cast<float> (sampleRate);
        const float freq = params.tuneHz * (1.0f + (0.3f + 7.0f * params.bend) * pitchEnv);
        pitchEnv *= pitchCoeff;

        phase += freq / sr;
        if (phase >= 1.0f) phase -= 1.0f;
        const float body = std::sin (Dsp::twoPi * phase) * ampEnv;
        ampEnv *= ampCoeff;

        const float click = noiseFilter.process (noise.next()).band * clickEnv * 1.5f * params.noise;
        clickEnv *= clickCoeff;

        const float out = crusher.process (Dsp::drive (body + click, params.drive, 8.0f));

        if (ampEnv < 1.0e-4f)
        {
            if (++quietCount >= 256) active = false;
        }

        return out * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    Params params;
    Dsp::Noise noise;
    Dsp::Svf noiseFilter;
    Dsp::Crusher crusher;

    float phase = 0.0f, ampEnv = 0.0f, pitchEnv = 0.0f, clickEnv = 0.0f;
    float ampCoeff = 0.999f, pitchCoeff = 0.999f, clickCoeff = 0.99f, velocity = 1.0f;
    int quietCount = 0;
    bool active = false;
};

// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include "DspUtils.h"

// Two-operator FM percussion. A sine modulator at (pitch x Ratio) phase-modulates
// a sine carrier. The FM index decays faster than the amplitude, so hits start
// bright and buzzy and settle toward a purer tone - woodblocks and claves at
// whole-number ratios, cowbells / metal / glitch blips at inharmonic ones.
// Harder hits are brighter (velocity scales the index).
class PercVoice : public DrumVoice
{
public:
    struct Params
    {
        float pitchHz  = 800.0f;  // carrier frequency
        float decaySec = 0.08f;   // to -60 dB
        float level    = 0.7f;    // 0..1
        float ratio    = 1.0f;    // modulator / carrier, 0.5 .. 8
        float index    = 0.4f;    // 0..1 -> FM depth 0 .. 8 radians
        float crush    = 0.0f;    // 0..1
    };

    void setParams (const Params& p)
    {
        params = p;
        crusher.setAmount (params.crush, sampleRate);
    }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        pitchCoeff = Dsp::decayCoeff (0.006f, sr);
        setParams (params);
        reset();
    }

    void reset() override
    {
        active = false;
        ampEnv = indexEnv = pitchEnv = 0.0f;
        carrierPhase = modPhase = 0.0f;
        quietCount = 0;
        crusher.reset();
    }

    void trigger (float vel) override
    {
        velocity = vel;
        carrierPhase = modPhase = 0.0f;
        ampEnv = indexEnv = pitchEnv = 1.0f;
        quietCount = 0;
        active = true;

        const float d = std::max (params.decaySec, 0.01f);
        ampCoeff   = Dsp::decayCoeff (d, sampleRate);
        indexCoeff = Dsp::decayCoeff (0.004f + 0.35f * d, sampleRate);
    }

    float process() override
    {
        if (! active)
            return 0.0f;

        const float sr = static_cast<float> (sampleRate);
        const float f = params.pitchHz * (1.0f + 0.5f * pitchEnv);
        pitchEnv *= pitchCoeff;

        modPhase += f * params.ratio / sr;
        modPhase -= std::floor (modPhase);
        carrierPhase += f / sr;
        carrierPhase -= std::floor (carrierPhase);

        const float depth = params.index * 8.0f * (0.5f + 0.5f * velocity) * indexEnv;
        const float mod = depth * std::sin (Dsp::twoPi * modPhase);
        const float out = std::sin (Dsp::twoPi * carrierPhase + mod) * ampEnv;
        ampEnv   *= ampCoeff;
        indexEnv *= indexCoeff;

        if (ampEnv < 1.0e-4f)
        {
            if (++quietCount >= 256) active = false;
        }

        return crusher.process (out) * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    Params params;
    Dsp::Crusher crusher;
    float carrierPhase = 0.0f, modPhase = 0.0f;
    float ampEnv = 0.0f, indexEnv = 0.0f, pitchEnv = 0.0f;
    float ampCoeff = 0.999f, indexCoeff = 0.999f, pitchCoeff = 0.99f, velocity = 1.0f;
    int quietCount = 0;
    bool active = false;
};

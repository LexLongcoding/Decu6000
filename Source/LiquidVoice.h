// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include "DspUtils.h"

// "Liquid Metal": an inharmonic FM tone (metallic) that "drips" and wobbles.
//   DRIP     a rising pitch sweep at the start - the bubble "bloop" of a drop -
//            up to two octaves, and a longer sweep the more you turn it up.
//   MODRATE  speed of an LFO that warbles both pitch and FM brightness, so the
//            tail moves like molten metal rather than ringing still.
// The FM index decays to a floor rather than to zero, so the tail stays metallic.
class LiquidVoice : public DrumVoice
{
public:
    struct Params
    {
        float pitchHz   = 330.0f;
        float decaySec  = 0.7f;
        float level     = 0.6f;
        float drip      = 0.5f;   // 0..1
        float modRateHz = 7.0f;   // LFO speed
    };

    void setParams (const Params& p) { params = p; }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        reset();
    }

    void reset() override
    {
        active = false;
        ampEnv = indexEnv = rise = 0.0f;
        carrierPhase = modPhase = lfoPhase = 0.0f;
        quietCount = 0;
    }

    void trigger (float vel) override
    {
        velocity = vel;
        carrierPhase = modPhase = 0.0f;
        lfoPhase = 0.25f;                 // start the wobble at its peak for a "splash"
        ampEnv = indexEnv = 1.0f;
        rise = 0.0f;
        quietCount = 0;
        active = true;

        const float d = std::max (params.decaySec, 0.03f);
        ampCoeff   = Dsp::decayCoeff (d, sampleRate);
        indexCoeff = Dsp::decayCoeff (0.01f + 0.15f * d, sampleRate);
        riseCoeff  = std::exp (-1.0f / ((0.012f + 0.06f * params.drip) * static_cast<float> (sampleRate)));
    }

    float process() override
    {
        if (! active)
            return 0.0f;

        const float sr = static_cast<float> (sampleRate);
        rise = 1.0f - (1.0f - rise) * riseCoeff;                       // 0 -> 1

        lfoPhase += params.modRateHz / sr;
        lfoPhase -= std::floor (lfoPhase);
        const float lfo = std::sin (Dsp::twoPi * lfoPhase);

        const float sweep = std::exp2 (2.0f * params.drip * (rise - 1.0f));   // starts low, rises to 1
        const float f = params.pitchHz * sweep * (1.0f + 0.015f * lfo);

        modPhase += f * 1.41f / sr;
        modPhase -= std::floor (modPhase);
        carrierPhase += f / sr;
        carrierPhase -= std::floor (carrierPhase);

        const float index = (0.6f + 2.4f * indexEnv) * (1.0f + 0.45f * lfo) * (0.6f + 0.4f * velocity);
        const float out = std::sin (Dsp::twoPi * carrierPhase + index * std::sin (Dsp::twoPi * modPhase)) * ampEnv;
        ampEnv   *= ampCoeff;
        indexEnv *= indexCoeff;

        if (ampEnv < 1.0e-4f)
        {
            if (++quietCount >= 256) active = false;
        }

        return std::tanh (1.2f * out) * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    Params params;
    float carrierPhase = 0.0f, modPhase = 0.0f, lfoPhase = 0.0f;
    float ampEnv = 0.0f, indexEnv = 0.0f, rise = 0.0f;
    float ampCoeff = 0.999f, indexCoeff = 0.999f, riseCoeff = 0.99f, velocity = 1.0f;
    int quietCount = 0;
    bool active = false;
};

// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include "DspUtils.h"

// "Digital Perc": a short pitched blip pushed through a sine wavefolder, with
// a chaotic digital source mixed in.
//   CHAOS  blends in a logistic-map oscillator (deterministic chaos, held for
//          a few samples like a cheap sample-rate converter) and makes the
//          blip's pitch jump between octaves/fifths at random - broken machine.
//   FOLD   drives the signal into a wavefolder: from clean tick (0) to harsh,
//          buzzy, metallic folding (1). Harder hits fold more.
class DigitalVoice : public DrumVoice
{
public:
    struct Params
    {
        float pitchHz  = 420.0f;
        float decaySec = 0.12f;
        float level    = 0.6f;
        float chaos    = 0.35f;   // 0..1
        float fold     = 0.5f;    // 0..1
    };

    void setParams (const Params& p) { params = p; }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        pitchCoeff = Dsp::decayCoeff (0.02f, sr);
        reset();
    }

    void reset() override
    {
        rng.reset();
        active = false;
        env = pitchEnv = 0.0f; phase = 0.0f;
        holdCounter = 0; held = 0.0f; jumpCounter = 0; pitchMult = 1.0f;
        quietCount = 0;
    }

    void trigger (float vel) override
    {
        velocity = vel;
        phase = 0.0f;
        env = pitchEnv = 1.0f;
        logistic = 0.2f + 0.3f * (0.5f + 0.5f * rng.next());   // fresh chaos each hit, in (0.2, 0.5)
        pitchMult = 1.0f;
        jumpCounter = 0;
        quietCount = 0;
        active = true;
        envCoeff = Dsp::decayCoeff (std::max (params.decaySec, 0.01f), sampleRate);
    }

    float process() override
    {
        if (! active)
            return 0.0f;

        const float sr = static_cast<float> (sampleRate);
        const float chaos = params.chaos;

        // random pitch jumps (octaves / fifths), more often with more chaos
        if (--jumpCounter <= 0)
        {
            jumpCounter = static_cast<int> (sr * (0.004f + 0.03f * (1.0f - chaos)));
            static constexpr float steps[] = { 0.5f, 1.0f, 1.0f, 1.5f, 2.0f, 3.0f };
            const float r = 0.5f + 0.5f * rng.next();
            pitchMult = r < chaos * 0.8f ? steps[static_cast<int> ((0.5f + 0.5f * rng.next()) * 5.99f)] : 1.0f;
        }

        const float f = params.pitchHz * pitchMult * (1.0f + 2.0f * pitchEnv);
        pitchEnv *= pitchCoeff;
        phase += f / sr;
        phase -= std::floor (phase);
        const float osc = std::sin (Dsp::twoPi * phase);

        // logistic map x -> r x (1 - x), r near 4 is fully chaotic; sample-and-hold
        if (--holdCounter <= 0)
        {
            holdCounter = 1 + static_cast<int> ((1.0f - chaos) * 6.0f);
            const float r = 3.6f + 0.399f * chaos;
            logistic = r * logistic * (1.0f - logistic);
            if (logistic <= 0.0f || logistic >= 1.0f) logistic = 0.37f;
            held = 2.0f * logistic - 1.0f;
        }

        const float src = osc * (1.0f - 0.6f * chaos) + held * 0.6f * chaos;
        const float drive = 1.0f + (params.fold * 7.0f) * (0.6f + 0.4f * velocity);
        const float folded = std::sin (drive * src * 1.5707963f);   // sine wavefolder

        const float out = folded * env;
        env *= envCoeff;

        if (env < 1.0e-4f)
        {
            if (++quietCount >= 256) active = false;
        }

        return out * 0.8f * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    Params params;
    Dsp::Noise rng { 0xD161A1u };
    float phase = 0.0f, env = 0.0f, pitchEnv = 0.0f, pitchMult = 1.0f;
    float logistic = 0.4f, held = 0.0f;
    float envCoeff = 0.999f, pitchCoeff = 0.99f, velocity = 1.0f;
    int holdCounter = 0, jumpCounter = 0, quietCount = 0;
    bool active = false;
};

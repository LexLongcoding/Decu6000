// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include "DspUtils.h"

// Two-oscillator tuned body (with a short pitch drop) plus filtered noise
// "snares". Drive saturates the sum, then Crush adds sample-rate / bit
// reduction for the glitchy, sampler-through-a-cheap-converter sound.
class SnareVoice : public DrumVoice
{
public:
    struct Params
    {
        float tuneHz   = 190.0f;  // 120..400, body fundamental
        float decaySec = 0.20f;   // body decay to -60 dB; noise follows it
        float snappy   = 0.6f;    // 0..1, noise amount (and noise length)
        float tone     = 0.5f;    // 0..1, noise brightness (high-pass cutoff)
        float drive    = 0.2f;    // 0..1
        float crush    = 0.0f;    // 0..1
        float level    = 0.8f;    // 0..1
    };

    void setParams (const Params& p)
    {
        params = p;
        const float cutoff = 800.0f * std::pow (10.0f, params.tone);        // 800 Hz .. 8 kHz
        noiseHp.setCoefficients (cutoff, 0.7f, sampleRate);
        noiseBp.setCoefficients (cutoff * 1.5f, 1.2f, sampleRate);
        crusher.setAmount (params.crush, sampleRate);
    }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        pitchCoeff = Dsp::decayCoeff (0.012f, sr);
        setParams (params);
        reset();
    }

    void reset() override
    {
        noise.reset();
        active = false;
        bodyEnv = noiseEnv = pitchEnv = 0.0f;
        phase1 = phase2 = 0.0f;
        quietCount = 0;
        noiseHp.reset(); noiseBp.reset(); crusher.reset();
    }

    void trigger (float vel) override
    {
        velocity = vel;
        phase1 = phase2 = 0.0f;
        bodyEnv = noiseEnv = pitchEnv = 1.0f;
        quietCount = 0;
        active = true;

        const float d = std::max (params.decaySec, 0.03f);
        bodyCoeff  = Dsp::decayCoeff (d, sampleRate);
        noiseCoeff = Dsp::decayCoeff (d * (0.6f + 0.9f * params.snappy), sampleRate);
    }

    float process() override
    {
        if (! active)
            return 0.0f;

        const float sr = static_cast<float> (sampleRate);
        const float bend = 1.0f + 0.8f * pitchEnv;
        pitchEnv *= pitchCoeff;

        phase1 += params.tuneHz * bend / sr;          if (phase1 >= 1.0f) phase1 -= 1.0f;
        phase2 += params.tuneHz * 1.62f * bend / sr;  if (phase2 >= 1.0f) phase2 -= 1.0f;
        const float body = (0.65f * std::sin (Dsp::twoPi * phase1)
                          + 0.35f * std::sin (Dsp::twoPi * phase2)) * bodyEnv;
        bodyEnv *= bodyCoeff;

        const float n  = noise.next();
        const auto  hp = noiseHp.process (n);
        const auto  bp = noiseBp.process (n);
        const float snares = (0.45f * hp.high + 0.8f * bp.band) * noiseEnv;
        noiseEnv *= noiseCoeff;

        const float mix = body * (1.0f - 0.3f * params.snappy) + snares * (0.1f + 0.6f * params.snappy);
        const float out = crusher.process (Dsp::drive (mix, params.drive, 8.0f));

        if (bodyEnv < 1.0e-4f && noiseEnv < 1.0e-4f)
        {
            if (++quietCount >= 256) active = false;
        }

        return out * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    Params params;
    Dsp::Noise noise { 0x51A2E00Du };
    Dsp::Svf noiseHp, noiseBp;
    Dsp::Crusher crusher;

    float phase1 = 0.0f, phase2 = 0.0f;
    float bodyEnv = 0.0f, noiseEnv = 0.0f, pitchEnv = 0.0f;
    float bodyCoeff = 0.999f, noiseCoeff = 0.999f, pitchCoeff = 0.99f;
    float velocity = 1.0f;
    int quietCount = 0;
    bool active = false;
};

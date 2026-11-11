// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include "DspUtils.h"
#include <array>

// 808-style metallic hat: six detuned square oscillators blended with noise,
// band-passed then high-passed. One class serves both closed and open hats;
// the processor gives each its own decay and lets the closed hat choke the open.
class HatVoice : public DrumVoice
{
public:
    explicit HatVoice (uint32_t noiseSeed = 0xC0FFEE11u) : noise (noiseSeed) {}

    struct Params
    {
        float tune     = 0.5f;   // 0..1 -> oscillator bank x0.5 .. x2
        float decaySec = 0.06f;   // to -60 dB
        float tone     = 0.5f;    // 0..1, high-pass cutoff 4 .. 12 kHz
        float metal    = 0.7f;    // 0..1, noise -> square bank
        float crush    = 0.0f;    // 0..1
        float level    = 0.6f;    // 0..1
    };

    void setParams (const Params& p)
    {
        params = p;
        const float hpCut = 4000.0f * std::pow (3.0f, params.tone);         // 4 .. 12 kHz
        bandPass.setCoefficients (hpCut * 0.9f, 0.9f, sampleRate);
        highPass.setCoefficients (hpCut, 0.7f, sampleRate);
        crusher.setAmount (params.crush, sampleRate);
    }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        chokeCoeff = Dsp::decayCoeff (0.006f, sr);   // ~6 ms fade, no click
        setParams (params);
        reset();
    }

    void reset() override
    {
        noise.reset();
        active = false;
        ampEnv = 0.0f;
        phases.fill (0.0f);
        quietCount = 0;
        bandPass.reset(); highPass.reset(); crusher.reset();
    }

    void trigger (float vel) override
    {
        velocity = vel;
        ampEnv = 1.0f;
        quietCount = 0;
        active = true;
        ampCoeff = Dsp::decayCoeff (std::max (params.decaySec, 0.01f), sampleRate);
    }

    // Quick fade-out, used when a closed hat cuts off an open one.
    void choke()
    {
        if (active)
            ampCoeff = std::min (ampCoeff, chokeCoeff);
    }

    float process() override
    {
        if (! active)
            return 0.0f;

        const float sr = static_cast<float> (sampleRate);
        const float scale = std::pow (2.0f, 2.0f * params.tune - 1.0f);

        float bank = 0.0f;
        for (size_t i = 0; i < phases.size(); ++i)
        {
            phases[i] += baseFreqs[i] * scale / sr;
            if (phases[i] >= 1.0f) phases[i] -= 1.0f;
            bank += phases[i] < 0.5f ? 1.0f : -1.0f;
        }
        bank *= (1.0f / 6.0f);

        const float src = params.metal * bank + (1.0f - params.metal) * noise.next();
        const float filtered = highPass.process (bandPass.process (src).band).high;

        const float out = crusher.process (filtered * 7.0f * ampEnv);
        ampEnv *= ampCoeff;

        if (ampEnv < 1.0e-4f)
        {
            if (++quietCount >= 256) active = false;
        }

        return std::tanh (out) * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    static constexpr std::array<float, 6> baseFreqs {{ 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f }};

    Params params;
    std::array<float, 6> phases {};
    Dsp::Noise noise;
    Dsp::Svf bandPass, highPass;
    Dsp::Crusher crusher;

    float ampEnv = 0.0f, ampCoeff = 0.999f, chokeCoeff = 0.99f, velocity = 1.0f;
    int quietCount = 0;
    bool active = false;
};

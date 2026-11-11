// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include "DspUtils.h"
#include <array>

// Ring-modulated metallic cymbal. Three pairs of square oscillators are
// multiplied together (ring mod), which spreads energy into dense,
// inharmonic sidebands - the clangy, cheap-sampler-cymbal sound. A noise
// layer adds wash, and an attack burst gives the strike. Crash and Ride are
// two instances with different settings.
class CymbalVoice : public DrumVoice
{
public:
    explicit CymbalVoice (uint32_t noiseSeed = 0xC7A5u) : noise (noiseSeed) {}

    struct Params
    {
        float tune     = 0.5f;    // 0..1 -> oscillator bank x0.5 .. x2
        float decaySec = 1.6f;    // to -60 dB
        float tone     = 0.5f;    // 0..1, high-pass cutoff 2 .. 8 kHz
        float ring     = 0.6f;    // 0..1, noise wash -> ring-mod clang
        float crush    = 0.0f;    // 0..1
        float level    = 0.6f;    // 0..1
    };

    void setParams (const Params& p)
    {
        params = p;
        const float cut = 2000.0f * std::pow (4.0f, params.tone);           // 2 .. 8 kHz
        highPass.setCoefficients (cut, 0.6f, sampleRate);
        presence.setCoefficients (cut * 1.8f, 0.8f, sampleRate);
        crusher.setAmount (params.crush, sampleRate);
    }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        attackCoeff = Dsp::decayCoeff (0.08f, sr);
        chokeCoeff  = Dsp::decayCoeff (0.05f, sr);
        setParams (params);
        reset();
    }

    void reset() override
    {
        noise.reset();
        active = false;
        ampEnv = attackEnv = 0.0f;
        phases.fill (0.0f);
        quietCount = 0;
        highPass.reset(); presence.reset(); crusher.reset();
    }

    void trigger (float vel) override
    {
        velocity = vel;
        ampEnv = attackEnv = 1.0f;
        quietCount = 0;
        active = true;
        ampCoeff = Dsp::decayCoeff (std::max (params.decaySec, 0.1f), sampleRate);
    }

    // Short fade (~50 ms) for hand-grab style muting; not wired to a pad yet.
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

        std::array<float, 6> sq {};
        for (size_t i = 0; i < phases.size(); ++i)
        {
            phases[i] += baseFreqs[i] * scale / sr;
            if (phases[i] >= 1.0f) phases[i] -= 1.0f;
            sq[i] = phases[i] < 0.5f ? 1.0f : -1.0f;
        }
        const float clang = (sq[0] * sq[1] + sq[2] * sq[3] + sq[4] * sq[5]) * (1.0f / 3.0f);

        const float n = noise.next();
        const float src = params.ring * clang + (1.0f - params.ring) * n;

        // body decays with the main envelope; the strike adds extra noise up front
        const float shaped = highPass.process (src).high * ampEnv
                           + presence.process (n).band * attackEnv * 0.8f;
        ampEnv    *= ampCoeff;
        attackEnv *= attackCoeff;

        const float out = crusher.process (shaped * 2.5f);

        if (ampEnv < 1.0e-4f && attackEnv < 1.0e-4f)
        {
            if (++quietCount >= 256) active = false;
        }

        return std::tanh (out) * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    // Pairs chosen so their sum/difference sidebands don't line up harmonically.
    static constexpr std::array<float, 6> baseFreqs {{ 317.0f, 459.3f, 583.1f, 811.7f, 1047.0f, 1373.9f }};

    Params params;
    std::array<float, 6> phases {};
    Dsp::Noise noise;
    Dsp::Svf highPass, presence;
    Dsp::Crusher crusher;

    float ampEnv = 0.0f, attackEnv = 0.0f;
    float ampCoeff = 0.999f, attackCoeff = 0.99f, chokeCoeff = 0.99f, velocity = 1.0f;
    int quietCount = 0;
    bool active = false;
};

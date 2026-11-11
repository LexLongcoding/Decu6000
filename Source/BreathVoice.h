// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include "DspUtils.h"
#include <array>

// "Synthetic Breath": formant synthesis - a robotic throat.
// A buzzy glottal source (band-limited saw) and breath noise go through three
// parallel resonant filters tuned to the formants of a vowel.
//   VOWEL   sweeps the formants U - O - A - E - I (dark to bright).
//   BREATH  blends from a buzzy robot "ah" (0) to a whispered gasp (1).
// Each hit is a short exhale: the vowel opens slightly and the pitch sags.
class BreathVoice : public DrumVoice
{
public:
    struct Params
    {
        float pitchHz  = 140.0f;  // glottal pitch
        float decaySec = 0.35f;
        float level    = 0.7f;
        float vowel    = 0.5f;    // 0..1  U O A E I
        float breath   = 0.4f;    // 0..1
    };

    void setParams (const Params& p) { params = p; }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        attackStep = 1.0f / (0.004f * static_cast<float> (sr));
        glideCoeff = Dsp::decayCoeff (0.12f, sr);
        reset();
    }

    void reset() override
    {
        noise.reset();
        active = false;
        env = 0.0f; attack = 1.0f; glide = 0.0f; phase = 0.0f;
        quietCount = 0;
        for (auto& f : formants) f.reset();
    }

    void trigger (float vel) override
    {
        velocity = vel;
        attack = 0.0f;          // short linear attack, then exponential decay
        env = 1.0f;
        glide = 1.0f;
        quietCount = 0;
        active = true;
        envCoeff = Dsp::decayCoeff (std::max (params.decaySec, 0.03f), sampleRate);
        updateFormants (clamp01 (params.vowel + 0.12f));
    }

    float process() override
    {
        if (! active)
            return 0.0f;

        const float sr = static_cast<float> (sampleRate);

        // formants: vowel position, opening a little at the start of the exhale
        if ((++coeffCounter & 15) == 0)
            updateFormants (clamp01 (params.vowel + 0.12f * glide));

        // glottal source: PolyBLEP sawtooth, pitch sags during the hit
        const float f = params.pitchHz * (1.0f + 0.15f * glide);
        const float dt = f / sr;
        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;
        float saw = 2.0f * phase - 1.0f;
        saw -= polyBlep (phase, dt);
        glide *= glideCoeff;

        const float breathAmt = params.breath;
        const float src = saw * (1.0f - breathAmt) * 0.6f + noise.next() * breathAmt * 1.4f;

        float out = 0.0f;
        for (size_t i = 0; i < formants.size(); ++i)
            out += formants[i].process (src).band * formantGain[i];

        if (attack < 1.0f) attack = std::min (1.0f, attack + attackStep);
        const float a = env * attack;
        env *= envCoeff;

        if (env < 1.0e-4f)
        {
            if (++quietCount >= 256) active = false;
        }

        return std::tanh (out * 2.2f * a) * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    static float clamp01 (float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }

    static float polyBlep (float t, float dt)
    {
        if (t < dt)        { t /= dt;               return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt;   return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    void updateFormants (float pos)
    {
        // U, O, A, E, I  - F1, F2, F3 (Hz)
        static constexpr float table[5][3] = { { 325, 700, 2700 }, { 450, 800, 2830 }, { 800, 1150, 2900 },
                                               { 400, 1600, 2700 }, { 270, 2140, 2950 } };
        static constexpr float bandwidth[3] = { 80.0f, 100.0f, 140.0f };

        const float x = pos * 4.0f;
        const int i0 = std::min (3, static_cast<int> (x));
        const float t = x - static_cast<float> (i0);
        for (size_t k = 0; k < 3; ++k)
        {
            const float fk = table[i0][k] + (table[i0 + 1][k] - table[i0][k]) * t;
            formants[k].setCoefficients (fk, fk / bandwidth[k], sampleRate);
        }
    }

    static constexpr std::array<float, 3> formantGain {{ 1.0f, 0.7f, 0.35f }};

    Params params;
    std::array<Dsp::Svf, 3> formants;
    Dsp::Noise noise { 0xB4EA71u };
    float phase = 0.0f, env = 0.0f, attack = 1.0f, glide = 0.0f;
    float envCoeff = 0.999f, glideCoeff = 0.99f, attackStep = 0.01f, velocity = 1.0f;
    unsigned coeffCounter = 0;
    int quietCount = 0;
    bool active = false;
};

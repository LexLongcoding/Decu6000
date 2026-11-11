// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include "DspUtils.h"
#include <array>

// "Crystal Impact": modal synthesis of a struck glass bar. Eight decaying
// partials, each a recursive (rotating-phasor) oscillator with its decay
// folded into the rotation, so it costs one complex multiply per sample.
//   INHARM  blends the partial ratios from harmonic (1,2,3...) to the
//           stretched ratios of a free bar (1, 2.76, 5.40, 8.93...) - glass.
//   SHIMMER adds a slightly detuned, longer-ringing twin of every partial;
//           the twins beat against the originals so the tail glitters.
// Higher partials die faster, and harder hits excite them more (brighter).
class CrystalVoice : public DrumVoice
{
public:
    struct Params
    {
        float pitchHz  = 880.0f;
        float decaySec = 1.6f;    // fundamental, to -60 dB
        float level    = 0.6f;
        float inharm   = 0.7f;    // 0..1
        float shimmer  = 0.4f;    // 0..1
    };

    void setParams (const Params& p)
    {
        params = p;
        updateCoefficients();
    }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        glintFilter.setCoefficients (7500.0f, 0.9f, sr);
        glintCoeff = Dsp::decayCoeff (0.015f, sr);
        updateCoefficients();
        reset();
    }

    void reset() override
    {
        noise.reset();
        for (auto& m : modes) m.re = m.im = 0.0f;
        glintEnv = 0.0f;
        active = false;
        quietCount = 0;
        glintFilter.reset();
    }

    void trigger (float vel) override
    {
        velocity = vel;
        float sum = 0.0f;
        for (size_t i = 0; i < numPartials; ++i)
        {
            // brighter with velocity: upper partials scaled by vel^(0.35 i)
            const float a = std::pow (static_cast<float> (i) + 1.0f, -0.8f)
                          * std::pow (std::max (vel, 0.05f), 0.35f * static_cast<float> (i));
            const float twinA = a * 0.8f * params.shimmer;
            modes[i].re = modes[i].enabled ? a : 0.0f;                         // start at phase 0 (output = im)
            modes[i].im = 0.0f;
            modes[i + numPartials].re = modes[i + numPartials].enabled ? twinA : 0.0f;
            modes[i + numPartials].im = 0.0f;
            sum += (modes[i].enabled ? a : 0.0f) + (modes[i + numPartials].enabled ? twinA : 0.0f);
        }
        norm = 1.6f / std::max (sum, 0.5f);   // tanh at the output catches the rare full-sum peak
        glintEnv = 1.0f;
        quietCount = 0;
        active = true;
    }

    float process() override
    {
        if (! active)
            return 0.0f;

        float out = 0.0f, energy = 0.0f;
        for (auto& m : modes)
        {
            const float re = m.re * m.cr - m.im * m.ci;
            const float im = m.re * m.ci + m.im * m.cr;
            m.re = re; m.im = im;
            out += im;
            energy += std::abs (re) + std::abs (im);
        }
        out *= norm;

        const float glint = glintFilter.process (noise.next()).band * glintEnv * 0.35f;
        glintEnv *= glintCoeff;

        if (energy * norm < 1.0e-4f && glintEnv < 1.0e-4f)
        {
            if (++quietCount >= 256) active = false;
        }
        else
        {
            quietCount = 0;
        }

        return std::tanh (out + glint) * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    static constexpr size_t numPartials = 8;
    static constexpr std::array<float, numPartials> barRatios {{ 1.0f, 2.756f, 5.404f, 8.933f, 13.34f, 18.64f, 24.81f, 31.87f }};

    struct Mode { float re = 0.0f, im = 0.0f, cr = 1.0f, ci = 0.0f; bool enabled = true; };

    void updateCoefficients()
    {
        const float sr = static_cast<float> (sampleRate);
        for (size_t i = 0; i < numPartials; ++i)
        {
            const float harmonic = static_cast<float> (i) + 1.0f;
            const float ratio = harmonic + (barRatios[i] - harmonic) * params.inharm;
            const float f = params.pitchHz * ratio;
            const float t60 = std::max (params.decaySec, 0.05f) / (1.0f + 0.8f * static_cast<float> (i));

            setMode (modes[i], f, t60, sr);
            setMode (modes[i + numPartials], f * (1.0f + params.shimmer * 0.006f * (1.0f + 0.5f * static_cast<float> (i))),
                     t60 * 1.3f, sr);
        }
    }

    static void setMode (Mode& m, float freq, float t60, float sr)
    {
        m.enabled = freq < 0.45f * sr;
        const float r = std::pow (0.001f, 1.0f / (t60 * sr));
        const float w = Dsp::twoPi * std::min (freq, 0.45f * sr) / sr;
        m.cr = r * std::cos (w);
        m.ci = r * std::sin (w);
        if (! m.enabled) { m.re = m.im = 0.0f; }
    }

    Params params;
    std::array<Mode, numPartials * 2> modes {};
    Dsp::Noise noise { 0xC4157A1u };
    Dsp::Svf glintFilter;
    float glintEnv = 0.0f, glintCoeff = 0.99f, norm = 1.0f, velocity = 1.0f;
    int quietCount = 0;
    bool active = false;
};

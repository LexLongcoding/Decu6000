// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include <algorithm>
#include <array>
#include <cmath>

// Sine kick with a pitch-envelope sweep, a soft-clip drive stage placed
// BEFORE the amp envelope (so the tail stays distorted), and a tuned
// feedback comb + saturator ("Drone") that can hold the tail as a drone.
class KickVoice : public DrumVoice
{
public:
    struct Params
    {
        float pitchHz  = 50.0f;   // 30..120
        float decaySec = 0.5f;    // time to -60 dB
        float level    = 0.8f;    // 0..1
        float drive    = 0.3f;    // 0..1
        float drone    = 0.0f;    // 0..1
    };

    void setParams (const Params& p) { params = p; }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        pitchCoeff = std::exp (-1.0f / (0.030f * static_cast<float> (sr)));
        reset();
    }

    void reset() override
    {
        active = false; ampEnv = 0.0f; pitchEnv = 0.0f; phase = 0.0f;
        quietCount = 0; writePos = 0;
        comb.fill (0.0f);
    }

    void trigger (float vel) override
    {
        velocity = vel;
        phase = 0.0f;
        ampEnv = 1.0f;
        pitchEnv = 1.0f;
        quietCount = 0;
        active = true;
        const float d = std::max (params.decaySec, 0.02f);
        ampCoeff = std::pow (0.001f, 1.0f / (d * static_cast<float> (sampleRate)));
    }

    float process() override
    {
        if (! active)
            return 0.0f;

        const float sr = static_cast<float> (sampleRate);
        const float freq = params.pitchHz * (1.0f + 5.0f * pitchEnv);
        pitchEnv *= pitchCoeff;

        phase += freq / sr;
        if (phase >= 1.0f) phase -= 1.0f;
        const float osc = std::sin (twoPi * phase);

        // drive grows with the decay setting
        const float decayNorm = std::clamp ((params.decaySec - 0.05f) / 2.45f, 0.0f, 1.0f);
        const float gain = 1.0f + params.drive * (2.0f + 10.0f * decayNorm);
        const float shaped = std::tanh (gain * osc) / std::tanh (gain);

        const float dry = shaped * ampEnv;
        ampEnv *= ampCoeff;

        // tuned feedback comb with saturation in the loop
        const int delay = std::clamp (static_cast<int> (std::lround (sr / std::max (params.pitchHz, 20.0f))),
                                      2, static_cast<int> (combSize) - 1);
        const size_t readPos = (writePos + combSize - static_cast<size_t> (delay)) % combSize;
        const float fb = 0.985f * params.drone;
        const float y = dry + fb * std::tanh (comb[readPos]);
        comb[writePos] = y;
        writePos = (writePos + 1) % combSize;

        if (ampEnv < 1.0e-4f && std::abs (y) < 1.0e-4f)
        {
            if (++quietCount >= 2048) active = false;
        }
        else
        {
            quietCount = 0;
        }

        return std::tanh (y) * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    static constexpr float twoPi = 6.28318530718f;
    static constexpr size_t combSize = 1 << 15;

    Params params;
    std::array<float, combSize> comb {};
    size_t writePos = 0;
    float phase = 0.0f, ampEnv = 0.0f, pitchEnv = 0.0f;
    float ampCoeff = 0.999f, pitchCoeff = 0.999f, velocity = 1.0f;
    int quietCount = 0;
    bool active = false;
};

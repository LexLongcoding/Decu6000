// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DrumVoice.h"
#include "DspUtils.h"
#include <array>

// "Bio Burst": a swarm of tiny sound grains released by each hit.
// Grains are short windowed sine bursts (with a little noise) scattered in
// time and pitch while a burst envelope dies away.
//   SWARM  how many grains, and how widely their pitches scatter
//          (sparse chirps at 0, a dense insect swarm at 1).
//   GRAIN  grain length: clicky static (0) to tonal buzzing chirps (1).
// Up to 24 grains overlap; they are all fixed slots, so nothing is allocated.
class BurstVoice : public DrumVoice
{
public:
    struct Params
    {
        float pitchHz  = 1200.0f;  // centre pitch of the swarm
        float decaySec = 0.6f;     // how long grains keep spawning
        float level    = 0.6f;
        float swarm    = 0.5f;     // 0..1
        float grain    = 0.3f;     // 0..1
    };

    void setParams (const Params& p) { params = p; }

    void prepare (double sr) override
    {
        DrumVoice::prepare (sr);
        reset();
    }

    void reset() override
    {
        noise.reset();
        active = false;
        env = 0.0f;
        spawnAccumulator = 0.0f;
        for (auto& g : grains) g.remaining = 0;
        quietCount = 0;
    }

    void trigger (float vel) override
    {
        velocity = vel;
        env = 1.0f;
        spawnAccumulator = 1.0f;     // first grain right away
        quietCount = 0;
        active = true;
        envCoeff = Dsp::decayCoeff (std::max (params.decaySec, 0.05f), sampleRate);
    }

    float process() override
    {
        if (! active)
            return 0.0f;

        const float sr = static_cast<float> (sampleRate);

        // spawn rate follows the burst envelope: 30 .. 900 grains / second at the start
        const float rate = (30.0f + 870.0f * params.swarm) * env;
        spawnAccumulator += rate / sr;
        while (spawnAccumulator >= 1.0f)
        {
            spawnAccumulator -= 1.0f;
            spawn (sr);
        }

        float out = 0.0f;
        bool anyGrain = false;
        for (auto& g : grains)
        {
            if (g.remaining <= 0) continue;
            anyGrain = true;
            g.phase += g.inc;
            g.phase -= std::floor (g.phase);
            const float w = std::sin (Dsp::pi * g.windowPos);            // half-sine window
            g.windowPos += g.windowInc;
            out += (std::sin (Dsp::twoPi * g.phase) * 0.85f + noise.next() * 0.15f) * w * g.amp;
            --g.remaining;
        }
        env *= envCoeff;

        if (env < 1.0e-3f && ! anyGrain)
        {
            if (++quietCount >= 256) active = false;
        }

        return std::tanh (out * 0.9f) * params.level * velocity;
    }

    bool isActive() const override { return active; }

private:
    struct Grain
    {
        float phase = 0.0f, inc = 0.0f, windowPos = 0.0f, windowInc = 0.0f, amp = 0.0f;
        int remaining = 0;
    };

    void spawn (float sr)
    {
        // reuse a free slot, or the one closest to finishing
        Grain* slot = &grains[0];
        for (auto& g : grains)
        {
            if (g.remaining <= 0) { slot = &g; break; }
            if (g.remaining < slot->remaining) slot = &g;
        }

        const float spreadOct = 0.15f + 1.6f * params.swarm;
        const float f = std::min (params.pitchHz * std::exp2 (spreadOct * noise.next()), 0.45f * sr);
        const float lengthSec = 0.0015f + 0.045f * params.grain * params.grain + 0.004f * params.grain;
        const int len = std::max (8, static_cast<int> (lengthSec * sr * (0.6f + 0.4f * (0.5f + 0.5f * noise.next()))));

        slot->phase = 0.0f;
        slot->inc = f / sr;
        slot->windowPos = 0.0f;
        slot->windowInc = 1.0f / static_cast<float> (len);
        slot->remaining = len;
        slot->amp = (0.5f + 0.5f * (0.5f + 0.5f * noise.next())) * (0.4f + 0.6f * env);
    }

    Params params;
    std::array<Grain, 24> grains {};
    Dsp::Noise noise { 0xB0B57u };
    float env = 0.0f, envCoeff = 0.999f, spawnAccumulator = 0.0f, velocity = 1.0f;
    int quietCount = 0;
    bool active = false;
};

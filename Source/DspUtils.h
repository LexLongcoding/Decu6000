// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

// Small, allocation-free DSP helpers shared by the Decu6000 voices.
// Kept JUCE-free so the voices can be unit-tested on their own.
namespace Dsp
{
    constexpr float pi    = 3.14159265359f;
    constexpr float twoPi = 6.28318530718f;

    // Coefficient for a one-pole exponential decay reaching -60 dB after `seconds`.
    inline float decayCoeff (float seconds, double sampleRate) noexcept
    {
        const float s = std::max (seconds, 0.001f);
        return std::pow (0.001f, 1.0f / (s * static_cast<float> (sampleRate)));
    }

    // Fast deterministic white noise (xorshift32), output in [-1, 1).
    class Noise
    {
    public:
        explicit Noise (uint32_t seed = 0x9E3779B9u) : initial (seed ? seed : 1u), state (initial) {}

        void reset() noexcept { state = initial; }   // repeatable after All Sound Off

        float next() noexcept
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return static_cast<float> (static_cast<int32_t> (state)) * (1.0f / 2147483648.0f);
        }

    private:
        uint32_t initial, state;
    };

    // Topology-preserving-transform state variable filter (Zavalishin / Simper).
    // One call gives low, band and high outputs.
    class Svf
    {
    public:
        struct Out { float low, band, high; };

        void setCoefficients (float cutoffHz, float q, double sampleRate) noexcept
        {
            const float sr = static_cast<float> (sampleRate);
            const float fc = std::clamp (cutoffHz, 10.0f, 0.45f * sr);
            const float g = std::tan (pi * fc / sr);
            k  = 1.0f / std::max (q, 0.1f);
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
        }

        void reset() noexcept { ic1 = ic2 = 0.0f; }

        Out process (float x) noexcept
        {
            const float v3 = x - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            return { v2, v1, x - k * v1 - v2 };
        }

    private:
        float k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
        float ic1 = 0.0f, ic2 = 0.0f;
    };

    // Lo-fi "Crush": sample-rate reduction (sample & hold) plus bit-depth reduction.
    // amount 0 = bypass, 1 = roughly 3 kHz / 4-bit.
    class Crusher
    {
    public:
        void setAmount (float amount, double sampleRate) noexcept
        {
            amt = std::clamp (amount, 0.0f, 1.0f);
            const float srScale = static_cast<float> (sampleRate) / 44100.0f;
            holdLength = (1.0f + amt * amt * 14.0f) * srScale;   // in samples
            levels = std::pow (2.0f, 16.0f - 12.0f * amt);      // 16-bit .. 4-bit
        }

        void reset() noexcept { counter = 1.0e9f; held = 0.0f; }   // latch on next sample

        float process (float x) noexcept
        {
            if (amt <= 0.0f)
                return x;

            counter += 1.0f;
            if (counter >= holdLength)
            {
                counter -= holdLength;
                if (counter >= holdLength) counter = 0.0f;   // after reset / hold change
                held = std::round (x * levels) / levels;
            }
            return held;
        }

    private:
        float amt = 0.0f, holdLength = 1.0f, levels = 65536.0f;
        float counter = 1.0e9f, held = 0.0f;
    };

    // Transparent below `knee`, then rolls off smoothly toward 1.0 (slope-continuous).
    // Used on the master bus so stacked hits saturate instead of hard-clipping.
    inline float softLimit (float x, float knee = 0.8f) noexcept
    {
        const float a = std::abs (x);
        if (a <= knee)
            return x;
        const float headroom = 1.0f - knee;
        const float y = knee + headroom * std::tanh ((a - knee) / headroom);
        return x < 0.0f ? -y : y;
    }

    // Normalised tanh drive: unity at full scale regardless of gain.
    inline float drive (float x, float amount, float maxGain = 10.0f) noexcept
    {
        const float g = 1.0f + amount * maxGain;
        return std::tanh (g * x) / std::tanh (g);
    }
}

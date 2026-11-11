// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

// Classic gated reverb on the send bus: predelay -> large stereo room -> a gate
// that opens when something hits the send, holds for GATE T, then slams shut.
// All buffers are allocated in prepare(); process() does not allocate.
class GatedReverb
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        sr = sampleRate;
        delay.assign (static_cast<size_t> (sampleRate * 0.3) + 2, 0.0f);   // up to 250 ms predelay
        wetL.assign (static_cast<size_t> (juce::jmax (1, maxBlockSize)), 0.0f);
        wetR = wetL;
        gain = wetL;

        juce::Reverb::Parameters p;
        p.roomSize = 0.88f;
        p.damping  = 0.35f;
        p.wetLevel = 1.0f;
        p.dryLevel = 0.0f;
        p.width    = 1.0f;
        reverb.setParameters (p);
        reverb.setSampleRate (sampleRate);

        attackCoeff  = std::exp (-1.0f / (0.001f * static_cast<float> (sampleRate)));
        releaseCoeff = std::exp (-1.0f / (0.012f * static_cast<float> (sampleRate)));
        reset();
    }

    void reset()
    {
        std::fill (delay.begin(), delay.end(), 0.0f);
        reverb.reset();
        writePos = 0;
        holdSamples = 0;
        gateEnv = 0.0f;
    }

    // Adds the gated wet signal (x returnLevel) into outL / outR.
    void process (const float* send, float* outL, float* outR, int numSamples,
                  float predelaySec, float gateSec, float returnLevel)
    {
        const int chunk = static_cast<int> (wetL.size());
        for (int start = 0; start < numSamples; start += chunk)
            processChunk (send + start, outL + start, outR != nullptr ? outR + start : nullptr,
                          juce::jmin (chunk, numSamples - start), predelaySec, gateSec, returnLevel);
    }

    bool isGateOpen() const { return gateEnv > 0.01f; }

private:
    void processChunk (const float* send, float* outL, float* outR, int n,
                       float predelaySec, float gateSec, float returnLevel)
    {
        const int size = static_cast<int> (delay.size());
        const int delaySamples = juce::jlimit (0, size - 2, juce::roundToInt (predelaySec * static_cast<float> (sr)));
        const int hold = juce::roundToInt (gateSec * static_cast<float> (sr));

        for (int i = 0; i < n; ++i)
        {
            const float x = send[i];
            delay[static_cast<size_t> (writePos)] = x;
            const int readPos = (writePos - delaySamples + size) % size;
            writePos = (writePos + 1) % size;
            wetL[static_cast<size_t> (i)] = wetR[static_cast<size_t> (i)] = delay[static_cast<size_t> (readPos)];

            // gate: retriggers on every hit above threshold, holds, then closes fast
            if (std::abs (x) > threshold) holdSamples = hold + delaySamples;
            else if (holdSamples > 0)     --holdSamples;
            const float target = holdSamples > 0 ? 1.0f : 0.0f;
            const float c = target > gateEnv ? attackCoeff : releaseCoeff;
            gateEnv = target + (gateEnv - target) * c;
            gain[static_cast<size_t> (i)] = gateEnv;
        }

        reverb.processStereo (wetL.data(), wetR.data(), n);

        for (int i = 0; i < n; ++i)
        {
            const float g = gain[static_cast<size_t> (i)] * returnLevel;
            outL[i] += wetL[static_cast<size_t> (i)] * g;
            if (outR != nullptr) outR[i] += wetR[static_cast<size_t> (i)] * g;
        }
    }

    static constexpr float threshold = 0.02f;   // about -34 dB on the send

    double sr = 44100.0;
    juce::Reverb reverb;
    std::vector<float> delay, wetL, wetR, gain;
    int writePos = 0, holdSamples = 0;
    float gateEnv = 0.0f, attackCoeff = 0.9f, releaseCoeff = 0.99f;
};

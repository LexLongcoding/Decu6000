// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include "DspUtils.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

// The Bio-Neural Engine: a global instability layer over the whole kit.
//
// LUCID / UNSTABLE sets how unstable the engine gets; BLEND sets how much of it
// you hear (dry/wet for the audio effects, and it scales PROB / MICRO-T too).
// With either at 0 the engine is asleep and fully bypassed (output untouched). Turned up, it "awakens": an awakening level
// rises as you keep playing and relaxes when you stop, and the effective
// instability is  I = lucid * (0.3 + 0.7 * awakening)  plus a little jitter.
// Every effect below is scaled by I:
//   GRAIN    chance of glitch stutters (a short slice of the output repeats)
//   CRUSH    sample-rate reduction        BITS   bit-depth reduction
//   RATE     speed of a drifting tape-style pitch "wow" (random walk, not a sine)
//   MICRO-T  random micro-timing push on each hit       (handled via onHit)
//   PROB     chance each hit plays at all               (handled via onHit)
//   SEED     restarts the random generator: same seed -> same glitches
//
// All memory is allocated in prepare(); process() does not allocate.
class BioEngine
{
public:
    struct Params
    {
        float lucid   = 0.0f;    // 0..1 how unstable
        float blend   = 1.0f;    // 0..1 how much of the engine you hear (0 = off)
        float grain   = 0.3f;    // 0..1
        float crush   = 0.3f;    // 0..1
        float bits    = 0.3f;    // 0..1
        float rateHz  = 1.5f;    // wow drift speed
        float microT  = 0.012f;  // seconds, max push
        float prob    = 1.0f;    // 0..1 chance a hit plays
        int   seed    = 4096;
    };

    static constexpr int historySize = 128;   // instability curve for the editor (~6.4 s)

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        capture.assign (static_cast<size_t> (sampleRate * 2.0), 0.0f);      // 2 s stutter memory (mono)
        wowL.assign (static_cast<size_t> (sampleRate * 0.03) + 4, 0.0f);    // 30 ms wow delay
        wowR = wowL;
        (void) maxBlock;
        historyInterval = static_cast<int> (sampleRate * 0.05);              // one point / 50 ms
        mixStep = 1.0f / (0.05f * static_cast<float> (sampleRate));          // 50 ms bypass crossfade
        reset();
    }

    void reset()
    {
        std::fill (capture.begin(), capture.end(), 0.0f);
        std::fill (wowL.begin(), wowL.end(), 0.0f);
        std::fill (wowR.begin(), wowR.end(), 0.0f);
        capPos = wowPos = 0;
        stutterRemaining = 0;
        awakening = 0.0f;
        wowValue = wowTarget = wowStart = 0.0f; wowPhase = 1.0f;
        holdCounter = 1.0e9f; heldL = heldR = 0.0f;
        historyCounter = historyInterval;
        reseed (params.seed);
    }

    void setParams (const Params& p)
    {
        if (p.seed != params.seed)
            reseed (p.seed);
        params = p;
    }

    // Called for every incoming hit. Returns false if PROB drops it; otherwise
    // sets the micro-timing delay (in samples) to apply before triggering.
    bool onHit (int& delaySamples)
    {
        delaySamples = 0;
        if (! isAwake())
            return true;

        awakening += 0.045f * (1.0f - awakening);
        const float amt = std::min (1.0f, currentInstability() * 1.5f);

        const float dropChance = (1.0f - params.prob) * amt * params.blend;
        if (uniform() < dropChance)
            return false;

        delaySamples = static_cast<int> (params.microT * amt * params.blend * uniform() * static_cast<float> (sr));
        return true;
    }

    // In-place on the stereo mix. outR may be null (mono).
    void process (float* L, float* R, int n)
    {
        const bool asleep = ! isAwake();
        if (asleep && mix <= 0.0f)
        {
            advanceIdle (n);
            return;                                          // true bypass
        }

        const float awakeDecay = std::exp (-1.0f / (6.0f * static_cast<float> (sr)));
        const int capSize = static_cast<int> (capture.size());
        const int wowSize = static_cast<int> (wowL.size());

        for (int i = 0; i < n; ++i)
        {
            awakening *= awakeDecay;
            const float inst = currentInstability();

            // ---- control: jitter + history for the editor -------------------
            if (--historyCounter <= 0)
            {
                historyCounter = historyInterval;
                jitter += 0.35f * (displayNoise.next() * 0.15f - jitter);   // display only - separate RNG
                pushHistory (std::clamp (inst * (1.0f + jitter), 0.0f, 1.0f), glitchFlag);
                glitchFlag = false;
            }

            const float dryL = L[i];
            const float dryR = R != nullptr ? R[i] : dryL;
            float l = dryL, r = dryR;

            // ---- stutter (GRAIN) ---------------------------------------------
            capture[static_cast<size_t> (capPos)] = 0.5f * (l + r);
            if (stutterRemaining <= 0)
            {
                // chance per sample tuned so GRAIN 1 at full instability ~ 3 stutters / s
                const float chance = params.grain * params.grain * inst * 3.0f / static_cast<float> (sr);
                if (uniform() < chance)
                    startStutter (capSize);
            }
            if (stutterRemaining > 0)
            {
                const int pos = (sliceStart + sliceOffset) % capSize;
                // soft edges on every repeated slice, and a fade in/out against the dry signal
                const float sliceEdge = std::min ({ 1.0f, static_cast<float> (sliceOffset) / fadeLen,
                                                    static_cast<float> (sliceLen - sliceOffset) / fadeLen });
                const float inOut = std::min ({ 1.0f, static_cast<float> (stutterTotal - stutterRemaining) / fadeLen,
                                                static_cast<float> (stutterRemaining) / fadeLen });
                const float stut = capture[static_cast<size_t> (pos)] * sliceEdge;
                l = l + (stut - l) * inOut;
                r = r + (stut - r) * inOut;
                if (++sliceOffset >= sliceLen) sliceOffset = 0;
                --stutterRemaining;
            }
            capPos = (capPos + 1) % capSize;

            // ---- wow (RATE): random-walk modulated delay ---------------------
            wowPhase += params.rateHz / static_cast<float> (sr);
            if (wowPhase >= 1.0f)
            {
                wowPhase -= 1.0f;
                wowStart = wowTarget;
                wowTarget = uniformBipolar();
            }
            const float smooth = wowPhase * wowPhase * (3.0f - 2.0f * wowPhase);
            wowValue = wowStart + (wowTarget - wowStart) * smooth;
            const float delaySamples = static_cast<float> (sr) * (0.008f + 0.004f * inst * wowValue);

            wowL[static_cast<size_t> (wowPos)] = l;
            wowR[static_cast<size_t> (wowPos)] = r;
            l = readFrac (wowL, wowPos, delaySamples, wowSize);
            r = readFrac (wowR, wowPos, delaySamples, wowSize);
            wowPos = (wowPos + 1) % wowSize;

            // ---- crush (CRUSH = sample rate, BITS = bit depth) ---------------
            // crush & bits answer LUCID directly (full effect once half-awake)
            const float fx = std::min (1.0f, inst * 1.6f);
            const float c = params.crush * fx;
            const float holdLen = (1.0f + c * c * 20.0f) * static_cast<float> (sr) / 44100.0f;
            holdCounter += 1.0f;
            if (holdCounter >= holdLen)
            {
                holdCounter -= holdLen;
                if (holdCounter >= holdLen) holdCounter = 0.0f;
                heldL = l; heldR = r;
            }
            if (c > 0.0005f) { l = heldL; r = heldR; }

            const float b = params.bits * fx;
            if (b > 0.0005f)
            {
                const float levels = std::exp2 (std::round (16.0f - 13.0f * b));   // whole bit depths, 16 .. 3
                l = std::round (l * levels) / levels;
                r = std::round (r * levels) / levels;
            }

            // ---- crossfade with dry so waking / sleeping never clicks --------
            const float target = asleep ? 0.0f : params.blend;
            mix = mix < target ? std::min (target, mix + mixStep) : std::max (target, mix - mixStep);
            if (mix >= 1.0f)
            {
                L[i] = l;
                if (R != nullptr) R[i] = r;
            }
            else
            {
                L[i] = dryL + (l - dryL) * mix;
                if (R != nullptr) R[i] = dryR + (r - dryR) * mix;
            }
        }
    }

    // Editor access (message thread): instability history 0..1 and glitch marks.
    float getHistory (int indexFromOldest, bool& glitch) const
    {
        const int w = historyWrite.load (std::memory_order_acquire);
        const int idx = (w + indexFromOldest) % historySize;
        glitch = historyGlitch[static_cast<size_t> (idx)].load (std::memory_order_relaxed);
        return history[static_cast<size_t> (idx)].load (std::memory_order_relaxed);
    }
    int getStutterCount() const { return stutterCount.load(); }
    bool isAwake() const        { return params.lucid > 0.0f && params.blend > 0.0f; }

private:
    float currentInstability() const { return params.lucid * (0.3f + 0.7f * awakening); }

    void reseed (int seed)
    {
        rng = static_cast<uint32_t> (seed) * 2654435761u + 0x9E3779B9u;
        if (rng == 0) rng = 1;
    }
    float uniform()        { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                             return static_cast<float> (rng >> 8) * (1.0f / 16777216.0f); }
    float uniformBipolar() { return uniform() * 2.0f - 1.0f; }

    void startStutter (int capSize)
    {
        static constexpr float sliceMs[] = { 23.0f, 46.0f, 62.5f, 93.75f, 125.0f };
        const float ms = sliceMs[static_cast<int> (uniform() * 4.999f)];
        sliceLen = std::max (64, static_cast<int> (ms * 0.001f * static_cast<float> (sr)));
        const int repeats = 2 + static_cast<int> (uniform() * 6.0f);
        stutterRemaining = stutterTotal = sliceLen * repeats;
        sliceStart = (capPos - sliceLen + capSize) % capSize;   // the slice that just played
        sliceOffset = 0;
        glitchFlag = true;
        stutterCount.fetch_add (1);
    }

    static float readFrac (const std::vector<float>& buf, int writePos, float delay, int size)
    {
        float rp = static_cast<float> (writePos) - delay;
        while (rp < 0.0f) rp += static_cast<float> (size);
        const int i0 = static_cast<int> (rp);
        const float t = rp - static_cast<float> (i0);
        const float a = buf[static_cast<size_t> (i0 % size)], b = buf[static_cast<size_t> ((i0 + 1) % size)];
        return a + (b - a) * t;
    }

    void advanceIdle (int n)
    {
        // keep the history scrolling (flat) while asleep
        historyCounter -= n;
        while (historyCounter <= 0)
        {
            historyCounter += historyInterval;
            pushHistory (0.0f, false);
        }
        awakening = 0.0f;
    }

    void pushHistory (float v, bool glitch)
    {
        const int w = historyWrite.load (std::memory_order_relaxed);
        history[static_cast<size_t> (w)].store (v, std::memory_order_relaxed);
        historyGlitch[static_cast<size_t> (w)].store (glitch, std::memory_order_relaxed);
        historyWrite.store ((w + 1) % historySize, std::memory_order_release);
    }

    Params params;
    double sr = 44100.0;
    uint32_t rng = 1;
    Dsp::Noise displayNoise { 0x0D15EA5Eu };

    std::vector<float> capture, wowL, wowR;
    int capPos = 0, wowPos = 0;
    int stutterRemaining = 0, stutterTotal = 1, sliceLen = 1, sliceStart = 0, sliceOffset = 0;
    static constexpr float fadeLen = 96.0f;

    float awakening = 0.0f, jitter = 0.0f, mix = 0.0f, mixStep = 0.001f;
    float wowValue = 0.0f, wowTarget = 0.0f, wowStart = 0.0f, wowPhase = 1.0f;
    float holdCounter = 1.0e9f, heldL = 0.0f, heldR = 0.0f;

    int historyInterval = 2205, historyCounter = 0;
    bool glitchFlag = false;
    std::array<std::atomic<float>, historySize> history {};
    std::array<std::atomic<bool>, historySize> historyGlitch {};
    std::atomic<int> historyWrite { 0 }, stutterCount { 0 };
};

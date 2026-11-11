// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

// Minimal base class shared by every Decu6000 voice (mono output).
class DrumVoice
{
public:
    virtual ~DrumVoice() = default;

    virtual void prepare (double newSampleRate) { sampleRate = newSampleRate; }
    virtual void reset() = 0;
    virtual void trigger (float velocity) = 0;   // velocity 0..1
    virtual float process() = 0;                 // one mono sample
    virtual bool isActive() const = 0;

protected:
    double sampleRate = 44100.0;
};

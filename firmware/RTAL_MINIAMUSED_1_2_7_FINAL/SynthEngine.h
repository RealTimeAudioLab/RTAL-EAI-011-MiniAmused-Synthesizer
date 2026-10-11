#pragma once
#include <Arduino.h>
#include <atomic>

class SynthEngine {
public:
    static SynthEngine& instance();

    bool begin();
    void runAudioTask();

    void noteOn(uint8_t note, uint8_t velocity);
    void noteOff(uint8_t note);
    void allNotesOff();
    void pitchBend(int bend);

    int currentNote() const { return _note; }
    bool gateActive() const { return _gate; }

    uint32_t audioBlockCount() const { return _audioBlocks; }
    uint32_t audioWriteErrors() const { return _audioWriteErrors; }
    uint32_t audioShortWrites() const { return _audioShortWrites; }
    uint32_t dspAverageUs() const { return _dspAverageUs; }
    uint32_t dspPeakUs() const { return _dspPeakUs; }
    uint8_t activeVoiceCount() const { return _activeVoiceCount; }
    void resetDspPeak() { _dspPeakUs = 0; }

private:
    SynthEngine() = default;

    volatile int _note = 48;
    volatile int _velocity = 100;
    volatile bool _gate = false;
    volatile int _pitchBend = 0;
    std::atomic<uint32_t> _triggerSerial {0};

    volatile uint32_t _audioBlocks = 0;
    volatile uint32_t _audioWriteErrors = 0;
    volatile uint32_t _audioShortWrites = 0;
    volatile uint32_t _dspAverageUs = 0;
    volatile uint32_t _dspPeakUs = 0;
    volatile uint8_t _activeVoiceCount = 0;

    // DUAL BENCH allocator state. Voice 0/1 are independent oscillator pitches;
    // filter, envelopes, VCA and delay remain global/shared.
    volatile bool _dualActive[2] = {false, false};
    volatile uint8_t _dualNote[2] = {48, 48};
    volatile uint8_t _dualVelocity[2] = {100, 100};
    volatile uint32_t _dualAge[2] = {0, 0};


    // Held-note stack for monophonic priority handling.
    volatile uint8_t _heldCount[128] = {};
    volatile uint32_t _age[128] = {};
    volatile uint32_t _ageCounter = 1;

    void selectHeldNote();
    void rebuildDualVoices(uint8_t preferredNote = 255);
};

#pragma once
#include <Arduino.h>

class ArpEngine {
public:
    static ArpEngine& instance();
    void noteOn(uint8_t note,uint8_t velocity);
    void noteOff(uint8_t note);
    void allNotesOff();
    void service();
    void reset();
    uint8_t heldCount() const { return _heldCount; }
    int currentNote() const { return _outNote; }
private:
    ArpEngine()=default;
    struct Held { uint8_t note=0, vel=100; bool down=false; bool keyDown=false; };
    Held _held[128]{};
    uint8_t _heldCount=0;
    int _outNote=-1;
    int _step=0, _rhythmStep=0, _dir=1, _lastIndex=0, _repeatIndex=0;
    uint32_t _lastClock=0, _stepStartUs=0, _stepPeriodUs=125000;
    uint32_t _nextStepUs=0, _nextRatchetUs=0, _ratchetPeriodUs=0;
    uint8_t _ratchetTotal=1, _ratchetIndex=0, _ratchetNote=0, _ratchetVel=100;
    bool _gateOn=false, _wasEnabled=false, _wasRunning=false, _schedulerPrimed=false, _wasMidiSync=true, _wasHold=false, _lastAccent=false;
    uint32_t _rng=0x13579BDFu;
    uint32_t rnd();
    int collect(uint8_t *notes,uint8_t *vels) const;
    int chooseIndex(int n,int mode);
    void triggerStep();
    void triggerVoice(uint8_t note,uint8_t vel,uint32_t subPeriodUs);
    void releaseOutput();
    void releaseLatchedNotes();
};

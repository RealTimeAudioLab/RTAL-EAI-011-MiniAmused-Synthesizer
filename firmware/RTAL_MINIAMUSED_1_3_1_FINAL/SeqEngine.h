#pragma once
#include <Arduino.h>
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

// MiniAmused v1.3.0 C: two 16-step patterns, independently editable,
// note articulation + sync. Only Core-0 RTAL_ARP owns the synth note output.
// No SD serialization until D. No blocking work in this real-time class.
class SeqEngine {
public:
    struct Step {
        uint8_t note=48;
        uint8_t velocity=100;
        uint8_t gate=75;
        uint8_t flags=0;
    };
    enum : uint8_t { REST=1, TIE=2, SLIDE=4, ACCENT=8 };
    enum PlayMode : uint8_t { A=0, B=1, AB=2, AABB=3 };
    enum Rate : uint8_t { QUARTER=0, EIGHTH=1, SIXTEENTH=2, THIRTYSECOND=3 };
    enum RecordMode : uint8_t { REC_OFF=0, REC_STEP=1, REC_LIVE=2 };

    struct Snapshot {
        Step patterns[2][16];
        uint8_t lengths[2];
        uint8_t mode;
        uint8_t rate;
        uint16_t tempo;
        uint8_t swing;
        uint8_t root;
        bool keyTranspose;
        bool midiSync;
    };
    static Snapshot defaultSnapshot();
    Snapshot snapshot() const;
    void applySnapshot(const Snapshot &value);
    void resetForPreset();

    static SeqEngine &instance();
    void begin();
    void service(); // only RTAL_ARP Core-0 task
    void setPlaying(bool enabled);
    bool playing() const { return _requested.load(std::memory_order_acquire); }
    bool running() const { return _running.load(std::memory_order_acquire); }

    bool midiSync() const;
    void setMidiSync(bool on);
    uint16_t tempo() const;
    void setTempo(uint16_t bpm);
    uint8_t length(uint8_t variation) const;
    void setLength(uint8_t variation,uint8_t len);
    PlayMode mode() const;
    void setMode(PlayMode mode);
    Step step(uint8_t variation,uint8_t index) const;
    void setStep(uint8_t variation,uint8_t index,const Step &step);
    void copyAtoB();
    uint8_t currentStep() const { return _currentStep.load(std::memory_order_relaxed); }
    uint8_t currentVariation() const { return _currentVariation.load(std::memory_order_relaxed); }

    uint8_t swing() const;
    void setSwing(uint8_t amount); // 0..45 percent of one grid interval
    Rate rate() const;
    void setRate(Rate rate);
    bool keyTranspose() const;
    void setKeyTranspose(bool enabled);
    uint8_t root() const;
    void setRoot(uint8_t root);
    int8_t transposeSemitones() const { return _transpose.load(std::memory_order_relaxed); }
    void noteInput(uint8_t note,uint8_t velocity); // MIDI task; no synth output
    // Recording controls are independent of the SEQ transport. A MIDI task
    // only performs bounded, locked 4-byte step writes; LVGL never runs there.
    RecordMode recordMode() const { return (RecordMode)_recordMode.load(std::memory_order_acquire); }
    void setRecordMode(RecordMode mode);
    void setRecordCursor(uint8_t variation,uint8_t index);
    uint8_t recordCursorStep() const { return (uint8_t)(_recordCursor.load(std::memory_order_acquire)&255U); }
    uint8_t recordCursorVariation() const { return (uint8_t)(_recordCursor.load(std::memory_order_acquire)>>8); }
    uint32_t recordRevision() const { return _recordRevision.load(std::memory_order_acquire); }
    uint8_t recordedStep() const { return (uint8_t)(_lastRecordedCell.load(std::memory_order_acquire)&255U); }
    uint8_t recordedVariation() const { return (uint8_t)(_lastRecordedCell.load(std::memory_order_acquire)>>8); }

private:
    SeqEngine()=default;
    mutable portMUX_TYPE _mux=portMUX_INITIALIZER_UNLOCKED;
    Step _pattern[2][16]{};
    uint8_t _length[2]={16,16};
    PlayMode _mode=AB;
    Rate _rate=SIXTEENTH;
    uint16_t _tempo=120;
    uint8_t _swing=0;
    uint8_t _root=60;
    bool _keyTranspose=false;
    bool _midiSync=false;
    std::atomic<int8_t> _transpose{0};
    std::atomic<uint8_t> _lastKey{60};
    std::atomic<bool> _haveKey{false};

    std::atomic<uint8_t> _recordMode{REC_OFF};
    std::atomic<uint16_t> _recordCursor{0}; // variation << 8 | step
    std::atomic<uint16_t> _lastRecordedCell{0};
    std::atomic<uint32_t> _recordRevision{0};
    std::atomic<uint32_t> _lastStepOnsetUs{0};
    std::atomic<uint32_t> _lastStepPeriodUs{125000};
    std::atomic<uint16_t> _liveCurrentCell{0};
    std::atomic<uint16_t> _liveNextCell{0};

    std::atomic<bool> _requested{false};
    std::atomic<bool> _running{false};
    std::atomic<uint8_t> _currentStep{255};
    std::atomic<uint8_t> _currentVariation{255};
    bool _active=false;
    bool _wasSync=false;
    uint8_t _wasRate=SIXTEENTH;
    uint32_t _lastTransportRevision=0;
    uint32_t _lastClock=0;
    bool _waitFirstClock=false;
    uint8_t _clockTicks=0;
    uint32_t _nextStepUs=0;
    uint32_t _gateOffUs=0;
    uint32_t _pendingStepUs=0;
    bool _pendingStep=false;
    int _outNote=-1;
    bool _gateOn=false;
    bool _tieHold=false;
    uint8_t _position=0;
    uint8_t _cycle=0;
    uint8_t _variation=0;
    uint32_t _stepCount=0;

    void releaseNote();
    void resetPosition();
    void advancePosition();
    void fireStep(uint32_t basePeriodUs);
    uint32_t stepBasePeriodUs(uint16_t bpm, Rate rate) const;
    uint32_t stepIntervalUs(uint32_t basePeriodUs, uint8_t swing) const;
};

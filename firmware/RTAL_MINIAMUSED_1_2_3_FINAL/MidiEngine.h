#pragma once
#include <Arduino.h>
#include <atomic>
#include "ParameterIDs.h"

enum class MidiMapMode : uint8_t { STANDARD=0, MOPHO=1 };

class MidiEngine {
public:
    static MidiEngine& instance();
    void begin();
    void poll();

    bool takeProgramChange(uint8_t &program);

    uint32_t rxCount() const { return _rxCount; }
    uint32_t noteOnCount() const { return _noteOnCount; }
    uint32_t noteOffCount() const { return _noteOffCount; }
    uint32_t ccCount() const { return _ccCount; }
    uint32_t pitchBendCount() const { return _pitchBendCount; }
    uint8_t lastCC() const { return _lastCC; }
    uint8_t lastCCValue() const { return _lastCCValue; }
    uint32_t mappedCCCount() const { return _mappedCCCount; }
    uint32_t programChangeCount() const { return _programChangeCount; }
    uint32_t clockCount() const { return _clockCount; }
    uint32_t dinClockCount() const { return _dinClockCount; }
    uint32_t usbClockCount() const { return _usbClockCount; }
    uint32_t usbRxCount() const { return _usbRxCount; }
    bool transportRunning() const { return _transportRunning; }
    bool clockActive() const;
    float clockBpm() const { return _clockBpm; }

    // v0.8.1: 0=OMNI, 1..16=fixed receive channel. Applies equally to DIN/USB.
    uint8_t channelSetting() const { return _midiChannel.load(std::memory_order_relaxed); }
    void setChannelSetting(uint8_t channel);
    bool acceptsChannel(uint8_t channel) const;

    // v1.1.1: selectable MIDI mapping profile + persistent MIDI Learn.
    MidiMapMode mapMode() const { return (MidiMapMode)_mapMode.load(std::memory_order_relaxed); }
    void setMapMode(MidiMapMode mode);
    uint8_t ccBank() const { return _ccBank.load(std::memory_order_relaxed); }
    void startMidiLearn(ParameterID id);
    void cancelMidiLearn();
    bool midiLearnActive() const { return _learnActive.load(std::memory_order_relaxed); }
    ParameterID midiLearnParameter() const { return (ParameterID)_learnParam.load(std::memory_order_relaxed); }
    bool takeMidiLearnResult(ParameterID &id, uint8_t &bank, uint8_t &cc, MidiMapMode &mode);
    void resetCurrentMidiMap();
    void _setCCBankInternal(uint8_t bank);
    void _completeMidiLearnInternal(ParameterID id,uint8_t bank,uint8_t cc,MidiMapMode mode);

    void countNoteOn() { ++_rxCount; ++_noteOnCount; }
    void countNoteOff() { ++_rxCount; ++_noteOffCount; }
    void countCC() { ++_rxCount; ++_ccCount; }
    void recordMappedCC(uint8_t cc, uint8_t value)
    {
        _lastCC=cc;
        _lastCCValue=value;
        ++_mappedCCCount;
    }
    void countPitchBend() { ++_rxCount; ++_pitchBendCount; }

    void recordProgramChange(uint8_t program, bool fromUSB);
    void recordClock(bool fromUSB);
    void recordTransport(uint8_t status, bool fromUSB);
    void countUsbMessage() { ++_usbRxCount; }

private:
    MidiEngine() = default;

    volatile uint32_t _rxCount = 0;
    volatile uint32_t _noteOnCount = 0;
    volatile uint32_t _noteOffCount = 0;
    volatile uint32_t _ccCount = 0;
    volatile uint32_t _pitchBendCount = 0;
    volatile uint32_t _mappedCCCount = 0;
    volatile uint8_t _lastCC = 0;
    volatile uint8_t _lastCCValue = 0;
    volatile uint32_t _programChangeCount = 0;
    volatile uint32_t _clockCount = 0;
    volatile uint32_t _dinClockCount = 0;
    volatile uint32_t _usbClockCount = 0;
    volatile uint32_t _usbRxCount = 0;
    volatile bool _transportRunning = false;
    volatile uint32_t _lastClockUs = 0;
    volatile uint32_t _prevClockUs = 0;
    volatile float _clockBpm = 120.0f;
    std::atomic<int32_t> _pendingProgram {-1};
    std::atomic<uint8_t> _midiChannel {0};
    std::atomic<uint8_t> _mapMode {0};
    std::atomic<uint8_t> _ccBank {0};
    std::atomic<bool> _learnActive {false};
    std::atomic<uint16_t> _learnParam {0};
    std::atomic<int32_t> _learnResult {-1}; // packed mode/bank/cc/parameter
};

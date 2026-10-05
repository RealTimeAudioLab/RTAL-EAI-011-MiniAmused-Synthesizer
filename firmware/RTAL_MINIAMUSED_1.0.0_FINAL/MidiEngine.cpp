#include "MidiEngine.h"
#include "MidiCCMap.h"
#include "SynthEngine.h"
#include "Parameters.h"
#include "RTAL_IO_PINS.h"

#include <MIDI.h>
#include "soc/soc_caps.h"

#if !SOC_USB_OTG_SUPPORTED
#error "RTAL MiniMoog USB MIDI requires an ESP32-S3/S2 native USB OTG target"
#endif

#include "USB.h"
#include "USBMIDI.h"
#include <Preferences.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI);
USBMIDI USBMidi("RTAL MIDI");

// v0.5.8a - MIDI Real-Time Priority Fix
// Continuous CC streams are coalesced per scheduler pass. Note On/Off,
// All Notes Off, Pitch Bend, Program Change and realtime messages stay
// immediate. This prevents dense controller sweeps from generating hundreds
// of parameter/UI atomic updates before a queued Note Off can be serviced.
static uint8_t s_pendingCCValue[128] = {};
static bool    s_pendingCC[128] = {};

static const MidiCCEntry* findMappedCC(uint8_t cc)
{
    for(const auto &e : kMidiCCMap)
        if(e.cc == cc)
            return &e;
    return nullptr;
}

static float normalizedCCValue(ParameterID id, uint8_t value)
{
    float normalized = value / 127.0f;

    // MASTER TUNE CC116: widened center detent retained unchanged.
    if(id == ParameterID::MASTER_TUNE) {
        if(value >= 60 && value <= 67)
            normalized = 0.5f;
        else if(value < 60)
            normalized = 0.5f * ((float)value / 60.0f);
        else
            normalized = 0.5f + 0.5f * ((float)(value - 67) / 60.0f);
    }
    return normalized;
}

static void flushPendingCC()
{
    // Apply at most one value per mapped CC per 1 ms MIDI scheduler pass.
    // The newest value wins. No DSP calibration or smoothing is changed.
    for(const auto &e : kMidiCCMap) {
        const uint8_t cc = e.cc;
        if(!s_pendingCC[cc])
            continue;

        const uint8_t value = s_pendingCCValue[cc];
        s_pendingCC[cc] = false;

        Parameters::instance().set(
            e.id,
            normalizedCCValue(e.id, value),
            ParameterSource::MIDI
        );
    }
}

static void routeNoteOn(uint8_t note, uint8_t vel, bool fromUSB)
{
    note &= 0x7F;
    vel  &= 0x7F;

    if(fromUSB)
        MidiEngine::instance().countUsbMessage();

    if(vel==0) {
        MidiEngine::instance().countNoteOff();
        SynthEngine::instance().noteOff(note);
    } else {
        MidiEngine::instance().countNoteOn();
        SynthEngine::instance().noteOn(note,vel);
    }
}

static void routeNoteOff(uint8_t note, bool fromUSB)
{
    note &= 0x7F;

    if(fromUSB)
        MidiEngine::instance().countUsbMessage();

    MidiEngine::instance().countNoteOff();
    SynthEngine::instance().noteOff(note);
}

static void routePitchBend(int bend, bool fromUSB)
{
    // v0.5.8: hard clamp malformed/out-of-range input before it reaches DSP.
    if(bend < -8192) bend = -8192;
    if(bend >  8191) bend =  8191;

    if(fromUSB)
        MidiEngine::instance().countUsbMessage();

    MidiEngine::instance().countPitchBend();
    SynthEngine::instance().pitchBend(bend);
}

static void routeCC(uint8_t cc, uint8_t value, bool fromUSB)
{
    cc &= 0x7F;
    value &= 0x7F;

    if(fromUSB)
        MidiEngine::instance().countUsbMessage();

    MidiEngine::instance().countCC();

    // Safety messages must never wait behind a controller sweep.
    if(cc==120 || cc==123) {
        SynthEngine::instance().allNotesOff();
        return;
    }

    // v0.5.8a: mapped continuous/controller traffic is coalesced. We still
    // count every received mapped CC for diagnostics, but only the newest
    // value of each CC is committed to Parameters at the end of this poll.
    if(findMappedCC(cc)) {
        s_pendingCCValue[cc] = value;
        s_pendingCC[cc] = true;
        MidiEngine::instance().recordMappedCC(cc,value);
    }
}

static void routeProgramChange(uint8_t program, bool fromUSB)
{
    program &= 0x7F;

    if(fromUSB)
        MidiEngine::instance().countUsbMessage();

    MidiEngine::instance().recordProgramChange(program,fromUSB);
}

static void routeClock(bool fromUSB)
{
    MidiEngine::instance().recordClock(fromUSB);
}

static void routeTransport(uint8_t status, bool fromUSB)
{
    MidiEngine::instance().recordTransport(status,fromUSB);
}

// DIN callbacks ---------------------------------------------------------------
static void onNoteOn(byte ch, byte note, byte vel)
{
    if(!MidiEngine::instance().acceptsChannel((uint8_t)ch)) return;
    routeNoteOn(note,vel,false);
}

static void onNoteOff(byte ch, byte note, byte vel)
{
    (void)vel;
    if(!MidiEngine::instance().acceptsChannel((uint8_t)ch)) return;
    routeNoteOff(note,false);
}

static void onPitchBend(byte ch, int bend)
{
    if(!MidiEngine::instance().acceptsChannel((uint8_t)ch)) return;
    routePitchBend(bend,false);
}

static void onCC(byte ch, byte cc, byte value)
{
    if(!MidiEngine::instance().acceptsChannel((uint8_t)ch)) return;
    routeCC(cc,value,false);
}

static void onProgramChange(byte ch, byte program)
{
    if(!MidiEngine::instance().acceptsChannel((uint8_t)ch)) return;
    routeProgramChange(program,false);
}

static void onClock()
{
    routeClock(false);
}

static void onStart()
{
    routeTransport(0xFA,false);
}

static void onContinue()
{
    routeTransport(0xFB,false);
}

static void onStop()
{
    routeTransport(0xFC,false);
}

static void pollUsbMidi()
{
    midiEventPacket_t p={0,0,0,0};

    // v0.5.8a: drain a bounded burst each 1 ms scheduler pass. 64 packets gives
    // substantially more headroom for dense CC/clock traffic without allowing
    // USB MIDI to monopolize Core 0 indefinitely.
    for(int guard=0;guard<64;guard++) {
        if(!USBMidi.readPacket(&p))
            break;

        const uint8_t status=p.byte1;

        // USB MIDI realtime / single byte.
        if(status==0xF8) {
            routeClock(true);
            continue;
        }
        if(status==0xFA || status==0xFB || status==0xFC) {
            routeTransport(status,true);
            continue;
        }

        const uint8_t type=status & 0xF0;
        const uint8_t channel=(status & 0x0F) + 1U;

        // Channel voice messages obey the same global OMNI/CH1..16 filter as DIN.
        // System realtime above deliberately bypasses this filter.
        if(type>=0x80 && type<=0xE0 && !MidiEngine::instance().acceptsChannel(channel))
            continue;

        switch(type) {
            case 0x80:
                routeNoteOff(p.byte2,true);
                break;

            case 0x90:
                routeNoteOn(p.byte2,p.byte3,true);
                break;

            case 0xB0:
                routeCC(p.byte2,p.byte3,true);
                break;

            case 0xC0:
                routeProgramChange(p.byte2,true);
                break;

            case 0xE0: {
                const int bend=
                    ((int)(p.byte2 & 0x7F) |
                    ((int)(p.byte3 & 0x7F)<<7)) - 8192;
                routePitchBend(bend,true);
                break;
            }

            default:
                // Other USB-MIDI messages are deliberately ignored in v0.5.8.
                MidiEngine::instance().countUsbMessage();
                break;
        }
    }
}

MidiEngine& MidiEngine::instance()
{
    static MidiEngine m;
    return m;
}

void MidiEngine::begin()
{
    // v0.8.1: global MIDI receive channel is device configuration, not a sound preset.
    Preferences prefs;
    prefs.begin("rtal-midi", true);
    uint8_t saved=prefs.getUChar("rxch",0);
    prefs.end();
    if(saved>16) saved=0;
    _midiChannel.store(saved,std::memory_order_relaxed);
    if(saved==0) Serial.println("[MIDI CH] OMNI"); else Serial.printf("[MIDI CH] CH%u\n",(unsigned)saved);

    // DIN MIDI
    Serial2.begin(31250,SERIAL_8N1,MIDI_RX_PIN,MIDI_TX_PIN);
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff();
    MIDI.setHandleNoteOn(onNoteOn);
    MIDI.setHandleNoteOff(onNoteOff);
    MIDI.setHandleControlChange(onCC);
    MIDI.setHandlePitchBend(onPitchBend);
    MIDI.setHandleProgramChange(onProgramChange);
    MIDI.setHandleClock(onClock);
    MIDI.setHandleStart(onStart);
    MIDI.setHandleContinue(onContinue);
    MIDI.setHandleStop(onStop);

    // Native ESP32-S3 USB Device identity.
    USB.productName("RTAL MiniMoog");
    USB.manufacturerName("Real Time Audio Lab");

    USBMidi.begin();
    USB.begin();
}

void MidiEngine::poll()
{
    // v0.5.8a: drain a bounded DIN burst before USB. Arduino MIDI read() returns
    // true for a completed accepted message. The bound prevents a permanently
    // busy DIN stream from starving USB/UI work on Core 0.
    for(int guard=0; guard<16; ++guard) {
        if(!MIDI.read())
            break;
    }

    pollUsbMidi();

    // Commit controller state only after both input streams have been drained.
    // Therefore Note Off / Note On / realtime traffic encountered in the same
    // burst is handled before the expensive parameter/UI dirty propagation.
    flushPendingCC();
}


bool MidiEngine::acceptsChannel(uint8_t channel) const
{
    const uint8_t wanted=_midiChannel.load(std::memory_order_relaxed);
    return wanted==0 || channel==wanted;
}

void MidiEngine::setChannelSetting(uint8_t channel)
{
    if(channel>16) channel=0;

    const uint8_t oldChannel=_midiChannel.load(std::memory_order_relaxed);
    if(channel==oldChannel) return;

    // v0.8.1b MIDI-CHANNEL-NOTE-FIX1:
    // Release every note accepted under the old channel policy BEFORE the
    // new channel filter becomes active. Otherwise its later Note-Off may be
    // rejected after CH1 -> CH2 (or OMNI/channel) and leave a hanging note.
    // SynthEngine::allNotesOff() drops the held-note state and gate only; the
    // normal amp/filter RELEASE envelopes continue, so this is not a hard mute.
    SynthEngine::instance().allNotesOff();

    _midiChannel.store(channel,std::memory_order_relaxed);

    Preferences prefs;
    if(prefs.begin("rtal-midi",false)) {
        prefs.putUChar("rxch",channel);
        prefs.end();
    }
    if(channel==0) Serial.println("[MIDI CH] set OMNI"); else Serial.printf("[MIDI CH] set CH%u\n",(unsigned)channel);
}

void MidiEngine::recordProgramChange(uint8_t program, bool fromUSB)
{
    (void)fromUSB;
    ++_rxCount;
    ++_programChangeCount;

    // Full MIDI Program Change range maps 1:1 to preset slots 000..127.
    _pendingProgram.store((int32_t)(program & 0x7F),std::memory_order_release);
}

bool MidiEngine::takeProgramChange(uint8_t &program)
{
    const int32_t p=_pendingProgram.exchange(-1,std::memory_order_acq_rel);
    if(p<0 || p>127)
        return false;

    program=(uint8_t)p;
    return true;
}

void MidiEngine::recordClock(bool fromUSB)
{
    ++_rxCount;
    ++_clockCount;

    const uint32_t now=micros();
    const uint32_t prev=_lastClockUs;
    _prevClockUs=prev;
    _lastClockUs=now;
    if(prev!=0) {
        const uint32_t dt=now-prev;
        // MIDI clock = 24 pulses per quarter note. Reject obvious glitches.
        if(dt>=5000U && dt<=100000U) {
            const float inst=60000000.0f/(24.0f*(float)dt);
            _clockBpm += 0.05f*(inst-_clockBpm);
        }
    }

    if(fromUSB) {
        ++_usbClockCount;
        ++_usbRxCount;
    } else {
        ++_dinClockCount;
    }
}

void MidiEngine::recordTransport(uint8_t status, bool fromUSB)
{
    ++_rxCount;
    if(fromUSB)
        ++_usbRxCount;

    if(status==0xFA || status==0xFB)
        _transportRunning=true;
    else if(status==0xFC)
        _transportRunning=false;
}


bool MidiEngine::clockActive() const
{
    const uint32_t last=_lastClockUs;
    return last!=0 && (uint32_t)(micros()-last)<300000U;
}

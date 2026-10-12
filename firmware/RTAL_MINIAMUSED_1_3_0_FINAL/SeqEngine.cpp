#include "SeqEngine.h"
#include "ArpEngine.h"
#include "SynthEngine.h"
#include "MidiEngine.h"
#include "Parameters.h"

SeqEngine &SeqEngine::instance() { static SeqEngine s; return s; }

void SeqEngine::begin() {
    portENTER_CRITICAL(&_mux);
    for(uint8_t v=0;v<2;++v) {
        _length[v]=16;
        for(uint8_t i=0;i<16;++i) {
            Step &s=_pattern[v][i];
            s.note=(uint8_t)(48+(i%4==0?0:i%4==1?7:i%4==2?12:3)+(v?5:0));
            s.velocity=100; s.gate=75;
            s.flags=(i%4==3)?REST:0;
        }
    }
    _mode=AB; _tempo=120; _rate=SIXTEENTH; _swing=0;
    _midiSync=false; _root=60; _keyTranspose=false;
    portEXIT_CRITICAL(&_mux);
    _transpose.store(0,std::memory_order_release);
    _haveKey.store(false,std::memory_order_release);
    _lastKey.store(60,std::memory_order_release);
    _recordMode.store(REC_OFF);
    _recordCursor.store(0);_lastRecordedCell.store(0);
    _recordRevision.store(0);_lastStepOnsetUs.store(0);
    _requested.store(false); _running.store(false);
    _active=false;_gateOn=false;_outNote=-1;_tieHold=false;
    _pendingStep=false;
    resetPosition();
}

SeqEngine::Snapshot SeqEngine::defaultSnapshot() {
    Snapshot snap{};
    for (uint8_t v=0; v<2; ++v) {
        snap.lengths[v]=16;
        for (uint8_t i=0; i<16; ++i) {
            Step &step=snap.patterns[v][i];
            step.note=(uint8_t)(48+(i%4==0?0:i%4==1?7:i%4==2?12:3)+(v?5:0));
            step.velocity=100; step.gate=75;
            step.flags=(i%4==3)?REST:0;
        }
    }
    snap.mode=AB; snap.rate=SIXTEENTH; snap.tempo=120;
    snap.swing=0; snap.root=60; snap.keyTranspose=false;
    snap.midiSync=false;
    return snap;
}

SeqEngine::Snapshot SeqEngine::snapshot() const {
    Snapshot snap{};
    portENTER_CRITICAL(&_mux);
    for (int v=0; v<2; ++v) {
        snap.lengths[v]=_length[v];
        for (int i=0; i<16; ++i) snap.patterns[v][i]=_pattern[v][i];
    }
    snap.mode=(uint8_t)_mode; snap.rate=(uint8_t)_rate;
    snap.tempo=_tempo; snap.swing=_swing; snap.root=_root;
    snap.keyTranspose=_keyTranspose; snap.midiSync=_midiSync;
    portEXIT_CRITICAL(&_mux);
    return snap;
}

void SeqEngine::applySnapshot(const Snapshot &snap) {
    // Called by the UI task only after the realtime scheduler has stopped.
    // Keep the critical region bounded and never access SD from inside it.
    _requested.store(false,std::memory_order_release);
    _recordMode.store(REC_OFF,std::memory_order_release);
    portENTER_CRITICAL(&_mux);
    for (int v=0; v<2; ++v) {
        _length[v]=snap.lengths[v];
        for (int i=0; i<16; ++i) _pattern[v][i]=snap.patterns[v][i];
    }
    _mode=(PlayMode)snap.mode; _rate=(Rate)snap.rate;
    _tempo=snap.tempo; _swing=snap.swing; _root=snap.root;
    _keyTranspose=snap.keyTranspose; _midiSync=snap.midiSync;
    portEXIT_CRITICAL(&_mux);
    _haveKey.store(false,std::memory_order_release);
    _lastKey.store(snap.root,std::memory_order_release);
    _transpose.store(0,std::memory_order_release);
    _recordCursor.store(0,std::memory_order_release);
    _recordRevision.fetch_add(1,std::memory_order_acq_rel);
}

void SeqEngine::resetForPreset() { applySnapshot(defaultSnapshot()); }

void SeqEngine::setPlaying(bool enable) { _requested.store(enable,std::memory_order_release); }
void SeqEngine::setRecordMode(RecordMode mode) {
    if(mode>REC_LIVE)mode=REC_OFF;
    // Input capture is never enabled mid-way through unannounced mode changes.
    _recordMode.store((uint8_t)mode,std::memory_order_release);
}
void SeqEngine::setRecordCursor(uint8_t variation,uint8_t index) {
    if(variation>1)return;
    if(index>=16)index=15;
    _recordCursor.store((uint16_t)((variation<<8)|index),std::memory_order_release);
}
bool SeqEngine::midiSync() const { portENTER_CRITICAL(&_mux);bool v=_midiSync;portEXIT_CRITICAL(&_mux);return v; }
void SeqEngine::setMidiSync(bool on) { portENTER_CRITICAL(&_mux);_midiSync=on;portEXIT_CRITICAL(&_mux); }
uint16_t SeqEngine::tempo() const { portENTER_CRITICAL(&_mux);uint16_t n=_tempo;portEXIT_CRITICAL(&_mux);return n; }
void SeqEngine::setTempo(uint16_t n) { if(n<40)n=40;if(n>300)n=300;portENTER_CRITICAL(&_mux);_tempo=n;portEXIT_CRITICAL(&_mux); }
uint8_t SeqEngine::length(uint8_t v) const { if(v>1)return 16;portENTER_CRITICAL(&_mux);uint8_t n=_length[v];portEXIT_CRITICAL(&_mux);return n; }
void SeqEngine::setLength(uint8_t v,uint8_t n) { if(v>1)return;if(n<1)n=1;if(n>16)n=16;portENTER_CRITICAL(&_mux);_length[v]=n;portEXIT_CRITICAL(&_mux); }
SeqEngine::PlayMode SeqEngine::mode() const { portENTER_CRITICAL(&_mux);PlayMode n=_mode;portEXIT_CRITICAL(&_mux);return n; }
void SeqEngine::setMode(PlayMode m) { if(m>AABB)return;portENTER_CRITICAL(&_mux);_mode=m;portEXIT_CRITICAL(&_mux); }
SeqEngine::Step SeqEngine::step(uint8_t v,uint8_t i) const { Step s{};if(v>1||i>=16)return s;portENTER_CRITICAL(&_mux);s=_pattern[v][i];portEXIT_CRITICAL(&_mux);return s; }
void SeqEngine::setStep(uint8_t v,uint8_t i,const Step &s) {
    if(v>1||i>=16)return;
    Step x=s;
    if(x.note>127)x.note=127;
    if(x.velocity<1)x.velocity=1;
    if(x.velocity>127)x.velocity=127;
    if(x.gate<1)x.gate=1;
    if(x.gate>99)x.gate=99; // avoid overlap at exact boundaries
    x.flags&=(REST|TIE|SLIDE|ACCENT);
    // REST and TIE are mutually exclusive. A TIE inherits previous pitch.
    if(x.flags&REST)x.flags&=~(TIE|SLIDE|ACCENT);
    if(x.flags&TIE)x.flags&=~(SLIDE|ACCENT);
    portENTER_CRITICAL(&_mux);_pattern[v][i]=x;portEXIT_CRITICAL(&_mux);
}
void SeqEngine::copyAtoB() { portENTER_CRITICAL(&_mux);for(int i=0;i<16;i++)_pattern[1][i]=_pattern[0][i];_length[1]=_length[0];portEXIT_CRITICAL(&_mux); }
uint8_t SeqEngine::swing() const { portENTER_CRITICAL(&_mux);auto v=_swing;portEXIT_CRITICAL(&_mux);return v; }
void SeqEngine::setSwing(uint8_t v) { if(v>45)v=45;portENTER_CRITICAL(&_mux);_swing=v;portEXIT_CRITICAL(&_mux); }
SeqEngine::Rate SeqEngine::rate() const { portENTER_CRITICAL(&_mux);auto r=_rate;portEXIT_CRITICAL(&_mux);return r; }
void SeqEngine::setRate(Rate r) { if(r>THIRTYSECOND)return;portENTER_CRITICAL(&_mux);_rate=r;portEXIT_CRITICAL(&_mux); }
bool SeqEngine::keyTranspose() const { portENTER_CRITICAL(&_mux);auto v=_keyTranspose;portEXIT_CRITICAL(&_mux);return v; }
void SeqEngine::setKeyTranspose(bool v) { portENTER_CRITICAL(&_mux);_keyTranspose=v;portEXIT_CRITICAL(&_mux); }
uint8_t SeqEngine::root() const { portENTER_CRITICAL(&_mux);auto r=_root;portEXIT_CRITICAL(&_mux);return r; }
void SeqEngine::setRoot(uint8_t n) {
    if(n>127)n=127;
    portENTER_CRITICAL(&_mux);_root=n;portEXIT_CRITICAL(&_mux);
    if(_haveKey.load(std::memory_order_acquire)) {
        int offset=(int)_lastKey.load(std::memory_order_acquire)-(int)n;
        if(offset>63)offset=63;
        if(offset< -63)offset= -63;
        _transpose.store((int8_t)offset,std::memory_order_release);
    }
}
void SeqEngine::noteInput(uint8_t n,uint8_t velocity) {
    if(!velocity)return;
    if(n>127)n=127;
    const RecordMode rec=recordMode();
    if(rec!=REC_OFF) {
        uint16_t cell=0;
        if(rec==REC_STEP) {
            cell=_recordCursor.load(std::memory_order_acquire);
        } else {
            // Quantize NOTE ON to the nearest current/next step onset.
            // The clock owns grid positions; MIDI never alters scheduler state.
            if(!running())return;
            const uint32_t at=_lastStepOnsetUs.load(std::memory_order_acquire);
            if(!at)return;
            const uint32_t period=_lastStepPeriodUs.load(std::memory_order_acquire);
            const uint32_t age=(uint32_t)(micros()-at);
            cell=(age > period/2U && age<period*2U)
                ?_liveNextCell.load(std::memory_order_acquire)
                :_liveCurrentCell.load(std::memory_order_acquire);
        }
        const uint8_t v=(uint8_t)(cell>>8),i=(uint8_t)cell;
        if(v>1||i>=16)return;
        Step s=step(v,i);
        s.note=n;s.velocity=velocity;
        // Recorded NOTE ON replaces REST/TIE/SLIDE but retains ACCENT/GATE.
        s.flags&=(uint8_t)~(REST|TIE|SLIDE);
        setStep(v,i,s);
        _lastRecordedCell.store(cell,std::memory_order_release);
        if(rec==REC_STEP) {
            uint8_t next=(uint8_t)(i+1);
            if(next>=length(v))next=0;
            _recordCursor.store((uint16_t)((v<<8)|next),std::memory_order_release);
        }
        _recordRevision.fetch_add(1,std::memory_order_acq_rel);
        return;
    }
    // Key transpose retains the B behavior when recording is OFF.
    const int rootNote=root();
    int offset=(int)n-rootNote;
    if(offset>63)offset=63;
    if(offset< -63)offset= -63;
    _lastKey.store(n,std::memory_order_release);
    _haveKey.store(true,std::memory_order_release);
    _transpose.store((int8_t)offset,std::memory_order_release);
}

void SeqEngine::releaseNote() {
    if(_gateOn && _outNote>=0)SynthEngine::instance().noteOff((uint8_t)_outNote);
    _gateOn=false;_outNote=-1;_tieHold=false;
    SynthEngine::instance().setSequencerSlide(false);
}
void SeqEngine::resetPosition() {
    _position=0;_cycle=0;_stepCount=0;
    _variation=(mode()==B)?1:0;
    _pendingStep=false;
    _currentStep.store(255);_currentVariation.store(255);
}
void SeqEngine::advancePosition() {
    const uint8_t len=length(_variation);
    if(++_position<len)return;
    _position=0;
    const PlayMode m=mode();
    if(m==A)_variation=0;
    else if(m==B)_variation=1;
    else if(m==AB)_variation^=1;
    else { _cycle=(uint8_t)((_cycle+1)%4);_variation=_cycle>=2?1:0; }
}
uint32_t SeqEngine::stepBasePeriodUs(uint16_t bpm,Rate r) const {
    static const uint32_t multiplier[4]={60000000UL,30000000UL,15000000UL,7500000UL};
    return multiplier[(uint8_t)r]/(bpm?bpm:120);
}
uint32_t SeqEngine::stepIntervalUs(uint32_t base,uint8_t sw) const {
    const uint32_t delta=(base*(uint32_t)sw)/100U;
    // _stepCount refers to the number of emitted grid steps. After step 0,
    // wait longer for step 1; after step 1 wait shorter for step 2.
    return (_stepCount&1U)?base+delta:base-delta;
}
void SeqEngine::fireStep(uint32_t basePeriodUs) {
    if(_position>=length(_variation))_position=0;
    const uint8_t v=_variation,pos=_position;
    const Step st=step(v,pos);
    const uint32_t now=micros();
    const bool rest=(st.flags&REST)!=0;
    const bool tie=(st.flags&TIE)!=0;
    const bool slide=(st.flags&SLIDE)!=0;
    _currentVariation.store(v,std::memory_order_relaxed);
    _currentStep.store(pos,std::memory_order_relaxed);
    _liveCurrentCell.store((uint16_t)((v<<8)|pos),std::memory_order_release);
    _lastStepPeriodUs.store(basePeriodUs,std::memory_order_release);
    _lastStepOnsetUs.store(now,std::memory_order_release);

    if(rest){releaseNote();}
    else if(tie) {
        // TIE means: extend a preceding active note, never retrigger it.
        // Leading ties with no note are silent.
        if(!_gateOn)SynthEngine::instance().setSequencerSlide(false);
    } else {
        const int shift=keyTranspose()?(int)_transpose.load(std::memory_order_acquire):0;
        int nn=(int)st.note+shift;
        if(nn<0)nn=0;
        if(nn>127)nn=127;
        int vel=st.velocity;
        if(st.flags&ACCENT) {vel+=24;if(vel>127)vel=127;}
        const bool legato=slide && _gateOn;
        if(legato) {
            // Keep gate high. Synth explicitly suppresses MULTI retrigger.
            SynthEngine::instance().setSequencerSlide(true);
            SynthEngine::instance().noteOnLegato((uint8_t)nn,(uint8_t)vel);
            SynthEngine::instance().noteOff((uint8_t)_outNote);
        } else {
            releaseNote();
            SynthEngine::instance().setSequencerSlide(false);
            SynthEngine::instance().noteOn((uint8_t)nn,(uint8_t)vel);
        }
        _outNote=nn;_gateOn=true;
        // This VCA scaling is active ONLY while SEQ owns the note path.
        // Keep a safe bounded range under accents and high velocity.
        float gain=0.35f+0.65f*((float)vel/127.0f);
        if(st.flags&ACCENT)gain*=1.12f;
        if(gain>1.12f)gain=1.12f;
        SynthEngine::instance().setSequencerGain(gain);
    }
    ++_stepCount;
    advancePosition();
    _liveNextCell.store((uint16_t)((_variation<<8)|_position),std::memory_order_release);
    const Step following=step(_variation,_position);
    _tieHold=_gateOn && (tie || (!(following.flags&REST) && (following.flags&(TIE|SLIDE))));
    if(_gateOn && !_tieHold) {
        const uint32_t interval=stepIntervalUs(basePeriodUs,swing());
        const uint32_t onUs=(interval*(uint32_t)st.gate)/100U;
        _gateOffUs=now+(onUs<1000?1000:onUs);
    }
}

void SeqEngine::service() {
    MidiEngine &m=MidiEngine::instance();
    const bool arpOn=Parameters::instance().target(ParameterID::ARP_ENABLE)>=0.5f;
    if(arpOn && (playing() || recordMode()!=REC_OFF)) {
        // An explicitly enabled ARP owns incoming MIDI; disarm SEQ recording.
        _requested.store(false,std::memory_order_release);
        setRecordMode(REC_OFF);
    }
    const bool selected=playing();
    if(!selected && _active) {
        releaseNote();
        SynthEngine::instance().setSequencerGain(1.0f);
        _active=false;_running.store(false);
        _currentStep.store(255);_currentVariation.store(255);
        _pendingStep=false;
        SynthEngine::instance().allNotesOff();
    }
    if(!selected) {ArpEngine::instance().service();return;}
    if(!_active) {
        ArpEngine::instance().allNotesOff();
        _active=true;_wasSync=!midiSync();_wasRate=(uint8_t)rate();
        _lastTransportRevision=m.transportRevision();
        _lastClock=m.clockCount();_clockTicks=0;_waitFirstClock=true;
        resetPosition();_nextStepUs=micros();
    }
    const bool sync=midiSync();
    const Rate r=rate();
    if(sync!=_wasSync || (uint8_t)r!=_wasRate) {
        releaseNote();resetPosition();
        _lastClock=m.clockCount();_clockTicks=0;_waitFirstClock=true;
        _nextStepUs=micros();_wasSync=sync;_wasRate=(uint8_t)r;
    }
    const uint32_t now=micros();
    if(sync) {
        const uint32_t revision=m.transportRevision();
        if(revision!=_lastTransportRevision) {
            _lastTransportRevision=revision;
            const uint8_t status=m.lastTransportStatus();
            if(status==0xFA) {
                releaseNote();resetPosition();_lastClock=m.clockCount();
                _clockTicks=0;_waitFirstClock=true;
            } else if(status==0xFB) {
                _lastClock=m.clockCount();_clockTicks=0;_waitFirstClock=true;
                _pendingStep=false;
            } else if(status==0xFC) {
                releaseNote();_pendingStep=false;
            }
        }
        const bool transport=m.transportRunning();
        const bool activeClock=m.clockActive();
        _running.store(transport&&activeClock,std::memory_order_release);
        if(!transport||!activeClock) {
            releaseNote();_pendingStep=false;
            _lastClock=m.clockCount();_clockTicks=0;_waitFirstClock=true;
            return;
        }
        // Resolve a swung step scheduled between two F8 boundaries.
        if(_pendingStep && (int32_t)(now-_pendingStepUs)>=0) {
            _pendingStep=false;
            fireStep(stepBasePeriodUs((uint16_t)(m.clockBpm()>1?m.clockBpm():120),r));
        }
        if(_gateOn&&!_tieHold&&(int32_t)(now-_gateOffUs)>=0)releaseNote();
        const uint32_t clocks=m.clockCount();
        uint32_t delta=clocks-_lastClock;
        if(delta>96) {
            releaseNote();_pendingStep=false;
            _lastClock=clocks;_clockTicks=0;_waitFirstClock=true;
            return;
        }
        const uint8_t ticks=(r==QUARTER)?24:(r==EIGHTH)?12:(r==SIXTEENTH)?6:3;
        const float bpm=m.clockBpm()>1?m.clockBpm():120.0f;
        const uint32_t beatUs=(uint32_t)(60000000.0f/bpm);
        const uint32_t period=(beatUs*ticks)/24U;
        const uint32_t swDelay=(period*(uint32_t)swing())/100U;
        while(delta--) {
            ++_lastClock;
            bool boundary=false;
            if(_waitFirstClock){_waitFirstClock=false;_clockTicks=0;boundary=true;}
            else if(++_clockTicks>=ticks){_clockTicks=0;boundary=true;}
            if(!boundary)continue;
            if((_stepCount&1U) && swDelay>0) {
                // Alternate steps are placed after their nominal tick.
                // Their paired successor stays exactly on the clock grid.
                _pendingStep=true;_pendingStepUs=micros()+swDelay;
            } else {
                if(_pendingStep) { // stale swing event after stalled clock service
                    _pendingStep=false;releaseNote();
                }
                fireStep(period);
            }
        }
        return;
    }
    _running.store(true,std::memory_order_release);
    if(_gateOn&&!_tieHold&&(int32_t)(now-_gateOffUs)>=0)releaseNote();
    const uint32_t base=stepBasePeriodUs(tempo(),r);
    if((int32_t)(now-_nextStepUs)>=0) {
        fireStep(base);
        _nextStepUs+=stepIntervalUs(base,swing());
        // Recover from UI/SD stalls without emitting a burst of old events.
        if((int32_t)(now-_nextStepUs)>(int32_t)(2U*base))_nextStepUs=now+base;
    }
}

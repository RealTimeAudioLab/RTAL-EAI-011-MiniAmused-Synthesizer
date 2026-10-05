#include "ArpEngine.h"
#include "Parameters.h"
#include "MidiEngine.h"
#include "SynthEngine.h"
#include <math.h>

ArpEngine& ArpEngine::instance(){ static ArpEngine a; return a; }
uint32_t ArpEngine::rnd(){ _rng^=_rng<<13; _rng^=_rng>>17; _rng^=_rng<<5; return _rng; }

int ArpEngine::collect(uint8_t *notes,uint8_t *vels) const {
    int n=0; for(int i=0;i<128;i++) if(_held[i].down){ notes[n]=(uint8_t)i; vels[n]=_held[i].vel; n++; } return n;
}
void ArpEngine::releaseOutput(){ if(_gateOn && _outNote>=0) SynthEngine::instance().noteOff((uint8_t)_outNote); _gateOn=false; _outNote=-1; }
void ArpEngine::reset(){ releaseOutput(); _step=0; _rhythmStep=0; _dir=1; _lastIndex=0; _repeatIndex=0; _lastClock=MidiEngine::instance().clockCount(); _schedulerPrimed=false; _ratchetTotal=1; _ratchetIndex=0; _lastAccent=false; }

void ArpEngine::noteOn(uint8_t note,uint8_t velocity){
    note&=127; velocity&=127;
    const bool en=Parameters::instance().target(ParameterID::ARP_ENABLE)>=0.5f;
    if(!en){ SynthEngine::instance().noteOn(note,velocity); return; }
    if(!_held[note].down){ _held[note].down=true; _heldCount++; }
    _held[note].keyDown=true;
    _held[note].vel=velocity?velocity:1;
}
void ArpEngine::noteOff(uint8_t note){
    note&=127;
    const bool en=Parameters::instance().target(ParameterID::ARP_ENABLE)>=0.5f;
    if(!en){ SynthEngine::instance().noteOff(note); return; }
    const bool hold=Parameters::instance().target(ParameterID::ARP_HOLD)>=0.5f;
    _held[note].keyDown=false;
    if(!hold && _held[note].down){ _held[note].down=false; if(_heldCount) _heldCount--; }
    if(!hold && _heldCount==0) releaseOutput();
}
void ArpEngine::releaseLatchedNotes(){
    // HOLD ON -> OFF: discard only notes whose physical keys are already up.
    // Keys that are still physically held remain valid ARP input notes.
    for(auto &h:_held){
        if(h.down && !h.keyDown){ h.down=false; if(_heldCount) _heldCount--; }
    }
    if(_heldCount==0) releaseOutput();
}
void ArpEngine::allNotesOff(){ for(auto &h:_held){ h.down=false; h.keyDown=false; } _heldCount=0; releaseOutput(); SynthEngine::instance().allNotesOff(); _schedulerPrimed=false; }

int ArpEngine::chooseIndex(int n,int mode){
    if(n<=1) return 0;
    // ARP3 REPEAT: probability of replaying the previous chord index.
    // This is evaluated before the selected pitch algorithm and therefore works
    // in UP/DOWN/UP-DOWN/RANDOM/MARKOV without changing their base behavior at 0%.
    const float repeat=Parameters::instance().target(ParameterID::ARP_REPEAT);
    if(repeat>0.0f && (rnd()%10000U) < (uint32_t)(repeat*10000.0f)) {
        int i=_repeatIndex; if(i<0)i=0; if(i>=n)i=n-1; return i;
    }
    if(mode==0){ int i=_step%n; _step++; _lastIndex=i; return i; }
    if(mode==1){ int i=(n-1)-(_step%n); _step++; _lastIndex=i; return i; }
    if(mode==2){ if(_lastIndex<=0) _dir=1; else if(_lastIndex>=n-1) _dir=-1; int i=_lastIndex; _lastIndex+=_dir; return i; }
    if(mode==3){ int i=(int)(rnd()%(uint32_t)n); _lastIndex=i; return i; }
    const float markov=Parameters::instance().target(ParameterID::ARP_MARKOV);
    if((rnd()%1000U) >= (uint32_t)(markov*1000.0f)){ int i=(_lastIndex+1)%n; _lastIndex=i; return i; }
    const float mutation=Parameters::instance().target(ParameterID::ARP_MUTATION);
    if((rnd()%1000U) < (uint32_t)(mutation*1000.0f)) return (int)(rnd()%(uint32_t)n);
    const float range=Parameters::instance().target(ParameterID::ARP_RANGE);
    const uint32_t r=rnd()%100U; int delta=0;
    if(r<12) delta=0; else if(r<42) delta=1; else if(r<72) delta=-1; else if(r<86) delta=(range>0.5f?2:1); else delta=(range>0.5f?-2:-1);
    int i=(_lastIndex+delta)%n; if(i<0)i+=n; _lastIndex=i; return i;
}

void ArpEngine::triggerVoice(uint8_t note,uint8_t vel,uint32_t subPeriodUs){
    releaseOutput(); SynthEngine::instance().noteOn(note,vel); _outNote=note; _gateOn=true; _stepStartUs=micros();
    _ratchetPeriodUs=subPeriodUs;
}

void ArpEngine::triggerStep(){
    const int rhythmStep=_rhythmStep++;
    uint8_t notes[128],vels[128]; int n=collect(notes,vels); if(n<=0){ releaseOutput(); return; }
    // DENSITY is a true rest probability. A rest still advances the rhythmic step,
    // but does not advance pitch selection, keeping sparse patterns coherent.
    const float density=Parameters::instance().target(ParameterID::ARP_DENSITY);
    if((rnd()%10000U) >= (uint32_t)(density*10000.0f)){ releaseOutput(); _ratchetTotal=1; _ratchetIndex=0; return; }
    int mode=(int)lroundf(Parameters::instance().target(ParameterID::ARP_MODE)*4.0f); if(mode<0)mode=0;if(mode>4)mode=4;
    int idx=chooseIndex(n,mode);
    _repeatIndex=idx;
    int octs=1+(int)lroundf(Parameters::instance().target(ParameterID::ARP_OCTAVES)*3.0f);
    int octave=(_step/n)%octs;
    int note=(int)notes[idx]+12*octave; while(note>127) note-=12;

    uint8_t vel=vels[idx];
    // ARP3 ACCENT MODE: 4 STEP / 3 STEP / 2 STEP / RANDOM / MARKOV.
    // ACCENT still controls only the strength of the velocity lift.
    const float accent=Parameters::instance().target(ParameterID::ARP_ACCENT);
    int accentMode=(int)lroundf(Parameters::instance().target(ParameterID::ARP_ACCENT_MODE)*4.0f);
    if(accentMode<0) accentMode=0; if(accentMode>4) accentMode=4;
    bool doAccent=false;
    if(accentMode==0) doAccent=((rhythmStep%4)==0);
    else if(accentMode==1) doAccent=((rhythmStep%3)==0);
    else if(accentMode==2) doAccent=((rhythmStep%2)==0);
    else if(accentMode==3) doAccent=((rnd()%4U)==0U); // 25% random accents
    else {
        // Markov-like two-state accent process: accented steps tend to cluster briefly,
        // while non-accented steps remain the more likely state.
        if(_lastAccent) doAccent=((rnd()%100U)<55U);
        else doAccent=((rnd()%100U)<22U);
        _lastAccent=doAccent;
    }
    if(accent>0.0f && doAccent) { int v=vel+(int)lroundf((127-vel)*accent); vel=(uint8_t)(v>127?127:v); }

    int rr=(int)lroundf(Parameters::instance().target(ParameterID::ARP_RATCHET)*3.0f);
    _ratchetTotal=(uint8_t)(rr<=0?1:rr+1); // OFF,2X,3X,4X
    _ratchetIndex=1; _ratchetNote=(uint8_t)note; _ratchetVel=vel;
    const uint32_t sub=_stepPeriodUs/_ratchetTotal;
    triggerVoice(_ratchetNote,_ratchetVel,sub);
    _nextRatchetUs=micros()+sub;
}

void ArpEngine::service(){
    const bool en=Parameters::instance().target(ParameterID::ARP_ENABLE)>=0.5f;
    if(en!=_wasEnabled){ if(en){ SynthEngine::instance().allNotesOff(); reset(); } else { allNotesOff(); } _wasEnabled=en; }
    if(!en) return;

    const bool hold=Parameters::instance().target(ParameterID::ARP_HOLD)>=0.5f;
    if(hold!=_wasHold){
        if(!hold) releaseLatchedNotes();
        _wasHold=hold;
    }

    const bool midiSync=Parameters::instance().target(ParameterID::ARP_MIDI_SYNC)>=0.5f;
    if(midiSync!=_wasMidiSync){ reset(); _wasMidiSync=midiSync; }

    // External sync preserves the proven MIDI transport behavior. Internal sync
    // runs whenever ARP is enabled, independent of incoming F8/FA/FB/FC.
    const bool running=midiSync ? MidiEngine::instance().transportRunning() : true;
    if(running && !_wasRunning) reset();
    if(!running && _wasRunning) releaseOutput();
    _wasRunning=running; if(!running) return;

    static const uint8_t divs[6]={24,12,8,6,4,3};
    int ri=(int)lroundf(Parameters::instance().target(ParameterID::ARP_RATE)*5.0f); if(ri<0)ri=0;if(ri>5)ri=5;
    float bpm;
    if(midiSync) bpm=MidiEngine::instance().clockBpm();
    else bpm=40.0f + 260.0f*Parameters::instance().target(ParameterID::ARP_TEMPO);
    if(bpm<20.0f) bpm=120.0f;
    _stepPeriodUs=(uint32_t)(60000000.0f/bpm*(float)divs[ri]/24.0f);
    const uint32_t now=micros();

    if(midiSync) {
        // MIDI clock establishes phase; microsecond scheduler permits swing.
        const uint32_t clocks=MidiEngine::instance().clockCount();
        const uint32_t d=clocks-_lastClock;
        if(!_schedulerPrimed && d>=divs[ri]) { _lastClock=clocks-(d%divs[ri]); _nextStepUs=now; _schedulerPrimed=true; }
    } else if(!_schedulerPrimed) {
        // Internal clock starts immediately and is serviced by the dedicated Core-0 task.
        _nextStepUs=now;
        _schedulerPrimed=true;
    }

    if(_schedulerPrimed && (int32_t)(now-_nextStepUs)>=0){
        triggerStep();
        const float sw=Parameters::instance().target(ParameterID::ARP_SWING);
        const float amount=0.25f*sw;
        const bool odd=(_rhythmStep & 1)!=0;
        const float mul=odd ? (1.0f+amount) : (1.0f-amount);
        _nextStepUs += (uint32_t)(_stepPeriodUs*mul);
        // Recover gracefully after long stalls instead of firing a burst of overdue steps.
        if((int32_t)(now-_nextStepUs) > (int32_t)(_stepPeriodUs*2U)) _nextStepUs=now+_stepPeriodUs;
    }

    if(_ratchetTotal>1 && _ratchetIndex<_ratchetTotal && (int32_t)(now-_nextRatchetUs)>=0){
        triggerVoice(_ratchetNote,_ratchetVel,_ratchetPeriodUs);
        _ratchetIndex++; _nextRatchetUs += _ratchetPeriodUs;
    }
    if(_gateOn){
        float g=Parameters::instance().target(ParameterID::ARP_GATE); float frac=0.10f+0.88f*g;
        const uint32_t period=(_ratchetTotal>1)?_ratchetPeriodUs:_stepPeriodUs;
        if((uint32_t)(micros()-_stepStartUs) >= (uint32_t)(period*frac)) releaseOutput();
    }
}

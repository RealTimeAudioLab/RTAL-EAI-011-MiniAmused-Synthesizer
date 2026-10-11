#pragma once
#include <Arduino.h>
#include "RTAL_IO_PINS.h"

class Parameters;

class DelayEngine {
public:
    bool begin();
    void prepareBlock(Parameters &p);
    inline void process(float in, float &outL, float &outR)
    {
        if(!_on || !_ready) { outL=in; outR=in; return; }

        float dl, dr;
        if(_xfadeActive) {
            const float aL=readFrac(_bufL,_pos,_tapOldSamples);
            const float aR=readFrac(_bufR,_pos,_tapOldSamples*_rightTapRatio);
            const float bL=readFrac(_bufL,_pos,_tapNewSamples);
            const float bR=readFrac(_bufR,_pos,_tapNewSamples*_rightTapRatio);
            const float x=_xfade;
            dl=aL+(bL-aL)*x;
            dr=aR+(bR-aR)*x;
            _xfade += XFADE_STEP;
            if(_xfade>=1.0f) {
                _xfade=1.0f;
                _xfadeActive=false;
                _tapCurrentSamples=_tapNewSamples;
            }
        } else {
            dl=readFrac(_bufL,_pos,_tapCurrentSamples);
            dr=readFrac(_bufR,_pos,_tapCurrentSamples*_rightTapRatio);
        }

        // v0.7.3 PINGPONG1 feedback/input matrix.
        // LEGACY is bit-for-bit the old topology: R->L, L->R, mono seed to both.
        float fbSrcL=dr, fbSrcR=dl;
        float inL=in, inR=in;
        const float a=_pingAmount;
        switch(_pingMode) {
            default: // LEGACY
            case 0:
                break;
            case 1: // CLASSIC: seed left, then alternate L<->R; AMOUNT blends same/cross feedback
                fbSrcL=dl + (dr-dl)*a;
                fbSrcR=dr + (dl-dr)*a;
                inL=in;
                inR=in*(1.0f-a);
                break;
            case 2: { // SOFT: centered seed, gentle cross-feedback up to 50%
                const float x=0.5f*a;
                fbSrcL=dl + (dr-dl)*x;
                fbSrcR=dr + (dl-dr)*x;
                break;
            }
            case 3: // WIDE: left-biased seed + full cross path, retains existing WIDTH tap offset
                fbSrcL=dl + (dr-dl)*a;
                fbSrcR=dr + (dl-dr)*a;
                inL=in;
                inR=in*(1.0f-0.75f*a);
                break;
            case 4: { // DUAL: mostly independent L/R delays, only subtle crossfeed
                const float x=0.25f*a;
                fbSrcL=dl + (dr-dl)*x;
                fbSrcR=dr + (dl-dr)*x;
                break;
            }
        }

        _fbFiltL += _filterCoeff * (fbSrcL - _fbFiltL);
        _fbFiltR += _filterCoeff * (fbSrcR - _fbFiltR);
        _bufL[_pos]=toQ15(inL + _fbFiltL*_feedback);
        _bufR[_pos]=toQ15(inR + _fbFiltR*_feedback);
        if(++_pos>=DELAY_MAX) _pos=0;

        const float dry=1.0f-_mix;
        float wetL=in*dry + dl*_mix;
        float wetR=in*dry + dr*_mix;

        // v0.8.0 MIDI-FINAL1: true post-delay Mid/Side width.
        // 0%=mono, 100%=bit-identical L/R reconstruction, 200%=2x Side.
        const float mid=0.5f*(wetL+wetR);
        const float side=0.5f*(wetL-wetR)*_stereoWidth;
        outL=mid+side;
        outR=mid-side;
    }

    bool ready() const { return _ready; }

private:
    static constexpr uint32_t SR=RTAL_SAMPLE_RATE;
    static constexpr uint32_t DELAY_MAX=(SR*6U)/5U+8U;
    static constexpr float XFADE_MS=24.0f;
    static constexpr float XFADE_STEP=1.0f/(0.001f*XFADE_MS*(float)SR);

    int16_t *_bufL=nullptr;
    int16_t *_bufR=nullptr;
    uint32_t _pos=0;
    bool _ready=false;
    bool _on=false;

    // Stable-delay taps: never slew a live read pointer. Time changes use
    // two fixed taps and a short crossfade (WELLENBAD StableSyncDelay model).
    float _tapCurrentSamples=14400.0f;
    float _tapOldSamples=14400.0f;
    float _tapNewSamples=14400.0f;
    float _xfade=1.0f;
    bool _xfadeActive=false;
    bool _tapInitialized=false;

    // FREE mode debounce: keep old tap while the finger is moving, then make
    // one clean transition after the requested value has settled.
    float _freeRequestedMs=300.0f;
    float _freeObservedMs=300.0f;
    uint32_t _freeChangedAtMs=0;

    // SYNC mode stability: evaluate one BPM candidate per 24 F8 clocks,
    // require three mutually close windows, then apply hysteresis.
    uint32_t _syncClockAnchor=0;
    float _syncPrevWindowBpm=0.0f;
    uint8_t _syncStableWindows=0;
    float _syncAcceptedBpm=0.0f;
    int _lastDiv=-1;
    bool _lastSync=false;

    float _feedback=0.35f;
    float _mix=0.20f;
    float _filterCoeff=0.55f;
    float _rightTapRatio=0.934f;
    uint8_t _pingMode=0;
    float _pingAmount=1.0f;
    float _stereoWidth=1.0f;
    float _fbFiltL=0.0f;
    float _fbFiltR=0.0f;

    void requestTapMs(float ms, bool force=false);
    static float divisionMultiplier(int div);

    static inline float clamp1(float x) {
        if(x>0.999f) return 0.999f;
        if(x<-0.999f) return -0.999f;
        return x;
    }
    static inline int16_t toQ15(float x) { return (int16_t)(clamp1(x)*32767.0f); }
    static inline float fromQ15(int16_t x) { return (float)x*(1.0f/32768.0f); }
    static inline float readFrac(const int16_t *buf,uint32_t writePos,float delaySamples) {
        if(!buf) return 0.0f;
        if(delaySamples<1.0f) delaySamples=1.0f;
        if(delaySamples>(float)(DELAY_MAX-2)) delaySamples=(float)(DELAY_MAX-2);
        float rp=(float)writePos-delaySamples;
        while(rp<0.0f) rp+=(float)DELAY_MAX;
        while(rp>=(float)DELAY_MAX) rp-=(float)DELAY_MAX;
        uint32_t i0=(uint32_t)rp;
        uint32_t i1=i0+1; if(i1>=DELAY_MAX) i1=0;
        float f=rp-(float)i0;
        float a=fromQ15(buf[i0]), b=fromQ15(buf[i1]);
        return a+(b-a)*f;
    }
};

#include "DelayEngine.h"
#include "Parameters.h"
#include "ParameterIDs.h"
#include "MidiEngine.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <math.h>

static int16_t *allocDelayPsram(size_t samples)
{
    int16_t *p=(int16_t*)heap_caps_malloc(samples*sizeof(int16_t),
                                          MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
    if(p) memset(p,0,samples*sizeof(int16_t));
    return p;
}

float DelayEngine::divisionMultiplier(int div)
{
    static const float m[8]={1.0f,0.5f,0.75f,1.0f/3.0f,0.25f,0.375f,1.0f/6.0f,0.125f};
    if(div<0) div=0; else if(div>7) div=7;
    return m[div];
}

void DelayEngine::requestTapMs(float ms, bool force)
{
    if(ms<1.0f) ms=1.0f;
    const float maxMs=((float)(DELAY_MAX-2))*1000.0f/(float)SR;
    if(ms>maxMs) ms=maxMs;

    // Quantize targets to 0.5 ms. Small control/clock jitter must not move a tap.
    ms=floorf(ms*2.0f+0.5f)*0.5f;
    const float samples=ms*((float)SR*0.001f);

    if(!_tapInitialized) {
        _tapCurrentSamples=_tapOldSamples=_tapNewSamples=samples;
        _tapInitialized=true;
        _xfadeActive=false;
        _xfade=1.0f;
        return;
    }

    const float currentMs=_tapCurrentSamples*1000.0f/(float)SR;
    if(!force && fabsf(ms-currentMs)<4.0f) return; // WELLENBAD-style dead band

    // Never retarget a running crossfade. A newer request is picked up by the
    // next control block after this 24 ms transition completes.
    if(_xfadeActive) return;

    _tapOldSamples=_tapCurrentSamples;
    _tapNewSamples=samples;
    _xfade=0.0f;
    _xfadeActive=true;
}

bool DelayEngine::begin()
{
    _bufL=allocDelayPsram(DELAY_MAX);
    _bufR=allocDelayPsram(DELAY_MAX);
    _ready=(_bufL && _bufR);
    Serial.printf("[DELAY] PSRAM stereo buffers %s samples/ch=%u freeHeap=%u freePSRAM=%u\n",
                  _ready?"OK":"FAIL",(unsigned)DELAY_MAX,
                  ESP.getFreeHeap(),ESP.getFreePsram());
    return _ready;
}

void DelayEngine::prepareBlock(Parameters &p)
{
    _on=_ready && p.target(ParameterID::DELAY_ENABLE)>=0.5f;
    const bool sync=p.target(ParameterID::DELAY_SYNC)>=0.5f;
    int div=(int)lroundf(p.target(ParameterID::DELAY_DIV)*7.0f);
    if(div<0) div=0; else if(div>7) div=7;

    if(sync && MidiEngine::instance().clockActive()) {
        const uint32_t clocks=MidiEngine::instance().clockCount();

        // On entry/division change, establish a clean anchor. The currently
        // sounding tap stays fixed until a stable tempo has been confirmed.
        if(!_lastSync || div!=_lastDiv) {
            _syncClockAnchor=clocks;
            _syncPrevWindowBpm=0.0f;
            _syncStableWindows=0;
            _syncAcceptedBpm=0.0f;
        }

        if((uint32_t)(clocks-_syncClockAnchor)>=24U) {
            _syncClockAnchor=clocks;
            float bpm=MidiEngine::instance().clockBpm();
            if(bpm<25.0f) bpm=25.0f;
            if(bpm>300.0f) bpm=300.0f;

            if(_syncPrevWindowBpm>0.0f && fabsf(bpm-_syncPrevWindowBpm)<=0.75f)
                ++_syncStableWindows;
            else
                _syncStableWindows=1;
            _syncPrevWindowBpm=bpm;

            if(_syncStableWindows>=3) {
                if(_syncAcceptedBpm<=0.0f || fabsf(bpm-_syncAcceptedBpm)>=2.0f || div!=_lastDiv) {
                    _syncAcceptedBpm=bpm;
                    requestTapMs((60000.0f/bpm)*divisionMultiplier(div), div!=_lastDiv);
                }
            }
        }
    } else {
        // FREE mode: use the raw requested parameter, but debounce movement.
        // This deliberately avoids p.smooth(DELAY_TIME): smoothing a live tap
        // is exactly the Doppler/flanger mechanism we want to eliminate.
        const float t=p.target(ParameterID::DELAY_TIME);
        const float ms=20.0f+980.0f*t*t;
        if(fabsf(ms-_freeObservedMs)>0.25f) {
            _freeObservedMs=ms;
            _freeChangedAtMs=millis();
        }
        _freeRequestedMs=ms;
        if((uint32_t)(millis()-_freeChangedAtMs)>=60U)
            requestTapMs(_freeRequestedMs, _lastSync);
    }

    _lastSync=sync;
    _lastDiv=div;

    _feedback=p.smooth(ParameterID::DELAY_FEEDBACK,0.18f)*0.88f;
    _mix=p.smooth(ParameterID::DELAY_MIX,0.18f);
    const float filt=p.smooth(ParameterID::DELAY_FILTER,0.18f);
    _filterCoeff=0.02f + 0.96f*filt*filt;
    const float width=p.smooth(ParameterID::DELAY_WIDTH,0.18f);
    _rightTapRatio=1.0f - 0.10f*width;

    int pm=(int)lroundf(p.target(ParameterID::DELAY_PING_MODE)*4.0f);
    if(pm<0) pm=0; else if(pm>4) pm=4;
    _pingMode=(uint8_t)pm;
    _pingAmount=p.smooth(ParameterID::DELAY_PING_AMOUNT,0.18f);
    _stereoWidth=2.0f*p.smooth(ParameterID::DELAY_STEREO_WIDTH,0.18f);
}

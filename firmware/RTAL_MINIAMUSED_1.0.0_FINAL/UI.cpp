#include "UI.h"
#include "esp_attr.h"
#include "Board.h"
#include "Parameters.h"
#include "PresetManager.h"
#include "SynthEngine.h"
#include "MidiEngine.h"
#include "BringUpConfig.h"
#include <lvgl.h>
#include <math.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {

static void makeKeyboardOverlay();

constexpr uint32_t C_BG      = 0x090909;
constexpr uint32_t C_PANEL   = 0x171411;
constexpr uint32_t C_PANEL2  = 0x10151A;
constexpr uint32_t C_GOLD    = 0xDAB77C;
constexpr uint32_t C_TEXT    = 0xF4E6C8;
constexpr uint32_t C_DIM     = 0x8E806D;
constexpr uint32_t C_BLUE    = 0xA7C3DE;
constexpr uint32_t C_BORDER  = 0x66513C;

static lv_obj_t *midiChannelButton=nullptr;
static lv_obj_t *midiChannelValue=nullptr;

static void refreshMidiChannelButton()
{
    if(!midiChannelValue) return;
    const uint8_t ch=MidiEngine::instance().channelSetting();
    char b[12];
    if(ch==0) snprintf(b,sizeof(b),"OMNI");
    else snprintf(b,sizeof(b),"CH %u",(unsigned)ch);
    lv_label_set_text(midiChannelValue,b);
}

static void midiChannelEvent(lv_event_t *e)
{
    if(!e || lv_event_get_code(e)!=LV_EVENT_CLICKED) return;
    uint8_t ch=MidiEngine::instance().channelSetting();
    ch=(ch>=16)?0:(uint8_t)(ch+1);
    MidiEngine::instance().setChannelSetting(ch);
    refreshMidiChannelButton();
}

static lv_obj_t *makeMidiChannelControl(lv_obj_t *parent,int x,int y,int w,int h)
{
    lv_obj_t *b=lv_btn_create(parent);
    midiChannelButton=b;
    lv_obj_set_pos(b,x,y); lv_obj_set_size(b,w,h);
    lv_obj_set_style_bg_color(b,lv_color_hex(0x242424),0);
    lv_obj_set_style_border_color(b,lv_color_hex(C_BORDER),0);
    lv_obj_set_style_border_width(b,1,0);
    lv_obj_set_style_radius(b,5,0);
    lv_obj_t *t=lv_label_create(b);
    lv_label_set_text(t,"MIDI CH");
    lv_obj_set_style_text_color(t,lv_color_hex(C_DIM),0);
    lv_obj_set_style_text_font(t,&lv_font_montserrat_16,0);
    lv_obj_align(t,LV_ALIGN_TOP_MID,0,-5);
    midiChannelValue=lv_label_create(b);
    lv_obj_set_style_text_color(midiChannelValue,lv_color_hex(C_TEXT),0);
    lv_obj_set_style_text_font(midiChannelValue,&lv_font_montserrat_12,0);
    lv_obj_align(midiChannelValue,LV_ALIGN_BOTTOM_MID,0,5);
    lv_obj_add_event_cb(b,midiChannelEvent,LV_EVENT_CLICKED,nullptr);
    refreshMidiChannelButton();
    return b;
}

enum Page { PAGE_OSC, PAGE_MIX, PAGE_FILTER, PAGE_MOD, PAGE_PRESET, PAGE_EFX, PAGE_COUNT };

struct Binding {
    ParameterID id;
    const char *name;
    bool stepped;
    uint8_t steps;
};

struct Control {
    lv_obj_t *obj = nullptr;
    lv_obj_t *value = nullptr;
    Binding *binding = nullptr;
};

static lv_obj_t *pageHost=nullptr;
static lv_obj_t *pages[PAGE_COUNT]{};
static lv_obj_t *navButtons[PAGE_COUNT]{};

static lv_obj_t *overlay=nullptr;
static lv_obj_t *overlayTitle=nullptr;
static lv_obj_t *overlayValue=nullptr;
static lv_obj_t *overlaySlider=nullptr;

static Binding *activeBinding=nullptr;
static Binding *pendingBinding=nullptr;
static bool parameterOpenPending=false;
static uint32_t parameterOpenRequestedAt=0;
static bool parameterClosePending=false;
static uint32_t parameterCloseRequestedAt=0;
static bool controlRefreshPending=false;
static uint32_t controlRefreshRequestedAt=0;
static uint32_t parameterEventCounter=0;
static uint32_t parameterOpenCount=0;
static Binding *lastChangedBinding=nullptr;

static Binding *pendingValueBinding=nullptr;
static float pendingValue=0.0f;
static bool parameterValuePending=false;
static uint32_t parameterValueRequestedAt=0;

// Survives watchdog/software resets. This lets the next boot report what the
// UI was doing immediately before a crash.
static RTC_DATA_ATTR uint32_t rtcParamMagic=0;
static RTC_DATA_ATTR uint32_t rtcParamEvent=0;
static RTC_DATA_ATTR int32_t rtcParamId=-1;
static RTC_DATA_ATTR float rtcParamValue=0.0f;
static RTC_DATA_ATTR uint32_t rtcParamPhase=0;

static constexpr uint32_t RTC_PARAM_MAGIC = 0x5254414CUL; // "RTAL"



static uint32_t lastRevision=0;
static bool pageValuesDirty=false;
static uint64_t pendingParameterDirtyMask=0;
static uint32_t midiUiRefreshCount=0;
static bool externalSliderSyncGuard=false;
static uint32_t overlayLastGuiActivityMs=0;

// cycle after a continuous parameter overlay has been shown. No LVGL objects
// or DSP behavior are changed by this instrumentation.

static Page activePage=PAGE_OSC;
static bool pageChangePending=false;
static Page pendingPage=PAGE_OSC;
static uint32_t pageChangeRequestedAt=0;
static uint32_t pageChangeCount=0;

static bool presetActionPending=false;
static bool presetSavePending=false;
static uint32_t presetActionRequestedAt=0;
static lv_obj_t *presetStatusLabel=nullptr;
static lv_obj_t *presetHeaderLabel=nullptr;
static lv_obj_t *presetSlotLabel=nullptr;
static lv_obj_t *presetNameLabel=nullptr;
static lv_obj_t *presetPrevButton=nullptr;
static lv_obj_t *presetNextButton=nullptr;
static lv_obj_t *presetActionMatrix=nullptr;

static uint16_t currentPresetSlot=0;
static char currentPresetName[32]="INIT";
static bool presetBrowsePending=false;
static int presetBrowseDelta=0;
static uint32_t presetBrowseRequestedAt=0;

static lv_obj_t *presetNameOverlay=nullptr;
static lv_obj_t *presetNameTextLabel=nullptr;
static lv_obj_t *presetNameMatrix=nullptr;
static bool presetNameOpenPending=false;
static uint32_t presetNameOpenRequestedAt=0;
static bool presetNameClosePending=false;
static bool presetNameCommitPending=false;
static uint32_t presetNameCloseRequestedAt=0;
static char presetEditName[21]="";

static int presetConfirmAction=0;
static int presetMgmtArmedAction=0;
static uint32_t presetMgmtArmedAt=0;
static bool presetMgmtExecutePending=false;
static uint32_t presetMgmtExecuteRequestedAt=0; // 1=INIT, 2=DELETE



static constexpr uint16_t PRESET_SLOT_MIN=0;
static constexpr uint16_t PRESET_SLOT_MAX=127;
static bool midiProgramChangePending=false;
static uint8_t midiProgramChangeSlot=0;
static uint32_t midiProgramChangeRequestedAt=0;





static lv_obj_t *keyboardOverlay=nullptr;
static bool keyboardOpenPending=false;
static uint32_t keyboardOpenRequestedAt=0;
static bool keyboardClosePending=false;
static uint32_t keyboardCloseRequestedAt=0;

static uint8_t keyboardPostShowTrace=0;
static lv_obj_t *keyboardNoteLabel=nullptr;
static lv_obj_t *keyboardHoldLabel=nullptr;
static lv_obj_t *keyboardKey[24]{};
static bool keyboardHold=false;
static int keyboardBaseNote=48; // C3
static int keyboardTouchNote=-1;
static int keyboardLatchedNote=-1;
static int lastVisualNote=-999;
static bool lastVisualGate=false;

static const int kSemitoneMap[24] = {
    0,2,4,5,7,9,11,12,14,16,17,19,21,23,
    1,3,6,8,10,13,15,18,20,22
};

static bool isBlackSemitone(int semi)
{
    semi %= 12;
    return semi==1 || semi==3 || semi==6 || semi==8 || semi==10;
}

static void keyboardAllNotesOff()
{
    if(keyboardTouchNote >= 0)
        SynthEngine::instance().noteOff((uint8_t)keyboardTouchNote);

    if(keyboardLatchedNote >= 0)
        SynthEngine::instance().noteOff((uint8_t)keyboardLatchedNote);

    keyboardTouchNote=-1;
    keyboardLatchedNote=-1;
}

static Control controls[64];
static int controlCount=0;

static Binding B_MASTER {ParameterID::MASTER_VOLUME,"MASTER VOLUME",false,0};
static Binding B_MTUNE  {ParameterID::MASTER_TUNE,"MASTER TUNE",false,0};
static Binding B_GLIDE  {ParameterID::GLIDE_TIME,"GLIDE",false,0};
static Binding B_GLIDEEN{ParameterID::GLIDE_ENABLE,"GLIDE ON",true,2};

static Binding B_O1R {ParameterID::OSC1_RANGE,"OSC 1 RANGE",true,5};
static Binding B_O1W {ParameterID::OSC1_WAVE,"OSC 1 WAVE",true,6};
static Binding B_O1L {ParameterID::OSC1_LEVEL,"OSC 1 LEVEL",false,0};
static Binding B_O2R {ParameterID::OSC2_RANGE,"OSC 2 RANGE",true,5};
static Binding B_O2W {ParameterID::OSC2_WAVE,"OSC 2 WAVE",true,6};
static Binding B_O2T {ParameterID::OSC2_TUNE,"OSC 2 TUNE",false,0};
static Binding B_O2L {ParameterID::OSC2_LEVEL,"OSC 2 LEVEL",false,0};
static Binding B_O3R {ParameterID::OSC3_RANGE,"OSC 3 RANGE",true,5};
static Binding B_O3W {ParameterID::OSC3_WAVE,"OSC 3 WAVE",true,6};
static Binding B_O3T {ParameterID::OSC3_TUNE,"OSC 3 TUNE",false,0};
static Binding B_O3L {ParameterID::OSC3_LEVEL,"OSC 3 LEVEL",false,0};
static Binding B_O3K {ParameterID::OSC3_KEYBOARD_CONTROL,"OSC 3 KEYBOARD",true,2};

static Binding B_NOISE {ParameterID::NOISE_LEVEL,"NOISE LEVEL",false,0};
static Binding B_NCOL  {ParameterID::NOISE_COLOR,"NOISE COLOR",false,0};
static Binding B_DRIVE {ParameterID::MIXER_DRIVE,"MIXER DRIVE",false,0};
static Binding B_FB    {ParameterID::FEEDBACK_LEVEL,"FEEDBACK",false,0};

static Binding B_CUT   {ParameterID::FILTER_CUTOFF,"CUTOFF",false,0};
static Binding B_RES   {ParameterID::FILTER_RESONANCE,"EMPHASIS",false,0};
static Binding B_CONT  {ParameterID::FILTER_CONTOUR,"CONTOUR AMOUNT",false,0};
static Binding B_KT    {ParameterID::FILTER_KEYTRACK,"KEYBOARD TRACK",true,4};

static Binding B_FA {ParameterID::FILTER_ATTACK,"FILTER ATTACK",false,0};
static Binding B_FD {ParameterID::FILTER_DECAY,"FILTER DECAY",false,0};
static Binding B_FS {ParameterID::FILTER_SUSTAIN,"FILTER SUSTAIN",false,0};
static Binding B_FR {ParameterID::FILTER_RELEASE,"FILTER RELEASE",false,0};
static Binding B_AA {ParameterID::AMP_ATTACK,"LOUDNESS ATTACK",false,0};
static Binding B_AD {ParameterID::AMP_DECAY,"LOUDNESS DECAY",false,0};
static Binding B_AS {ParameterID::AMP_SUSTAIN,"LOUDNESS SUSTAIN",false,0};
static Binding B_AR {ParameterID::AMP_RELEASE,"LOUDNESS RELEASE",false,0};

static Binding B_MW {ParameterID::MOD_WHEEL,"MOD WHEEL",false,0};
static Binding B_MM {ParameterID::MOD_MIX,"OSC3 / NOISE MIX",false,0};
static Binding B_OM {ParameterID::OSC_MOD_ENABLE,"OSC MOD",true,2};
static Binding B_FM {ParameterID::FILTER_MOD_ENABLE,"FILTER MOD",true,2};
static Binding B_DEC{ParameterID::DECAY_ENABLE,"DECAY",true,2};
static Binding B_PRI{ParameterID::NOTE_PRIORITY,"NOTE PRIORITY",true,3};
static Binding B_TRG{ParameterID::TRIGGER_MODE,"TRIGGER MODE",true,2};

static Binding B_DEN {ParameterID::DELAY_ENABLE,"DELAY",true,2};
static Binding B_DTIME{ParameterID::DELAY_TIME,"TIME",false,0};
static Binding B_DFB {ParameterID::DELAY_FEEDBACK,"FEEDBACK",false,0};
static Binding B_DMIX{ParameterID::DELAY_MIX,"MIX",false,0};
static Binding B_DFILT{ParameterID::DELAY_FILTER,"FILTER",false,0};
static Binding B_DWIDTH{ParameterID::DELAY_WIDTH,"WIDTH",false,0};
static Binding B_DSYNC{ParameterID::DELAY_SYNC,"SYNC",true,2};
static Binding B_DDIV {ParameterID::DELAY_DIV,"DIV",true,8};
static Binding B_OTIME{ParameterID::OVERLAY_TIMEOUT,"OVERLAY TIMEOUT",false,0};
static Binding B_DPMODE{ParameterID::DELAY_PING_MODE,"PING PONG MODE",true,5};
static Binding B_DPAMT{ParameterID::DELAY_PING_AMOUNT,"PING PONG AMOUNT",false,0};
static Binding B_DSTWIDTH{ParameterID::DELAY_STEREO_WIDTH,"STEREO WIDTH",false,0};

static float val(ParameterID id){ return Parameters::instance().target(id); }

static const char *rangeText(float v)
{
    static const char *t[]={"32'","16'","8'","4'","2'"};
    int i=(int)(v*5.0f); if(i<0)i=0; if(i>4)i=4; return t[i];
}
static const char *waveText(float v)
{
    static const char *t[]={"TRI","SHARK","SAW","SAW/SQ","SQUARE","PULSE"};
    int i=(int)(v*6.0f); if(i<0)i=0; if(i>5)i=5; return t[i];
}
static const char *priorityText(float v)
{
    return v<0.333f ? "LOW" : (v>0.666f ? "HIGH" : "LAST");
}

static const char *delayDivText(float v)
{
    static const char *t[]={"1/4","1/8","1/8D","1/8T","1/16","1/16D","1/16T","1/32"};
    int i=(int)lroundf(v*7.0f); if(i<0)i=0; if(i>7)i=7; return t[i];
}

static const char *delayPingModeText(float v)
{
    static const char *t[]={"LEGACY","CLASSIC","SOFT","WIDE","DUAL"};
    int i=(int)lroundf(v*4.0f); if(i<0)i=0; if(i>4)i=4; return t[i];
}

static const char *noiseColorText(float v)
{
    if (v < 0.08f) return "WHITE";
    if (v > 0.92f) return "PINK";
    return nullptr;
}

static const char *keyTrackText(float v)
{
    if(v < (1.0f/6.0f)) return "OFF";
    if(v < 0.5f) return "1/3";
    if(v < (5.0f/6.0f)) return "2/3";
    return "FULL";
}

static void formatValue(Binding *b, char *buf, size_t n)
{
    float v=val(b->id);
    switch(b->id) {
        case ParameterID::OSC1_RANGE:
        case ParameterID::OSC2_RANGE:
        case ParameterID::OSC3_RANGE:
            snprintf(buf,n,"%s",rangeText(v)); break;
        case ParameterID::OSC1_WAVE:
        case ParameterID::OSC2_WAVE:
        case ParameterID::OSC3_WAVE:
            snprintf(buf,n,"%s",waveText(v)); break;
        case ParameterID::GLIDE_ENABLE:
        case ParameterID::OSC3_KEYBOARD_CONTROL:
        case ParameterID::OSC_MOD_ENABLE:
        case ParameterID::FILTER_MOD_ENABLE:
        case ParameterID::DECAY_ENABLE:
        case ParameterID::DELAY_ENABLE:
        case ParameterID::DELAY_SYNC:
            snprintf(buf,n,"%s",v>=0.5f?"ON":"OFF"); break;
        case ParameterID::TRIGGER_MODE:
            snprintf(buf,n,"%s",v>=0.5f?"MULTI":"SINGLE"); break;
        case ParameterID::NOTE_PRIORITY:
            snprintf(buf,n,"%s",priorityText(v)); break;
        case ParameterID::FILTER_KEYTRACK:
            snprintf(buf,n,"%s",keyTrackText(v)); break;
        case ParameterID::NOISE_COLOR: {
            const char *nc = noiseColorText(v);
            if(nc) snprintf(buf,n,"%s",nc);
            else snprintf(buf,n,"%d%%",(int)lroundf(v*100.0f));
            break;
        }
        case ParameterID::FILTER_CUTOFF: {
            float hz=20.0f*powf(1000.0f,v);
            if(hz>=1000) snprintf(buf,n,"%.2f kHz",hz/1000.0f);
            else snprintf(buf,n,"%.0f Hz",hz);
            break;
        }
        case ParameterID::OSC2_TUNE:
        case ParameterID::OSC3_TUNE:
            snprintf(buf,n,"%+.2f st",(v-0.5f)*14.0f); break;
        case ParameterID::MASTER_TUNE:
            snprintf(buf,n,"%+.2f st",(v-0.5f)*24.0f); break;
        case ParameterID::DELAY_DIV:
            snprintf(buf,n,"%s",delayDivText(v)); break;
        case ParameterID::DELAY_PING_MODE:
            snprintf(buf,n,"%s",delayPingModeText(v)); break;
        case ParameterID::OVERLAY_TIMEOUT: {
            const int sec=1+(int)lroundf(v*4.0f);
            snprintf(buf,n,"%d s",sec); break;
        }
        case ParameterID::DELAY_STEREO_WIDTH:
            snprintf(buf,n,"%d%%",(int)lroundf(v*200.0f)); break;
        case ParameterID::DELAY_TIME: {
            const float ms=20.0f+980.0f*v*v;
            snprintf(buf,n,"%.0f ms",ms); break;
        }
        case ParameterID::DELAY_FILTER: {
            // Display the actual -3 dB corner of the existing one-pole
            // feedback LPF. DSP mapping itself remains unchanged.
            const float a=0.02f + 0.96f*v*v;
            const float r=1.0f-a;
            const float c=1.0f-(a*a)/(2.0f*r);
            if(c<=-1.0f) {
                // The digital pole is so open that the -3 dB point lies
                // above Nyquist (24 kHz at 48 kHz sample rate).
                snprintf(buf,n,">24 kHz");
            } else {
                const float hz=(48000.0f/(2.0f*3.14159265358979323846f))*acosf(c);
                if(hz>=1000.0f) snprintf(buf,n,"%.2f kHz",hz/1000.0f);
                else snprintf(buf,n,"%.0f Hz",hz);
            }
            break;
        }
        default:
            snprintf(buf,n,"%d%%",(int)lroundf(v*100.0f)); break;
    }
}


static void refreshBindingControls(Binding *binding)
{
    if(!binding)
        return;

    char txt[32];
    formatValue(binding, txt, sizeof(txt));

    for(int i=0; i<controlCount; ++i) {
        if(controls[i].binding != binding || !controls[i].value)
            continue;

        lv_label_set_text(controls[i].value, txt);
    }
}



static Binding *bindingForId(ParameterID id)
{
    for(int i=0;i<controlCount;i++) {
        if(controls[i].binding && controls[i].binding->id==id)
            return controls[i].binding;
    }
    return nullptr;
}

static void refreshOneVisibleParameter(ParameterID id)
{
    Binding *b=bindingForId(id);
    if(!b)
        return;

    char txt[32];
    formatValue(b,txt,sizeof(txt));

    // Update only controls that belong to the currently visible page.
    lv_obj_t *page=pages[(int)activePage];
    for(int i=0;i<controlCount;i++) {
        if(controls[i].binding!=b || !controls[i].value)
            continue;

        lv_obj_t *p=controls[i].value;
        bool visiblePage=false;
        while(p) {
            if(p==page) {
                visiblePage=true;
                break;
            }
            p=lv_obj_get_parent(p);
        }

        if(visiblePage)
            lv_label_set_text(controls[i].value,txt);
    }

    // If the same parameter is currently open, synchronize its modal as well.
    // Never fight a finger that is actively touching the screen.
    if(activeBinding==b && overlay && overlayValue && overlaySlider &&
       !Board::touchPressed()) {

        externalSliderSyncGuard=true;
        lv_label_set_text(overlayValue,txt);
        lv_slider_set_value(
            overlaySlider,
            (int32_t)lroundf(val(id)*1000.0f),
            LV_ANIM_OFF
        );
        externalSliderSyncGuard=false;
    }
}

static void serviceParameterDirtyUI()
{
    // Accumulate changes from MIDI, PRESET, TOUCH and INTERNAL sources.
    pendingParameterDirtyMask |= Parameters::instance().takeDirtyMask();

    if(pendingParameterDirtyMask==0)
        return;

    // Preserve pending bits while a touch gesture is in progress.
    if(Board::touchPressed())
        return;

    uint64_t mask=pendingParameterDirtyMask;
    pendingParameterDirtyMask=0;

    int updated=0;

    for(size_t i=0;i<(size_t)ParameterID::COUNT && i<64;i++) {
        const uint64_t bit=(uint64_t)1ULL<<i;
        if((mask & bit)==0)
            continue;

        refreshOneVisibleParameter((ParameterID)i);
        updated++;
    }

    midiUiRefreshCount++;

    // Keep diagnostic output sparse.
    if((midiUiRefreshCount & 0x1F)==1) {
        Serial.printf("[UI SYNC] pass=%lu dirty=%d page=%d heap=%u\n",
                      (unsigned long)midiUiRefreshCount,
                      updated,
                      (int)activePage,
                      ESP.getFreeHeap());
        Serial.flush();
    }
}

static bool objBelongsToPage(lv_obj_t *obj, lv_obj_t *page)
{
    if(!obj || !page)
        return false;

    lv_obj_t *p=obj;
    while(p) {
        if(p==page)
            return true;
        p=lv_obj_get_parent(p);
    }

    return false;
}

static void refreshPageControls(Page pg)
{
    if(pg < 0 || pg >= PAGE_COUNT)
        return;

    lv_obj_t *page=pages[(int)pg];
    if(!page)
        return;

    char txt[32];
    int refreshed=0;

    for(int i=0; i<controlCount; ++i) {
        if(!controls[i].binding || !controls[i].value)
            continue;

        if(!objBelongsToPage(controls[i].value, page))
            continue;

        formatValue(controls[i].binding, txt, sizeof(txt));
        lv_label_set_text(controls[i].value, txt);
        refreshed++;
    }

    Serial.printf("[UI REFRESH] page=%d controls=%d heap=%u\n",
                  (int)pg,
                  refreshed,
                  ESP.getFreeHeap());
    Serial.flush();
}

static void refreshControls()
{
    char b[32];
    for(int i=0;i<controlCount;i++) {
        if(!controls[i].binding || !controls[i].value) continue;
        formatValue(controls[i].binding,b,sizeof(b));
        lv_label_set_text(controls[i].value,b);
    }
}

static void closeOverlay(lv_event_t *e)
{
    (void)e;

    parameterClosePending=true;
    parameterCloseRequestedAt=millis();

    if(activeBinding) {
        rtcParamMagic=RTC_PARAM_MAGIC;
        rtcParamEvent=parameterEventCounter;
        rtcParamId=(int32_t)activeBinding->id;
        rtcParamValue=val(activeBinding->id);
        rtcParamPhase=5; // close requested
    }
}

static void sliderReleasedClose(lv_event_t *e)
{
    (void)e;
    // 0.6.6 workaround: close via the already existing slider object.
    // No second clickable/button object is created in the overlay.
    parameterClosePending=true;
    parameterCloseRequestedAt=millis();
}

static void sliderChanged(lv_event_t *e)
{
    if(externalSliderSyncGuard)
        return;

    if(!activeBinding || !e)
        return;

    overlayLastGuiActivityMs=millis();

    lv_obj_t *target=(lv_obj_t*)lv_event_get_target(e);
    if(!target)
        return;

    const int32_t raw=lv_slider_get_value(target);
    float v=raw/1000.0f;

    // 0.6.8a CENTER-SNAP1: make the exact neutral tuning point easy to hit
    // on the touch slider. A +/-4% zone around the physical center maps to
    // exactly 0.500, i.e. +0.00 st. Other parameters remain continuous.
    const bool centerSnapParam =
        activeBinding->id == ParameterID::MASTER_TUNE ||
        activeBinding->id == ParameterID::OSC2_TUNE ||
        activeBinding->id == ParameterID::OSC3_TUNE;
    if(centerSnapParam && raw >= 460 && raw <= 540)
        v=0.5f;

    if(activeBinding->stepped && activeBinding->steps>1) {
        const int idx=(int)lroundf(v*(activeBinding->steps-1));
        v=(float)idx/(float)(activeBinding->steps-1);
    }

    parameterEventCounter++;
    pendingValueBinding=activeBinding;
    pendingValue=v;
    parameterValuePending=true;
    parameterValueRequestedAt=millis();
    lastChangedBinding=activeBinding;

    // RTC crash marker: phase 2 = slider event captured.
    rtcParamMagic=RTC_PARAM_MAGIC;
    rtcParamEvent=parameterEventCounter;
    rtcParamId=(int32_t)activeBinding->id;
    rtcParamValue=v;
    rtcParamPhase=2;

    // IMPORTANT: no Parameters::set(), label update, refresh or object mutation
    // here. This callback returns to LVGL immediately.
}

static void openControl(lv_event_t *e)
{
    Binding *b=(Binding*)lv_event_get_user_data(e);
    if(!b) return;

    pendingBinding=b;
    parameterOpenPending=true;
    parameterOpenRequestedAt=millis();

    Serial.printf("[PARAM] open requested: %s heap=%u\n",
                  b->name,
                  ESP.getFreeHeap());
    Serial.flush();

    // Do not modify the parameter overlay here.
    // This callback runs inside LVGL event dispatch.
}

static lv_obj_t *panel(lv_obj_t *parent,int x,int y,int w,int h,const char *title)
{
    lv_obj_t *p=lv_obj_create(parent);
    lv_obj_set_pos(p,x,y); lv_obj_set_size(p,w,h);
    lv_obj_set_style_bg_color(p,lv_color_hex(C_PANEL),0);
    lv_obj_set_style_border_color(p,lv_color_hex(C_BORDER),0);
    lv_obj_set_style_border_width(p,1,0);
    lv_obj_set_style_radius(p,6,0);
    lv_obj_set_style_pad_all(p,4,0);
    lv_obj_clear_flag(p,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *l=lv_label_create(p);
    lv_label_set_text(l,title);
    lv_obj_set_style_text_color(l,lv_color_hex(C_GOLD),0);
    lv_obj_align(l,LV_ALIGN_TOP_MID,0,0);
    return p;
}

static lv_obj_t *control(lv_obj_t *parent,int x,int y,int w,int h,const char *name,Binding *b)
{
    lv_obj_t *o=lv_btn_create(parent);
    lv_obj_set_pos(o,x,y);
    lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(0x211C18),0);
    lv_obj_set_style_border_color(o,lv_color_hex(0x6F563B),0);
    lv_obj_set_style_border_width(o,1,0);
    lv_obj_set_style_radius(o,4,0);
    lv_obj_set_style_pad_all(o,0,0);

    lv_obj_t *n=lv_label_create(o);
    lv_label_set_text(n,name);
    lv_obj_set_style_text_color(n,lv_color_hex(C_DIM),0);
    lv_obj_set_style_text_font(n,&lv_font_montserrat_12,0);
    lv_obj_align(n,LV_ALIGN_TOP_MID,0,3);

    lv_obj_t *v=lv_label_create(o);
    lv_obj_set_style_text_color(v,lv_color_hex(C_TEXT),0);
    lv_obj_set_style_text_font(v,&lv_font_montserrat_14,0);
    lv_obj_align(v,LV_ALIGN_BOTTOM_MID,0,-3);

    lv_obj_add_event_cb(o,openControl,LV_EVENT_CLICKED,b);
    if (controlCount < (int)(sizeof(controls) / sizeof(controls[0]))) {
        controls[controlCount].obj = o;
        controls[controlCount].value = v;
        controls[controlCount].binding = b;
        controlCount++;
    }
    return o;
}

static void makeOscPage()
{
    lv_obj_t *p=pages[PAGE_OSC];

    lv_obj_t *a=panel(p,4,4,151,182,"OSCILLATOR 1");
    lv_obj_t *b=panel(p,160,4,151,182,"OSCILLATOR 2");
    lv_obj_t *c=panel(p,316,4,151,182,"OSCILLATOR 3");

    control(a,7,34,137,40,"RANGE",&B_O1R);
    control(a,7,80,137,40,"WAVEFORM",&B_O1W);
    control(a,7,126,137,40,"LEVEL",&B_O1L);

    control(b,7,26,137,33,"RANGE",&B_O2R);
    control(b,7,62,137,33,"WAVEFORM",&B_O2W);
    control(b,7,98,137,33,"TUNE",&B_O2T);
    control(b,7,134,137,33,"LEVEL",&B_O2L);

    control(c,7,22,137,28,"RANGE",&B_O3R);
    control(c,7,52,137,28,"WAVEFORM",&B_O3W);
    control(c,7,82,137,28,"TUNE",&B_O3T);
    control(c,7,112,137,28,"LEVEL",&B_O3L);
    control(c,7,142,137,28,"KEYBOARD",&B_O3K);
}

static void makeMixPage()
{
    lv_obj_t *p=pages[PAGE_MIX];
    lv_obj_t *m=panel(p,5,5,270,180,"MIXER");
    control(m,8,30,118,42,"OSC 1 LEVEL",&B_O1L);
    control(m,136,30,118,42,"OSC 2 LEVEL",&B_O2L);
    control(m,8,80,118,42,"OSC 3 LEVEL",&B_O3L);
    control(m,136,80,118,42,"NOISE",&B_NOISE);
    control(m,8,130,118,38,"DRIVE",&B_DRIVE);
    control(m,136,130,118,38,"FEEDBACK",&B_FB);

    lv_obj_t *g=panel(p,282,5,177,180,"PERFORMANCE");
    // v0.5.7: complete the classic global pitch section without growing
    // the object tree: four compact controls replace the previous three.
    control(g,10,27,151,31,"MASTER",&B_MASTER);
    control(g,10,63,151,31,"MASTER TUNE",&B_MTUNE);
    control(g,10,99,151,31,"GLIDE TIME",&B_GLIDE);
    control(g,10,135,151,31,"GLIDE",&B_GLIDEEN);
}

static void makeFilterPage()
{
    lv_obj_t *p=pages[PAGE_FILTER];
    lv_obj_t *f=panel(p,5,5,190,180,"LADDER FILTER");
    control(f,8,30,168,38,"CUTOFF",&B_CUT);
    control(f,8,74,168,38,"EMPHASIS",&B_RES);
    control(f,8,118,168,38,"KEY TRACK",&B_KT);

    lv_obj_t *fc=panel(p,202,5,126,180,"FILTER CONTOUR");
    control(fc,7,27,106,25,"ATTACK",&B_FA);
    control(fc,7,56,106,25,"DECAY",&B_FD);
    control(fc,7,85,106,25,"SUSTAIN",&B_FS);
    control(fc,7,114,106,25,"RELEASE",&B_FR);
    control(fc,7,143,106,25,"AMOUNT",&B_CONT);

    lv_obj_t *ac=panel(p,335,5,124,180,"LOUDNESS");
    control(ac,7,27,104,25,"ATTACK",&B_AA);
    control(ac,7,56,104,25,"DECAY",&B_AD);
    control(ac,7,85,104,25,"SUSTAIN",&B_AS);
    control(ac,7,114,104,25,"RELEASE",&B_AR);
    control(ac,7,143,104,25,"DECAY SW",&B_DEC);
}


static void makeModPage()
{
    lv_obj_t *p=pages[PAGE_MOD];
    lv_obj_t *m=panel(p,5,5,220,180,"MODULATION");
    control(m,8,30,196,38,"MOD WHEEL",&B_MW);
    control(m,8,74,196,38,"OSC3 / NOISE MIX",&B_MM);
    control(m,8,118,94,38,"OSC MOD",&B_OM);
    control(m,110,118,94,38,"FILTER MOD",&B_FM);

    lv_obj_t *n=panel(p,232,5,110,180,"NOISE / UI");
    makeMidiChannelControl(n,7,29,90,34);
    control(n,7,69,90,34,"COLOR",&B_NCOL);
    control(n,7,109,90,34,"OVL TIME",&B_OTIME);

    lv_obj_t *k=panel(p,349,5,110,180,"KEYBOARD");
    control(k,7,29,90,38,"PRIORITY",&B_PRI);
    control(k,7,73,90,38,"TRIGGER",&B_TRG);
    control(k,7,117,90,38,"OSC3 KEY",&B_O3K);
}



static void updatePresetHeader()
{
    if(!presetHeaderLabel)
        return;

    char b[64];
    snprintf(b, sizeof(b), "%03u  %s",
             (unsigned)currentPresetSlot,
             currentPresetName);
    lv_label_set_text(presetHeaderLabel, b);
}

static void updatePresetBrowserLabels()
{
    char slotText[16];
    snprintf(slotText, sizeof(slotText), "%03u", (unsigned)currentPresetSlot);

    if(presetSlotLabel)
        lv_label_set_text(presetSlotLabel, slotText);

    char name[32];
    if(PresetManager::instance().readName(currentPresetSlot, name, sizeof(name))) {
        snprintf(currentPresetName, sizeof(currentPresetName), "%s", name);
    } else {
        snprintf(currentPresetName, sizeof(currentPresetName), "EMPTY");
    }

    if(presetNameLabel)
        lv_label_set_text(presetNameLabel, currentPresetName);

    if(presetStatusLabel) {
        lv_label_set_text(
            presetStatusLabel,
            PresetManager::instance().exists(currentPresetSlot)
                ? "READY"
                : "EMPTY SLOT"
        );
    }

    updatePresetHeader();
}

static void presetBrowseEvent(lv_event_t *e)
{
    if(!e)
        return;

    const intptr_t d=(intptr_t)lv_event_get_user_data(e);
    if(d != -1 && d != 1)
        return;

    presetBrowseDelta=(int)d;
    presetBrowsePending=true;
    presetBrowseRequestedAt=millis();

    Serial.printf("[PRESET BROWSE] request delta=%d slot=%03u\n",
                  presetBrowseDelta,
                  (unsigned)currentPresetSlot);
    Serial.flush();
}

static void serviceDeferredPresetBrowse()
{
    if(!presetBrowsePending)
        return;

    if(Board::touchPressed())
        return;

    if((uint32_t)(millis() - Board::touchLastChangeMs()) < 35)
        return;

    if((uint32_t)(millis() - presetBrowseRequestedAt) < 60)
        return;

    presetBrowsePending=false;

    int next=(int)currentPresetSlot + presetBrowseDelta;
    if(next < (int)PRESET_SLOT_MIN)
        next=(int)PRESET_SLOT_MAX;
    if(next > (int)PRESET_SLOT_MAX)
        next=(int)PRESET_SLOT_MIN;

    currentPresetSlot=(uint16_t)next;
    updatePresetBrowserLabels();

    Serial.printf("[PRESET BROWSE] slot=%03u name=%s exists=%d\n",
                  (unsigned)currentPresetSlot,
                  currentPresetName,
                  PresetManager::instance().exists(currentPresetSlot) ? 1 : 0);
    Serial.flush();
}


static void updatePresetNameEditLabel()
{
    if(!presetNameTextLabel) return;
    char b[32];
    snprintf(b,sizeof(b),"%s_",presetEditName);
    lv_label_set_text(presetNameTextLabel,b);
}

static void destroyPresetNameOverlay()
{
    if(presetNameOverlay)
        lv_obj_del(presetNameOverlay);
    presetNameOverlay=nullptr;
    presetNameTextLabel=nullptr;
    presetNameMatrix=nullptr;
}














static void presetNameMatrixEvent(lv_event_t *e)
{
    if(!e || !presetNameMatrix) return;

    uint32_t id=lv_btnmatrix_get_selected_btn(presetNameMatrix);
    if(id==LV_BTNMATRIX_BTN_NONE) return;

    const char *txt=lv_btnmatrix_get_btn_text(presetNameMatrix,id);
    if(!txt) return;

    if(strcmp(txt,"OK")==0) {
        presetNameCommitPending=true;
        presetNameClosePending=true;
        presetNameCloseRequestedAt=millis();
        return;
    }

    if(strcmp(txt,"CANCEL")==0) {
        presetNameCommitPending=false;
        presetNameClosePending=true;
        presetNameCloseRequestedAt=millis();
        return;
    }

    if(strcmp(txt,"<-")==0) {
        size_t len=strlen(presetEditName);
        if(len>0) presetEditName[len-1]=0;
        updatePresetNameEditLabel();
        return;
    }

    if(strcmp(txt,"SPACE")==0) {
        size_t len=strlen(presetEditName);
        if(len<20) {
            presetEditName[len]=' ';
            presetEditName[len+1]=0;
        }
        updatePresetNameEditLabel();
        return;
    }

    if(strlen(txt)==1) {
        size_t len=strlen(presetEditName);
        if(len<20) {
            presetEditName[len]=txt[0];
            presetEditName[len+1]=0;
        }
        updatePresetNameEditLabel();
    }
}

static void makePresetNameOverlay()
{
    destroyPresetNameOverlay();

    Serial.printf("[PRESET SAVEAS D01] create buttonmatrix editor heap=%u\n",
                  ESP.getFreeHeap());
    Serial.flush();

    presetNameOverlay=lv_obj_create(lv_scr_act());
    lv_obj_set_pos(presetNameOverlay,18,18);
    lv_obj_set_size(presetNameOverlay,444,216);
    lv_obj_set_style_bg_color(presetNameOverlay,lv_color_hex(0x121212),0);
    lv_obj_set_style_bg_opa(presetNameOverlay,248,0);
    lv_obj_set_style_border_color(presetNameOverlay,lv_color_hex(C_GOLD),0);
    lv_obj_set_style_border_width(presetNameOverlay,2,0);
    lv_obj_set_style_radius(presetNameOverlay,8,0);
    lv_obj_set_style_pad_all(presetNameOverlay,6,0);
    lv_obj_clear_flag(presetNameOverlay,LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title=lv_label_create(presetNameOverlay);
    lv_label_set_text(title,"SAVE AS - PRESET NAME");
    lv_obj_set_style_text_color(title,lv_color_hex(C_GOLD),0);
    lv_obj_align(title,LV_ALIGN_TOP_MID,0,2);

    presetNameTextLabel=lv_label_create(presetNameOverlay);
    lv_obj_set_style_text_color(presetNameTextLabel,lv_color_hex(C_TEXT),0);
    lv_obj_set_style_text_font(presetNameTextLabel,&lv_font_montserrat_16,0);
    lv_obj_align(presetNameTextLabel,LV_ALIGN_TOP_MID,0,25);
    updatePresetNameEditLabel();

    static const char *map[] = {
        "A","B","C","D","E","F","G","H","I","\n",
        "J","K","L","M","N","O","P","Q","R","\n",
        "S","T","U","V","W","X","Y","Z","0","\n",
        "1","2","3","4","5","6","7","8","9","\n",
        "SPACE","<-","CANCEL","OK",""
    };

    presetNameMatrix=lv_btnmatrix_create(presetNameOverlay);
    lv_btnmatrix_set_map(presetNameMatrix,map);
    lv_obj_set_size(presetNameMatrix,412,142);
    lv_obj_align(presetNameMatrix,LV_ALIGN_BOTTOM_MID,0,-2);
    lv_obj_set_style_text_font(presetNameMatrix,&lv_font_montserrat_12,0);
    lv_obj_add_event_cb(presetNameMatrix,presetNameMatrixEvent,
                        LV_EVENT_VALUE_CHANGED,nullptr);

    Serial.printf("[PRESET SAVEAS D02] editor ready heap=%u\n",
                  ESP.getFreeHeap());
    Serial.flush();
}

static void presetSaveAsEvent(lv_event_t *e)
{
    (void)e;
    presetNameOpenPending=true;
    presetNameOpenRequestedAt=millis();
    Serial.printf("[PRESET SAVEAS] requested slot=%03u\n",(unsigned)currentPresetSlot);
    Serial.flush();
}

static void serviceDeferredPresetNameOpen()
{
    if(!presetNameOpenPending) return;
    if(Board::touchPressed()) return;
    if((uint32_t)(millis()-Board::touchLastChangeMs())<35) return;
    if((uint32_t)(millis()-presetNameOpenRequestedAt)<80) return;

    presetNameOpenPending=false;

    char existing[32];
    if(PresetManager::instance().readName(currentPresetSlot,existing,sizeof(existing)))
        snprintf(presetEditName,sizeof(presetEditName),"%s",existing);
    else
        snprintf(presetEditName,sizeof(presetEditName),"Preset %03u",(unsigned)currentPresetSlot);

    makePresetNameOverlay();
    Serial.printf("[PRESET SAVEAS] editor open slot=%03u name=%s\n",
                  (unsigned)currentPresetSlot,presetEditName);
    Serial.flush();
}

static void serviceDeferredPresetNameClose()
{
    if(!presetNameClosePending) return;
    if(Board::touchPressed()) return;
    if((uint32_t)(millis()-Board::touchLastChangeMs())<35) return;
    if((uint32_t)(millis()-presetNameCloseRequestedAt)<60) return;

    presetNameClosePending=false;
    bool commit=presetNameCommitPending;
    presetNameCommitPending=false;

    char saveName[21];
    snprintf(saveName,sizeof(saveName),"%s",presetEditName);
    destroyPresetNameOverlay();

    if(!commit) {
        Serial.println("[PRESET SAVEAS] cancelled");
        Serial.flush();
        return;
    }

    if(saveName[0]==0)
        snprintf(saveName,sizeof(saveName),"Preset %03u",(unsigned)currentPresetSlot);

    bool ok=PresetManager::instance().ready() &&
            PresetManager::instance().save(currentPresetSlot,saveName);

    if(ok) {
        snprintf(currentPresetName,sizeof(currentPresetName),"%s",saveName);
        updatePresetBrowserLabels();
        updatePresetHeader();
        if(presetStatusLabel) lv_label_set_text(presetStatusLabel,"SAVE AS OK");
        Serial.printf("[PRESET SAVEAS] OK slot=%03u name=%s\n",
                      (unsigned)currentPresetSlot,currentPresetName);
    } else {
        if(presetStatusLabel) lv_label_set_text(presetStatusLabel,"SAVE AS FAILED");
        Serial.printf("[PRESET SAVEAS] FAILED slot=%03u\n",(unsigned)currentPresetSlot);
    }
    Serial.flush();
}















static void serviceDeferredPresetManagement()
{
    if(!presetMgmtExecutePending)
        return;

    if(Board::touchPressed())
        return;

    if((uint32_t)(millis()-Board::touchLastChangeMs())<35)
        return;

    if((uint32_t)(millis()-presetMgmtExecuteRequestedAt)<60)
        return;

    presetMgmtExecutePending=false;

    const int action=presetConfirmAction;
    presetConfirmAction=0;

    bool ok=false;

    if(action==1) {
        Parameters::instance().applyInitPatch();
        snprintf(currentPresetName,sizeof(currentPresetName),"INIT");
        updatePresetHeader();

        // Keep the safe page-local refresh architecture.
        pageValuesDirty=true;
        refreshPageControls(activePage);

        if(presetStatusLabel)
            lv_label_set_text(presetStatusLabel,"INIT OK");

        ok=true;
    }
    else if(action==2) {
        if(PresetManager::instance().exists(currentPresetSlot))
            ok=PresetManager::instance().remove(currentPresetSlot);
        else
            ok=true;

        if(ok) {
            updatePresetBrowserLabels();
            if(presetStatusLabel)
                lv_label_set_text(presetStatusLabel,"DELETE OK");
        } else {
            if(presetStatusLabel)
                lv_label_set_text(presetStatusLabel,"DELETE FAILED");
        }
    }

    Serial.printf("[PRESET MGMT] %s %s slot=%03u\n",
                  action==1?"INIT":"DELETE",
                  ok?"OK":"FAILED",
                  (unsigned)currentPresetSlot);
    Serial.flush();
}


static void serviceDeferredMidiProgramChange()
{
    if(!midiProgramChangePending)
        return;

    if(Board::touchPressed())
        return;

    if((uint32_t)(millis()-Board::touchLastChangeMs())<35)
        return;

    midiProgramChangePending=false;

    currentPresetSlot=midiProgramChangeSlot;
    updatePresetBrowserLabels();

    if(!PresetManager::instance().exists(currentPresetSlot)) {
        if(presetStatusLabel)
            lv_label_set_text(presetStatusLabel,"PC EMPTY SLOT");

        Serial.printf("[MIDI PC] slot=%03u EMPTY - load skipped\n",
                      (unsigned)currentPresetSlot);
        Serial.flush();
        return;
    }

    presetSavePending=false;
    presetActionPending=true;
    presetActionRequestedAt=millis();

    Serial.printf("[MIDI PC] slot=%03u load deferred\n",
                  (unsigned)currentPresetSlot);
    Serial.flush();
}

static void presetAction(lv_event_t *e)
{
    if(!e)
        return;

    const intptr_t action=(intptr_t)lv_event_get_user_data(e);
    if(action != 0 && action != 1)
        return;

    presetSavePending=(action == 1);
    presetActionPending=true;
    presetActionRequestedAt=millis();

    Serial.printf("[PRESET] %s requested slot=%03u heap=%u\n",
                  presetSavePending ? "SAVE" : "LOAD",
                  (unsigned)currentPresetSlot,
                  ESP.getFreeHeap());
    Serial.flush();
}



static void serviceDeferredPresetAction()
{
    if(!presetActionPending)
        return;

    // Same b9 rule that fixed the parameter-modal crashes:
    // never change UI/perform the blocking SD operation while the initiating
    // touch is still active.
    if(Board::touchPressed())
        return;

    if((uint32_t)(millis() - Board::touchLastChangeMs()) < 50)
        return;

    presetActionPending=false;
    const bool doSave=presetSavePending;

    if(presetStatusLabel)
        lv_label_set_text(presetStatusLabel,
                          doSave ? "SAVING..." : "LOADING...");

    Serial.printf("[PRESET D01] %s begin slot=%03u heap=%u\n",
                  doSave ? "SAVE" : "LOAD",
                  (unsigned)currentPresetSlot,
                  ESP.getFreeHeap());
    Serial.flush();

    const uint32_t t0=millis();
    bool ok=false;

    if(PresetManager::instance().ready()) {
        if(doSave) {
            char saveName[32];

            // `EMPTY` is a browser state, never a preset name.
            // On first save into an unused slot, create a real default name.
            if(!PresetManager::instance().exists(currentPresetSlot) ||
               strcmp(currentPresetName, "EMPTY") == 0) {
                snprintf(saveName, sizeof(saveName),
                         "Preset %03u",
                         (unsigned)currentPresetSlot);
            } else {
                snprintf(saveName, sizeof(saveName),
                         "%s",
                         currentPresetName);
            }

            ok=PresetManager::instance().save(currentPresetSlot, saveName);

            if(ok)
                snprintf(currentPresetName, sizeof(currentPresetName), "%s", saveName);
        } else {
            ok=PresetManager::instance().load(currentPresetSlot);
        }
    }

    const uint32_t dt=millis()-t0;

    if(ok) {
        char name[32];
        if(PresetManager::instance().readName(currentPresetSlot, name, sizeof(name)))
            snprintf(currentPresetName, sizeof(currentPresetName), "%s", name);
        else if(doSave)
            snprintf(currentPresetName, sizeof(currentPresetName), "Preset %03u",
                     (unsigned)currentPresetSlot);

        updatePresetBrowserLabels();
        updatePresetHeader();
    }

    if(ok && !doSave) {
        // Loaded parameter targets are already active in the audio engine.
        // Do not globally refresh all LVGL controls. Mark page values dirty;
        // only the currently shown page (and later pages when selected) will
        // be refreshed.
        lastRevision=Parameters::instance().revision();
        pageValuesDirty=true;
    }

    if(presetStatusLabel) {
        if(ok)
            lv_label_set_text(presetStatusLabel, doSave ? "SAVE OK" : "LOAD OK");
        else
            lv_label_set_text(presetStatusLabel, doSave ? "SAVE FAILED" : "LOAD FAILED");
    }

    if(ok && !doSave)
        refreshPageControls(activePage);

    Serial.printf("[PRESET D02] %s %s dt=%lu ms heap=%u minHeap=%u\n",
                  doSave ? "SAVE" : "LOAD",
                  ok ? "OK" : "FAILED",
                  (unsigned long)dt,
                  ESP.getFreeHeap(),
                  ESP.getMinFreeHeap());
    Serial.flush();
    if(ok && doSave) {
        Serial.printf("[PRESET D03] saved slot=%03u name=%s\n",
                      (unsigned)currentPresetSlot,
                      currentPresetName);
        Serial.flush();
    }
    if(ok && !doSave) {
        Serial.println("[PRESET D03] LOAD applied - page-local refresh pending");
        Serial.flush();
    }
}


static void presetActionMatrixEvent(lv_event_t *e)
{
    if(!e || !presetActionMatrix)
        return;

    uint32_t id=lv_btnmatrix_get_selected_btn(presetActionMatrix);
    if(id==LV_BTNMATRIX_BTN_NONE)
        return;

    const char *txt=lv_btnmatrix_get_btn_text(presetActionMatrix,id);
    if(!txt)
        return;

    if(strcmp(txt,"LOAD")==0) {
        presetSavePending=false;
        presetActionPending=true;
        presetActionRequestedAt=millis();
        Serial.printf("[PRESET] LOAD requested slot=%03u heap=%u\n",
                      (unsigned)currentPresetSlot,ESP.getFreeHeap());
    }
    else if(strcmp(txt,"SAVE")==0) {
        presetSavePending=true;
        presetActionPending=true;
        presetActionRequestedAt=millis();
        Serial.printf("[PRESET] SAVE requested slot=%03u heap=%u\n",
                      (unsigned)currentPresetSlot,ESP.getFreeHeap());
    }
    else if(strcmp(txt,"SAVE AS")==0) {
        presetNameOpenPending=true;
        presetNameOpenRequestedAt=millis();
        Serial.printf("[PRESET SAVEAS] requested slot=%03u\n",
                      (unsigned)currentPresetSlot);
    }
    else if(strcmp(txt,"INIT")==0 || strcmp(txt,"DELETE")==0) {
        const int action=(strcmp(txt,"INIT")==0)?1:2;
        const uint32_t now=millis();

        if(presetMgmtArmedAction!=action ||
           (uint32_t)(now-presetMgmtArmedAt)>3000) {
            presetMgmtArmedAction=action;
            presetMgmtArmedAt=now;

            if(presetStatusLabel)
                lv_label_set_text(
                    presetStatusLabel,
                    action==1?"INIT? TAP AGAIN":"DELETE? TAP AGAIN"
                );

            Serial.printf("[PRESET MGMT] armed %s slot=%03u\n",
                          action==1?"INIT":"DELETE",
                          (unsigned)currentPresetSlot);
        } else {
            presetMgmtArmedAction=0;
            presetMgmtExecutePending=true;
            presetConfirmAction=action;
            presetMgmtExecuteRequestedAt=now;

            Serial.printf("[PRESET MGMT] confirmed %s slot=%03u\n",
                          action==1?"INIT":"DELETE",
                          (unsigned)currentPresetSlot);
        }
    }

    Serial.flush();
}

static void makePresetPage()
{
    lv_obj_t *p=pages[PAGE_PRESET];
    lv_obj_t *q=panel(p,18,8,444,174,"PRESET BROWSER");

    // Only two conventional buttons remain on the page: previous / next.
    lv_obj_t *prev=lv_btn_create(q);
    presetPrevButton=prev;
    lv_obj_set_size(prev,54,38);
    lv_obj_align(prev,LV_ALIGN_LEFT_MID,14,-30);
    lv_obj_t *pl=lv_label_create(prev);
    lv_label_set_text(pl,"<");
    lv_obj_center(pl);
    lv_obj_add_event_cb(prev,presetBrowseEvent,LV_EVENT_CLICKED,(void*)(intptr_t)-1);

    lv_obj_t *next=lv_btn_create(q);
    presetNextButton=next;
    lv_obj_set_size(next,54,38);
    lv_obj_align(next,LV_ALIGN_RIGHT_MID,-14,-30);
    lv_obj_t *nl=lv_label_create(next);
    lv_label_set_text(nl,">");
    lv_obj_center(nl);
    lv_obj_add_event_cb(next,presetBrowseEvent,LV_EVENT_CLICKED,(void*)(intptr_t)1);

    presetSlotLabel=lv_label_create(q);
    lv_obj_set_style_text_color(presetSlotLabel,lv_color_hex(C_GOLD),0);
    lv_obj_set_style_text_font(presetSlotLabel,&lv_font_montserrat_16,0);
    lv_obj_align(presetSlotLabel,LV_ALIGN_TOP_MID,0,20);

    presetNameLabel=lv_label_create(q);
    lv_obj_set_style_text_color(presetNameLabel,lv_color_hex(C_TEXT),0);
    lv_obj_set_style_text_font(presetNameLabel,&lv_font_montserrat_14,0);
    lv_obj_align(presetNameLabel,LV_ALIGN_TOP_MID,0,44);

    presetStatusLabel=lv_label_create(q);
    lv_obj_set_style_text_color(presetStatusLabel,lv_color_hex(C_DIM),0);
    lv_obj_align(presetStatusLabel,LV_ALIGN_TOP_MID,0,65);

    static const char *actionMap[] = {
        "LOAD","SAVE","SAVE AS","\n",
        "INIT","DELETE",""
    };

    presetActionMatrix=lv_btnmatrix_create(q);
    lv_btnmatrix_set_map(presetActionMatrix,actionMap);
    lv_obj_set_size(presetActionMatrix,404,68);
    lv_obj_align(presetActionMatrix,LV_ALIGN_BOTTOM_MID,0,-5);
    lv_obj_set_style_text_font(presetActionMatrix,&lv_font_montserrat_12,0);
    lv_obj_add_event_cb(
        presetActionMatrix,
        presetActionMatrixEvent,
        LV_EVENT_VALUE_CHANGED,
        nullptr
    );

    updatePresetBrowserLabels();

    if(presetStatusLabel)
        lv_label_set_text(
            presetStatusLabel,
            PresetManager::instance().exists(currentPresetSlot)
                ? "READY"
                : "EMPTY SLOT"
        );

    Serial.printf("[PRESET UI] compact action matrix ready heap=%u\n",
                  ESP.getFreeHeap());
    Serial.flush();
}

static void makeEfxPage()
{
    lv_obj_t *p=pages[PAGE_EFX];
    lv_obj_t *q=panel(p,18,8,444,174,"STEREO DELAY");

    // 0.7.3a: 6 + 5 compact grid. Existing WIDTH remains unchanged;
    // STEREO WIDTH is a separate post-delay Mid/Side stage (0..200%).
    control(q,5,28,68,52,"DELAY",&B_DEN);
    control(q,78,28,68,52,"TIME",&B_DTIME);
    control(q,151,28,68,52,"FEEDBACK",&B_DFB);
    control(q,224,28,68,52,"MIX",&B_DMIX);
    control(q,297,28,68,52,"FILTER",&B_DFILT);
    control(q,370,28,68,52,"WIDTH",&B_DWIDTH);
    control(q,5,88,82,52,"ST WIDTH",&B_DSTWIDTH);
    control(q,92,88,82,52,"PING MODE",&B_DPMODE);
    control(q,179,88,82,52,"PING AMT",&B_DPAMT);
    control(q,266,88,82,52,"SYNC",&B_DSYNC);
    control(q,353,88,82,52,"DIV",&B_DDIV);

    // 0.6.9a: no redundant SYNC/DIV help text; controls are self-explanatory.
}


static void showPage(Page pg)
{
    activePage=pg;
    for(int i=0;i<PAGE_COUNT;i++) {
        if(i==(int)pg) lv_obj_clear_flag(pages[i],LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(pages[i],LV_OBJ_FLAG_HIDDEN);

        lv_obj_set_style_bg_color(navButtons[i],
            lv_color_hex(i==(int)pg ? 0x7A5731 : 0x242424),0);
    }
}

static void navCB(lv_event_t *e)
{
    if(!e)
        return;

    const intptr_t p=(intptr_t)lv_event_get_user_data(e);
    if(p < 0 || p >= PAGE_COUNT)
        return;

    pendingPage=(Page)p;
    pageChangePending=true;
    pageChangeRequestedAt=millis();

    Serial.printf("[NAV] requested page=%ld heap=%u\n",
                  (long)p,
                  ESP.getFreeHeap());
    Serial.flush();

    // Do not hide/show pages or restyle tabs inside LVGL's event callback.
}


static void updateKeyboardVisuals()
{
    if(!keyboardOverlay || !keyboardNoteLabel)
        return;

    const int note = SynthEngine::instance().currentNote();
    const bool gate = SynthEngine::instance().gateActive();

    if(note == lastVisualNote && gate == lastVisualGate)
        return;

    lastVisualNote = note;
    lastVisualGate = gate;

    char b[32];

    if(gate) {
        static const char *names[] = {
            "C","C#","D","D#","E","F",
            "F#","G","G#","A","A#","B"
        };

        const int oct = (note / 12) - 1;
        snprintf(
            b,
            sizeof(b),
            "%s%d  MIDI %d",
            names[note % 12],
            oct,
            note
        );
    } else {
        snprintf(b, sizeof(b), "--");
    }

    lv_label_set_text(keyboardNoteLabel, b);

    for(int semi=0; semi<24; ++semi) {
        if(!keyboardKey[semi])
            continue;

        const int mapped = keyboardBaseNote + semi;
        const bool active = gate && mapped == note;
        const bool black = isBlackSemitone(semi);

        lv_obj_set_style_bg_color(
            keyboardKey[semi],
            lv_color_hex(
                active ? 0xB88445 :
                (black ? 0x161616 : 0xE8E2D6)
            ),
            0
        );
    }
}

static void keyboardKeyEvent(lv_event_t *e)
{
    if(!e) return;

    const lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);

    if(!target || target == keyboardOverlay)
        return;

    int idx = -1;
    for(int i=0; i<24; ++i) {
        if(keyboardKey[i] == target) {
            idx = i;
            break;
        }
    }

    if(idx < 0)
        return;

    // keyboardKey[] is indexed directly by semitone 0..23.
    const uint8_t note = (uint8_t)(keyboardBaseNote + idx);

    if(code == LV_EVENT_PRESSED) {
        Serial.printf("[KEY AUDIO] DOWN idx=%d note=%u hold=%d\n",
                      idx, note, keyboardHold ? 1 : 0);
        Serial.flush();

        if(keyboardHold) {
            // Minimoog-style monophonic latch: replace the previously latched
            // note with the newly selected key.
            if(keyboardLatchedNote >= 0 &&
               keyboardLatchedNote != (int)note) {
                SynthEngine::instance().noteOff(
                    (uint8_t)keyboardLatchedNote
                );
            }

            keyboardLatchedNote = note;
            keyboardTouchNote = -1;
            SynthEngine::instance().noteOn(note, 100);
        } else {
            if(keyboardTouchNote >= 0 &&
               keyboardTouchNote != (int)note) {
                SynthEngine::instance().noteOff(
                    (uint8_t)keyboardTouchNote
                );
            }

            keyboardTouchNote = note;
            SynthEngine::instance().noteOn(note, 100);
        }

        lv_obj_set_style_border_width(target, 3, 0);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        Serial.printf("[KEY AUDIO] UP idx=%d note=%u hold=%d\n",
                      idx, note, keyboardHold ? 1 : 0);
        Serial.flush();

        lv_obj_set_style_border_width(target, 1, 0);

        if(!keyboardHold && keyboardTouchNote == (int)note) {
            SynthEngine::instance().noteOff(note);
            keyboardTouchNote = -1;
        }
    }
}

static void keyboardOctaveEvent(lv_event_t *e)
{
    intptr_t dir=(intptr_t)lv_event_get_user_data(e);
    keyboardAllNotesOff();
    keyboardBaseNote += dir>0 ? 12 : -12;
    if(keyboardBaseNote<24) keyboardBaseNote=24;
    if(keyboardBaseNote>72) keyboardBaseNote=72;

    // Update event userdata by recreating visible note values in labels only.
    // Buttons are recreated on octave change for deterministic mapping.
    lv_obj_clean(keyboardOverlay);

    // Rebuild complete overlay content.
    // forward declaration workaround through local lambda is avoided:
    // makeKeyboardOverlayContent() below is called via helper.
}

static void setKeyboardHold(bool on)
{
    if(keyboardHold==on) return;
    keyboardHold=on;
    if(!keyboardHold && keyboardLatchedNote>=0) {
        SynthEngine::instance().noteOff((uint8_t)keyboardLatchedNote);
        keyboardLatchedNote=-1;
    }
    if(keyboardHoldLabel)
        lv_label_set_text(keyboardHoldLabel,keyboardHold ? "HOLD ON" : "HOLD");
}

static void keyboardHoldEvent(lv_event_t *e)
{
    (void)e;
    setKeyboardHold(!keyboardHold);
}

static void closeKeyboardEvent(lv_event_t *e)
{
    (void)e;
    keyboardClosePending=true;
    keyboardCloseRequestedAt=millis();
    Serial.printf("[KEYS CLOSE] requested heap=%u\n", ESP.getFreeHeap());
    Serial.flush();
}

static void buildKeyboardKeys()
{
    Serial.println("[BUBBLE24 01] begin");
    Serial.flush();

    if(!keyboardOverlay) {
        Serial.println("[BUBBLE24 02] ERROR parent null");
        Serial.flush();
        return;
    }

    for(auto &k : keyboardKey) {
        k = nullptr;
    }

    static const uint8_t whiteSemis[14] = {
        0,2,4,5,7,9,11,
        12,14,16,17,19,21,23
    };

    static const uint8_t blackSemis[10] = {
        1,3,6,8,10,
        13,15,18,20,22
    };

    // Center each 23 px black key over the gap between its two
    // neighbouring 31 px white keys (2 px inter-key gap).
    static const int16_t blackX[10] = {
        26, 59, 125, 158, 191,
        257, 290, 356, 389, 422
    };

    const int16_t whiteY = 34;
    const int16_t whiteW = 31;
    const int16_t whiteH = 118;
    const int16_t whiteGap = 2;

    for(int i=0; i<14; ++i) {
        const int idx = whiteSemis[i];
        const int16_t x = 5 + i * (whiteW + whiteGap);

        lv_obj_t *key = lv_obj_create(keyboardOverlay);
        if(!key) return;

        keyboardKey[idx] = key;

        lv_obj_set_pos(key, x, whiteY);
        lv_obj_set_size(key, whiteW, whiteH);
        lv_obj_set_style_pad_all(key, 0, 0);
        lv_obj_set_style_radius(key, 0, 0);
        lv_obj_set_style_bg_color(key, lv_color_hex(0xE8E2D6), 0);
        lv_obj_set_style_bg_opa(key, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(key, lv_color_hex(0x202020), 0);
        lv_obj_set_style_border_width(key, 1, 0);
        lv_obj_clear_flag(key, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_add_flag(key, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(key, LV_OBJ_FLAG_EVENT_BUBBLE);
    }

    Serial.println("[BUBBLE24 03] white keys ready");
    Serial.flush();

    for(int i=0; i<10; ++i) {
        const int idx = blackSemis[i];

        lv_obj_t *key = lv_obj_create(keyboardOverlay);
        if(!key) return;

        keyboardKey[idx] = key;

        lv_obj_set_pos(key, blackX[i], whiteY);
        lv_obj_set_size(key, 23, 72);
        lv_obj_set_style_pad_all(key, 0, 0);
        lv_obj_set_style_radius(key, 0, 0);
        lv_obj_set_style_bg_color(key, lv_color_hex(0x161616), 0);
        lv_obj_set_style_bg_opa(key, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(key, lv_color_hex(0x080808), 0);
        lv_obj_set_style_border_width(key, 1, 0);
        lv_obj_clear_flag(key, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_add_flag(key, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(key, LV_OBJ_FLAG_EVENT_BUBBLE);
    }

    Serial.println("[BUBBLE24 04] all 24 keys ready");
    Serial.flush();

    // ONE callback total for the complete keyboard.
    lv_obj_add_event_cb(
        keyboardOverlay,
        keyboardKeyEvent,
        LV_EVENT_PRESSED,
        nullptr
    );

    lv_obj_add_event_cb(
        keyboardOverlay,
        keyboardKeyEvent,
        LV_EVENT_RELEASED,
        nullptr
    );

    lv_obj_add_event_cb(
        keyboardOverlay,
        keyboardKeyEvent,
        LV_EVENT_PRESS_LOST,
        nullptr
    );

    Serial.println("[BUBBLE24 05] central overlay callbacks ready");
    Serial.printf("[BUBBLE24 06] heap=%u\n", ESP.getFreeHeap());
    Serial.flush();
}

static void rebuildKeyboardOverlay()
{
    if(!keyboardOverlay) {
        Serial.println("[KEYS R00] ERROR: keyboardOverlay null");
        return;
    }

    Serial.printf("[KEYS R01] clean start heap=%u\n", ESP.getFreeHeap());

    // All child objects are destroyed by lv_obj_clean(). Never leave global
    // pointers referring to those deleted LVGL objects.
    lv_obj_clean(keyboardOverlay);

    keyboardNoteLabel = nullptr;
    keyboardHoldLabel = nullptr;
    for(auto &k : keyboardKey)
        k = nullptr;

    Serial.printf("[KEYS R02] clean complete heap=%u\n", ESP.getFreeHeap());

    lv_obj_t *title = lv_label_create(keyboardOverlay);
    if(!title) {
        Serial.println("[KEYS R03] ERROR title create");
        return;
    }
    lv_label_set_text(title,"TOUCH KEYBOARD");
    lv_obj_set_style_text_color(title,lv_color_hex(C_GOLD),0);
    lv_obj_set_pos(title,8,7);
    Serial.println("[KEYS R03] title OK");

    keyboardNoteLabel = lv_label_create(keyboardOverlay);
    if(!keyboardNoteLabel) {
        Serial.println("[KEYS R04] ERROR note label create");
        return;
    }
    lv_label_set_text(keyboardNoteLabel,"--");
    lv_obj_set_style_text_color(keyboardNoteLabel,lv_color_hex(C_TEXT),0);
    lv_obj_align(keyboardNoteLabel,LV_ALIGN_TOP_MID,0,7);
    Serial.println("[KEYS R04] note label OK");

    char range[32];
    int oct=(keyboardBaseNote/12)-1;
    snprintf(range,sizeof(range),"C%d - B%d",oct,oct+1);

    lv_obj_t *rl=lv_label_create(keyboardOverlay);
    if(!rl) {
        Serial.println("[KEYS R05] ERROR range label create");
        return;
    }
    lv_label_set_text(rl,range);
    lv_obj_set_style_text_color(rl,lv_color_hex(C_DIM),0);
    lv_obj_align(rl,LV_ALIGN_TOP_RIGHT,-8,7);
    Serial.println("[KEYS R05] range label OK");

    Serial.printf("[KEYS R06] before key build heap=%u\n", ESP.getFreeHeap());
    buildKeyboardKeys();
    Serial.printf("[KEYS R07] key build OK heap=%u\n", ESP.getFreeHeap());

    lv_obj_t *octm=lv_btn_create(keyboardOverlay);
    lv_obj_set_pos(octm,8,160);
    lv_obj_set_size(octm,75,30);
    lv_obj_t *om=lv_label_create(octm);
    lv_label_set_text(om,"OCT -");
    lv_obj_center(om);
    Serial.println("[KEYS R08] OCT- OK");

    lv_obj_t *hold=lv_btn_create(keyboardOverlay);
    lv_obj_set_pos(hold,90,160);
    lv_obj_set_size(hold,90,30);
    keyboardHoldLabel=lv_label_create(hold);
    lv_label_set_text(keyboardHoldLabel,keyboardHold?"HOLD ON":"HOLD");
    lv_obj_center(keyboardHoldLabel);
    lv_obj_add_event_cb(hold,keyboardHoldEvent,LV_EVENT_CLICKED,nullptr);
    Serial.println("[KEYS R09] HOLD OK");

    lv_obj_t *octp=lv_btn_create(keyboardOverlay);
    lv_obj_set_pos(octp,187,160);
    lv_obj_set_size(octp,75,30);
    lv_obj_t *op=lv_label_create(octp);
    lv_label_set_text(op,"OCT +");
    lv_obj_center(op);
    Serial.println("[KEYS R10] OCT+ OK");

    lv_obj_t *close=lv_btn_create(keyboardOverlay);
    lv_obj_set_pos(close,377,160);
    lv_obj_set_size(close,95,30);
    lv_obj_t *cl=lv_label_create(close);
    lv_label_set_text(cl,"CLOSE");
    lv_obj_center(cl);
    lv_obj_add_event_cb(close,closeKeyboardEvent,LV_EVENT_CLICKED,nullptr);
    Serial.println("[KEYS R11] CLOSE OK");

    lv_obj_add_event_cb(octm,[](lv_event_t *e){
        (void)e;
        keyboardAllNotesOff();
        keyboardBaseNote-=12;
        if(keyboardBaseNote<24) keyboardBaseNote=24;
        rebuildKeyboardOverlay();
    },LV_EVENT_CLICKED,nullptr);

    lv_obj_add_event_cb(octp,[](lv_event_t *e){
        (void)e;
        keyboardAllNotesOff();
        keyboardBaseNote+=12;
        if(keyboardBaseNote>72) keyboardBaseNote=72;
        rebuildKeyboardOverlay();
    },LV_EVENT_CLICKED,nullptr);

    Serial.printf("[KEYS R12] callbacks OK heap=%u\n", ESP.getFreeHeap());

    // v0.5.5a: SynthEngine is initialized before the keyboard can be opened.
    // Force the first visual refresh after rebuild.
    lastVisualNote = -999;
    lastVisualGate = false;

    Serial.printf("[KEYS R13] rebuild COMPLETE heap=%u\n", ESP.getFreeHeap());
}

static void showKeyboardEvent(lv_event_t *e)
{
    (void)e;

#if RTAL_UI_KEYBOARD
    Serial.printf("[KEYS] open requested, heap=%u\n", ESP.getFreeHeap());
    Serial.flush();

    keyboardClosePending = false;
    keyboardOpenPending = true;
    keyboardOpenRequestedAt = millis();

    // IMPORTANT:
    // Do not create, clean, rebuild, show or foreground the keyboard here.
    // This callback runs inside LVGL's event-dispatch path.
#else
    Serial.println("[UI] KEYS disabled");
#endif
}

static void makeKeyboardOverlay()
{
    if(keyboardOverlay) {
        Serial.println("[KEYS] overlay already exists");
        return;
    }

    Serial.printf("[KEYS 01] create overlay, heap=%u\n", ESP.getFreeHeap());

    keyboardOverlay=lv_obj_create(lv_scr_act());
    if(!keyboardOverlay) {
        Serial.println("[KEYS] FATAL: overlay allocation failed");
        return;
    }

    Serial.printf("[KEYS 02] overlay object OK, heap=%u\n", ESP.getFreeHeap());

    lv_obj_set_pos(keyboardOverlay,0,25);
    lv_obj_set_size(keyboardOverlay,480,205);
    lv_obj_set_style_bg_color(keyboardOverlay,lv_color_hex(0x0B0B0B),0);
    lv_obj_set_style_bg_opa(keyboardOverlay,LV_OPA_COVER,0);
    lv_obj_set_style_border_width(keyboardOverlay,0,0);
    lv_obj_set_style_pad_all(keyboardOverlay,0,0);
    lv_obj_clear_flag(keyboardOverlay,LV_OBJ_FLAG_SCROLLABLE);

    Serial.printf("[KEYS 03] before rebuild, heap=%u\n", ESP.getFreeHeap());
    rebuildKeyboardOverlay();
    Serial.printf("[KEYS 04] rebuild complete, heap=%u\n", ESP.getFreeHeap());

    lv_obj_add_flag(keyboardOverlay,LV_OBJ_FLAG_HIDDEN);
    Serial.println("[KEYS 05] lazy keyboard ready");
}

static void makeNav()
{
    static const char *names[]={"OSC","MIX","FILTER","MOD","PRESET","EFX"};
    constexpr int GAP=3, X0=4, W=76, H=32;
    for(int i=0;i<PAGE_COUNT;i++) {
        lv_obj_t *b=lv_btn_create(lv_scr_act());
        navButtons[i]=b;
        lv_obj_set_pos(b,X0+i*(W+GAP),235);
        lv_obj_set_size(b,W,H);
        lv_obj_set_style_radius(b,5,0);
        lv_obj_t *l=lv_label_create(b);
        lv_label_set_text(l,names[i]); lv_obj_center(l);
        lv_obj_add_event_cb(b,navCB,LV_EVENT_CLICKED,(void*)(intptr_t)i);
    }
}

static void destroyParameterOverlay()
{
    if(overlay) {
        lv_obj_del(overlay);
    }

    overlay=nullptr;
    overlayTitle=nullptr;
    overlayValue=nullptr;
    overlaySlider=nullptr;
}

static void makeOverlay()
{
    Serial.printf("[PARAM NEW 01] create fresh overlay heap=%u\n", ESP.getFreeHeap());

    overlay=lv_obj_create(lv_scr_act());
    lv_obj_set_pos(overlay,62,44);
    lv_obj_set_size(overlay,356,154);
    lv_obj_set_style_bg_color(overlay,lv_color_hex(0x121212),0);
    lv_obj_set_style_bg_opa(overlay,242,0);
    lv_obj_set_style_border_color(overlay,lv_color_hex(C_GOLD),0);
    lv_obj_set_style_border_width(overlay,2,0);
    lv_obj_set_style_radius(overlay,8,0);
    lv_obj_set_style_pad_all(overlay,0,0);
    lv_obj_clear_flag(overlay,LV_OBJ_FLAG_SCROLLABLE);

    overlayTitle=lv_label_create(overlay);
    lv_obj_set_style_text_color(overlayTitle,lv_color_hex(C_GOLD),0);
    lv_obj_set_style_text_font(overlayTitle,&lv_font_montserrat_14,0);
    lv_obj_align(overlayTitle,LV_ALIGN_TOP_MID,0,10);

    overlayValue=lv_label_create(overlay);
    lv_obj_set_style_text_color(overlayValue,lv_color_hex(0xFFFFFF),0);
    lv_obj_set_style_text_font(overlayValue,&lv_font_montserrat_16,0);
    lv_obj_align(overlayValue,LV_ALIGN_TOP_MID,0,40);

    overlaySlider=lv_slider_create(overlay);
    lv_obj_set_size(overlaySlider,276,14);
    lv_obj_align(overlaySlider,LV_ALIGN_CENTER,0,18);
    lv_slider_set_range(overlaySlider,0,1000);
    lv_obj_set_style_radius(overlaySlider,7,LV_PART_MAIN);
    lv_obj_set_style_radius(overlaySlider,7,LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(overlaySlider,6,LV_PART_KNOB);
    lv_obj_add_event_cb(overlaySlider,sliderChanged,LV_EVENT_VALUE_CHANGED,nullptr);

    // 0.6.8a CENTER-SNAP1: never create a second clickable object here.
    // The slider itself requests deferred close on LV_EVENT_RELEASED.
    // 0.7.1: inactivity timeout closes the overlay; RELEASE no longer closes it.

    lv_obj_add_flag(overlay,LV_OBJ_FLAG_HIDDEN);

    Serial.printf("[PARAM NEW 02] fresh overlay ready heap=%u\n", ESP.getFreeHeap());
}

}







static void serviceDeferredPageChange()
{
    if(!pageChangePending)
        return;

    if(Board::touchPressed())
        return;

    if((uint32_t)(millis() - Board::touchLastChangeMs()) < 35)
        return;

    // Let the originating tab button release/click cycle fully finish.
    if((uint32_t)(millis() - pageChangeRequestedAt) < 80)
        return;

    pageChangePending=false;

    // A page change also terminates any parameter session. Never leave a
    // hidden modal/slider tree attached to the screen.
    if(overlay) {
        parameterValuePending=false;
        pendingValueBinding=nullptr;
        lv_obj_add_flag(overlay,LV_OBJ_FLAG_HIDDEN);
        activeBinding=nullptr;
        lastChangedBinding=nullptr;
        pendingBinding=nullptr;
        parameterOpenPending=false;
        parameterClosePending=false;
    }

    pageChangeCount++;

    Serial.printf("[NAV D01] switch #%lu -> page=%d heap=%u\n",
                  (unsigned long)pageChangeCount,
                  (int)pendingPage,
                  ESP.getFreeHeap());
    Serial.flush();

    showPage(pendingPage);

    if(pageValuesDirty)
        refreshPageControls(activePage);

    Serial.printf("[NAV D02] page=%d active heap=%u\n",
                  (int)activePage,
                  ESP.getFreeHeap());
    Serial.flush();
}

static void serviceDeferredParameterClose()
{
    if(!parameterClosePending)
        return;

    if(Board::touchPressed())
        return;

    if((uint32_t)(millis() - Board::touchLastChangeMs()) < 35)
        return;

    parameterClosePending=false;
    parameterOpenPending=false;
    pendingBinding=nullptr;

    Binding *changed=lastChangedBinding;

    // Cancel any not-yet-applied slider event belonging to this modal before
    // deleting its LVGL object tree.
    parameterValuePending=false;
    pendingValueBinding=nullptr;

    if(overlay) lv_obj_add_flag(overlay,LV_OBJ_FLAG_HIDDEN);

    activeBinding=nullptr;
    lastChangedBinding=nullptr;

    if(changed)
        refreshBindingControls(changed);

    rtcParamPhase=6;

    Serial.printf("[PARAM] close+HIDE events=%lu heap=%u minHeap=%u touch=RELEASED\n",
                  (unsigned long)parameterEventCounter,
                  ESP.getFreeHeap(),
                  ESP.getMinFreeHeap());
    Serial.flush();
}

static void serviceDeferredParameterValue()
{
    if(!parameterValuePending)
        return;

    // Coalesce high-rate touch movement; 10 ms = up to 100 parameter updates/s.
    if((uint32_t)(millis() - parameterValueRequestedAt) < 10)
        return;

    Binding *b=pendingValueBinding;
    const float v=pendingValue;

    parameterValuePending=false;
    pendingValueBinding=nullptr;

    if(!b)
        return;

    // RTC phase 3 = applying value outside LVGL callback.
    rtcParamMagic=RTC_PARAM_MAGIC;
    rtcParamEvent=parameterEventCounter;
    rtcParamId=(int32_t)b->id;
    rtcParamValue=v;
    rtcParamPhase=3;

    Parameters::instance().set(
        b->id,
        v,
        ParameterSource::TOUCH
    );

    // Update only the overlay's small numeric field and only here, outside
    // lv_timer_handler/event dispatch.
    if(overlayValue && activeBinding==b) {
        char txt[32];
        formatValue(b,txt,sizeof(txt));
        lv_label_set_text(overlayValue,txt);

        // Reflect CENTER-SNAP on the knob as well. This runs in the deferred
        // UI service, never inside the LVGL VALUE_CHANGED callback.
        const bool centerSnapParam =
            b->id == ParameterID::MASTER_TUNE ||
            b->id == ParameterID::OSC2_TUNE ||
            b->id == ParameterID::OSC3_TUNE;
        if(centerSnapParam && v == 0.5f && overlaySlider) {
            externalSliderSyncGuard=true;
            lv_slider_set_value(overlaySlider,500,LV_ANIM_OFF);
            externalSliderSyncGuard=false;
        }
    }

    // RTC phase 4 = apply complete.
    rtcParamPhase=4;
}

static void serviceDeferredParameterOpen()
{
    if(!parameterOpenPending)
        return;

    if(Board::touchPressed())
        return;

    if((uint32_t)(millis() - Board::touchLastChangeMs()) < 35)
        return;

    Binding *b=pendingBinding;
    pendingBinding=nullptr;
    parameterOpenPending=false;

    if(!b) {
        Serial.println("[PARAM D01] ERROR binding unavailable");
        Serial.flush();
        return;
    }

    // v0.5.5f: discrete parameters never open a slider.
    // One released tap advances to the next legal value. This keeps the
    // proven deferred/touch-release architecture and avoids creating any
    // LVGL modal object for RANGE/WAVE/switch/priority controls.
    if(b->stepped && b->steps > 1) {
        const float current=val(b->id);
        int idx=(int)lroundf(current*(b->steps-1));
        idx=(idx+1)%b->steps;
        const float next=(float)idx/(float)(b->steps-1);

        Parameters::instance().set(b->id,next,ParameterSource::TOUCH);
        parameterEventCounter++;
        lastRevision=Parameters::instance().revision();
        refreshBindingControls(b);

        char txt[32];
        formatValue(b,txt,sizeof(txt));
        Serial.printf("[DISCRETE] %s -> %s step=%d/%d heap=%u\n",
                      b->name,txt,idx+1,b->steps,ESP.getFreeHeap());
        Serial.flush();
        return;
    }

    // 0.6.8a CENTER-SNAP1: reuse the one overlay created at boot.
    activeBinding=b;
    lastChangedBinding=b;

    if(!overlay || !overlayTitle || !overlayValue || !overlaySlider) {
        Serial.println("[066A ERROR] static overlay unavailable");
        Serial.flush();
        activeBinding=nullptr;
        lastChangedBinding=nullptr;
        return;
    }

    char valueTxt[32];
    formatValue(b,valueTxt,sizeof(valueTxt));
    externalSliderSyncGuard=true;
    lv_label_set_text(overlayTitle,b->name);
    lv_label_set_text(overlayValue,valueTxt);
    lv_slider_set_value(overlaySlider,(int32_t)lroundf(val(b->id)*1000.0f),LV_ANIM_OFF);
    externalSliderSyncGuard=false;
    lv_obj_clear_flag(overlay,LV_OBJ_FLAG_HIDDEN);
    overlayLastGuiActivityMs=millis();

    Serial.printf("[071 OVL] SHOW %s value=%s slider=%ld heap=%u controls=%d/%d\n",
                  b->name,valueTxt,(long)lv_slider_get_value(overlaySlider),
                  ESP.getFreeHeap(),controlCount,
                  (int)(sizeof(controls)/sizeof(controls[0])));
    Serial.flush();

}

static void serviceOverlayTimeout()
{
    if(!activeBinding || !overlay || lv_obj_has_flag(overlay,LV_OBJ_FLAG_HIDDEN))
        return;

    if(Board::touchPressed()) {
        overlayLastGuiActivityMs=millis();
        return;
    }

    const float v=Parameters::instance().target(ParameterID::OVERLAY_TIMEOUT);
    const uint32_t timeoutMs=(uint32_t)(1+(int)lroundf(v*4.0f))*1000U;
    if((uint32_t)(millis()-overlayLastGuiActivityMs) < timeoutMs)
        return;

    parameterClosePending=true;
    parameterCloseRequestedAt=millis();
}

static void serviceDeferredControlRefresh()
{
    // v0.5.5b4 intentionally performs no full-page refresh while a parameter
    // slider is active. Targeted labels are refreshed when the overlay closes.
}


static void clearKeyboardPointers()
{
    keyboardOverlay=nullptr;
    keyboardNoteLabel=nullptr;
    keyboardHoldLabel=nullptr;
    for(auto &k : keyboardKey) k=nullptr;
    keyboardTouchNote=-1;
    keyboardLatchedNote=-1;
    lastVisualNote=-999;
    lastVisualGate=false;
}

static void serviceDeferredKeyboardClose()
{
#if RTAL_UI_KEYBOARD
    if(!keyboardClosePending) return;
    if((uint32_t)(millis() - keyboardCloseRequestedAt) < 80) return;

    keyboardClosePending=false;
    Serial.printf("[KEYS CLOSE D01] begin heap=%u\n", ESP.getFreeHeap());
    Serial.flush();

    keyboardAllNotesOff();

    if(keyboardOverlay) {
        lv_obj_del(keyboardOverlay);
        clearKeyboardPointers();
    }

    Serial.printf("[KEYS CLOSE D02] deleted heap=%u\n", ESP.getFreeHeap());
    Serial.flush();
#endif
}

static void serviceDeferredKeyboardOpen()
{
#if RTAL_UI_KEYBOARD
    if(!keyboardOpenPending)
        return;

    // Give LVGL time to completely finish the KEYS button click/release cycle
    // before mutating the object tree.
    if((uint32_t)(millis() - keyboardOpenRequestedAt) < 120)
        return;

    keyboardOpenPending = false;

    Serial.printf("[KEYS D01] deferred open begin heap=%u\n", ESP.getFreeHeap());
    Serial.flush();

    if(!keyboardOverlay) {
        Serial.println("[KEYS D02] create keyboard outside event callback");
        Serial.flush();
        makeKeyboardOverlay();
    }

    if(!keyboardOverlay) {
        Serial.println("[KEYS D03] ERROR keyboard overlay unavailable");
        Serial.flush();
        return;
    }

    Serial.println("[KEYS D03] show overlay");
    Serial.flush();

    lv_obj_clear_flag(keyboardOverlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(keyboardOverlay);

    Serial.printf("[KEYS D04] shown heap=%u\n", ESP.getFreeHeap());
    Serial.flush();
#endif
}

void UI::begin()
{
    Serial.println("[UI 01] UI::begin enter");

    lv_obj_t *scr=lv_scr_act();
    lv_obj_set_style_bg_color(scr,lv_color_hex(C_BG),0);
    lv_obj_set_style_text_font(scr,&lv_font_montserrat_14,0);
    lv_obj_clear_flag(scr,LV_OBJ_FLAG_SCROLLABLE);
    Serial.println("[UI 02] screen style OK");

#if RTAL_UI_BASIC
    lv_obj_t *title=lv_label_create(scr);
    lv_label_set_text(title,"RTAL MINIAMUSED");
    lv_obj_set_style_text_color(title,lv_color_hex(C_GOLD),0);
    lv_obj_set_pos(title,8,5);

    lv_obj_t *status=lv_label_create(scr);
    presetHeaderLabel=status;
    lv_label_set_text(status,"000  INIT");
    lv_obj_set_style_text_color(status,lv_color_hex(C_DIM),0);
    lv_obj_align(status,LV_ALIGN_TOP_RIGHT,-8,5);
    Serial.println("[UI 03] header OK");

    pageHost=lv_obj_create(scr);
    lv_obj_set_pos(pageHost,0,25);
    lv_obj_set_size(pageHost,480,205);
    lv_obj_set_style_bg_opa(pageHost,LV_OPA_TRANSP,0);
    lv_obj_set_style_border_width(pageHost,0,0);
    lv_obj_set_style_pad_all(pageHost,0,0);
    lv_obj_clear_flag(pageHost,LV_OBJ_FLAG_SCROLLABLE);

    for(int i=0;i<PAGE_COUNT;i++) {
        pages[i]=lv_obj_create(pageHost);
        lv_obj_set_size(pages[i],480,205);
        lv_obj_set_pos(pages[i],0,0);
        lv_obj_set_style_bg_opa(pages[i],LV_OPA_TRANSP,0);
        lv_obj_set_style_border_width(pages[i],0,0);
        lv_obj_set_style_pad_all(pages[i],0,0);
        lv_obj_clear_flag(pages[i],LV_OBJ_FLAG_SCROLLABLE);
    }
    Serial.println("[UI 04] page containers OK");

    makeNav();
    Serial.println("[UI 05] navigation OK");
#endif

#if RTAL_UI_OSC
    Serial.println("[UI 10] make OSC page");
    makeOscPage();
    Serial.printf("[UI 11] OSC page OK heap=%u\n", ESP.getFreeHeap());
#endif

#if RTAL_UI_MIX
    Serial.println("[UI 20] make MIX page");
    makeMixPage();
    Serial.printf("[UI 21] MIX page OK heap=%u\n", ESP.getFreeHeap());
#endif

#if RTAL_UI_FILTER
    Serial.println("[UI 30] make FILTER page");
    makeFilterPage();
    Serial.printf("[UI 31] FILTER page OK heap=%u\n", ESP.getFreeHeap());
#endif

#if RTAL_UI_MOD
    Serial.println("[UI 40] make MOD page");
    makeModPage();
    Serial.printf("[UI 41] MOD page OK heap=%u\n", ESP.getFreeHeap());
#endif

#if RTAL_UI_PRESET
    Serial.println("[UI 50] make PRESET page");
    makePresetPage();
    Serial.printf("[UI 51] PRESET page OK heap=%u\n", ESP.getFreeHeap());
#endif

    Serial.println("[UI 55] make EFX page");
    makeEfxPage();
    Serial.printf("[UI 56] EFX page OK heap=%u\n", ESP.getFreeHeap());

#if RTAL_UI_BASIC
    destroyParameterOverlay();
    makeOverlay();
    Serial.printf("[UI 60] parameter modal STATIC create-once/show-hide heap=%u\n",
                  ESP.getFreeHeap());
#endif

#if RTAL_UI_KEYBOARD
    Serial.printf("[UI 70] touch keyboard deferred, heap=%u\n", ESP.getFreeHeap());
    Serial.println("[UI 71] lazy keyboard will be created on first KEYS press");
#endif

#if RTAL_UI_BASIC
    refreshControls();
    showPage(PAGE_OSC);
#endif

    lastRevision=Parameters::instance().revision();
    Serial.println("[UI 99] UI::begin complete");
}

void UI::service()
{
    Board::serviceLVGL();
    serviceDeferredPageChange();
    serviceDeferredParameterOpen();
    serviceDeferredParameterValue();
    serviceOverlayTimeout();
    serviceDeferredParameterClose();
    serviceDeferredControlRefresh();
    serviceParameterDirtyUI();
    serviceDeferredPresetBrowse();
    serviceDeferredMidiProgramChange();
    serviceDeferredPresetNameOpen();
    serviceDeferredPresetNameClose();
    serviceDeferredPresetManagement();
    serviceDeferredPresetAction();

#if RTAL_UI_KEYBOARD
    if(keyboardOverlay &&
       !lv_obj_has_flag(keyboardOverlay, LV_OBJ_FLAG_HIDDEN)) {
        updateKeyboardVisuals();
    }
#endif
}


uint32_t UI::parameterEventCount()
{
    return parameterEventCounter;
}

const char *UI::activeParameterName()
{
    if(activeBinding && activeBinding->name)
        return activeBinding->name;
    if(lastChangedBinding && lastChangedBinding->name)
        return lastChangedBinding->name;
    return "-";
}


void UI::printCrashForensics()
{
    if(rtcParamMagic != RTC_PARAM_MAGIC) {
        Serial.println("[FORENSICS] no retained parameter record");
        return;
    }

    Serial.printf(
        "[FORENSICS] lastParamId=%ld value=%.4f event=%lu phase=%lu\n",
        (long)rtcParamId,
        rtcParamValue,
        (unsigned long)rtcParamEvent,
        (unsigned long)rtcParamPhase
    );

    // Keep the record until a clean boot reaches READY; it is useful after
    // repeated reset loops.
}


int UI::activePageIndex()
{
    return (int)activePage;
}


void UI::requestProgramChange(uint8_t program)
{
    if(program>127)
        return;

    midiProgramChangeSlot=program;
    midiProgramChangePending=true;
    midiProgramChangeRequestedAt=millis();
}

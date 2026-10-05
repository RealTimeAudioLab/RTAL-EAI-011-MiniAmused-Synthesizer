#include <Arduino.h>
#include "esp_system.h"

#include "Board.h"
#include "Parameters.h"
#include "ParameterIDs.h"
#include "SynthEngine.h"
#include "ArpEngine.h"
#include "MidiEngine.h"
#include "PresetManager.h"
#include "RTAL_IO_PINS.h"
#include "UI.h"

// -----------------------------------------------------------------------------
// RTAL MiniAmused v1.2.3 FINAL
// Audio Engine Bring-Up + playable Touch Keyboard
//
// Proven d11 base retained:
//   SD/HSPI 8 MHz -> QSPI/NV3041A -> LVGL 8.4.0 -> GT911 -> full UI
//
// Added:
//   I2S / PCM5102A
//   SynthEngine
//   one AudioTask pinned to Core 1
//   bubbled Touch Keyboard -> SynthEngine noteOn/noteOff
//
// Still OFF:
//   MIDI
//   PresetManager load/save
// -----------------------------------------------------------------------------

static TaskHandle_t gAudioTask = nullptr;
static TaskHandle_t gMidiTask = nullptr;
static TaskHandle_t gArpTask = nullptr;

static void audioTaskEntry(void *arg)
{
    (void)arg;
    Serial.printf("[AUDIO TASK] start core=%d heap=%u\n",
                  xPortGetCoreID(),
                  ESP.getFreeHeap());
    Serial.flush();

    SynthEngine::instance().runAudioTask();
}


static void midiTaskEntry(void *arg)
{
    (void)arg;

    Serial.printf("[MIDI TASK] start core=%d heap=%u\n",
                  xPortGetCoreID(),
                  ESP.getFreeHeap());
    Serial.flush();

    for(;;) {
        MidiEngine::instance().poll();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void arpTaskEntry(void *arg)
{
    (void)arg;

    Serial.printf("[ARP TASK] start core=%d heap=%u\n",
                  xPortGetCoreID(),
                  ESP.getFreeHeap());
    Serial.flush();

    // Dedicated 1 ms scheduler: ARP timing is independent of LVGL/UI workload.
    for(;;) {
        ArpEngine::instance().service();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void setBringUpPatch()
{
    Parameters &p = Parameters::instance();

    // Full-engine default patch: all synthesis blocks are live.
    // Values are deliberately conservative for first stability testing.

    // Output
    p.set(ParameterID::MASTER_VOLUME, 0.62f, ParameterSource::INTERNAL);

    // Oscillator 1 - main tone
    p.set(ParameterID::OSC1_RANGE, 0.50f, ParameterSource::INTERNAL);
    p.set(ParameterID::OSC1_WAVE,  0.40f, ParameterSource::INTERNAL);
    p.set(ParameterID::OSC1_LEVEL, 0.62f, ParameterSource::INTERNAL);

    // Oscillator 2 - slight detune / body
    p.set(ParameterID::OSC2_RANGE, 0.50f, ParameterSource::INTERNAL);
    p.set(ParameterID::OSC2_WAVE,  0.40f, ParameterSource::INTERNAL);
    p.set(ParameterID::OSC2_TUNE,0.505f,ParameterSource::INTERNAL);
    p.set(ParameterID::OSC2_LEVEL, 0.34f, ParameterSource::INTERNAL);

    // Oscillator 3 - audible but lower, can also become modulation source
    p.set(ParameterID::OSC3_RANGE, 0.50f, ParameterSource::INTERNAL);
    p.set(ParameterID::OSC3_WAVE,  0.20f, ParameterSource::INTERNAL);
    p.set(ParameterID::OSC3_LEVEL, 0.22f, ParameterSource::INTERNAL);
    // Noise / feedback
    p.set(ParameterID::NOISE_LEVEL,    0.05f, ParameterSource::INTERNAL);
    p.set(ParameterID::FEEDBACK_LEVEL, 0.10f, ParameterSource::INTERNAL);

    // Filter
    p.set(ParameterID::FILTER_CUTOFF,    0.58f, ParameterSource::INTERNAL);
    p.set(ParameterID::FILTER_RESONANCE, 0.18f, ParameterSource::INTERNAL);
    p.set(ParameterID::FILTER_CONTOUR,   0.42f, ParameterSource::INTERNAL);
    p.set(ParameterID::FILTER_KEYTRACK,  0.45f, ParameterSource::INTERNAL);

    // Filter contour
    p.set(ParameterID::FILTER_ATTACK,  0.015f, ParameterSource::INTERNAL);
    p.set(ParameterID::FILTER_DECAY,   0.30f,  ParameterSource::INTERNAL);
    p.set(ParameterID::FILTER_SUSTAIN, 0.42f,  ParameterSource::INTERNAL);
    p.set(ParameterID::FILTER_RELEASE, 0.20f,  ParameterSource::INTERNAL);

    // Loudness contour
    p.set(ParameterID::AMP_ATTACK,  0.004f, ParameterSource::INTERNAL);
    p.set(ParameterID::AMP_DECAY,   0.22f,  ParameterSource::INTERNAL);
    p.set(ParameterID::AMP_SUSTAIN, 0.78f,  ParameterSource::INTERNAL);
    p.set(ParameterID::AMP_RELEASE, 0.18f,  ParameterSource::INTERNAL);

    // Performance
    p.set(ParameterID::GLIDE_TIME, 0.08f, ParameterSource::INTERNAL);
    // Modulation
    p.set(ParameterID::MOD_WHEEL,      0.0f,  ParameterSource::INTERNAL);
}

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("================================================");
    Serial.println("RTAL MiniAmused v1.2.3 FINAL");
    Serial.println("PARALLEL DIN MIDI + NATIVE USB MIDI / RTAL USB IDENTITY");
    Serial.println("DIN + USB MIDI IN / PresetManager ACTIVE");
    Serial.println("================================================");

    Serial.printf("[A01] LVGL %d.%d.%d\n",
                  lv_version_major(),
                  lv_version_minor(),
                  lv_version_patch());

    Serial.println("[A01b] LVGL 8.4 stable release baseline");
    Serial.println("[AUDIT] Functional baseline unchanged from 0.6.9a; see README_070_FEATURE_AUDIT1.md");

    Serial.printf("[A02] CPU=%u MHz PSRAM=%u heap=%u resetReason=%d\n",
                  ESP.getCpuFreqMHz(),
                  ESP.getPsramSize(),
                  ESP.getFreeHeap(),
                  (int)esp_reset_reason());

    UI::printCrashForensics();

    Serial.println("[A03] Parameters begin");
    Parameters::instance().begin();
    setBringUpPatch();

    Serial.println("[A04] Proven Board begin");
    if(!Board::begin()) {
        Serial.println("[A05] FATAL Board::begin");
        while(true) delay(1000);
    }
    Serial.println("[A05] SD + Display + LVGL + GT911 ready");

    Serial.println("[P01] PresetManager begin");
    PresetManager::instance().begin();
    Serial.printf("[P02] PresetManager %s\n",
                  PresetManager::instance().ready() ? "READY" : "DISABLED - SD unavailable");

    // v0.8.1c MIDI-CHANNEL-DISPLAY-FIX1:
    // Load persistent MIDI channel before UI creation so the MIDI CH control
    // is initialized from the actual NVS-backed receive channel after reset.
    Serial.println("[M01] MIDI begin");
    MidiEngine::instance().begin();
    Serial.printf("[M02] DIN MIDI ready RX=%d TX=%d 31250 baud OMNI\n",
                  MIDI_RX_PIN,
                  MIDI_TX_PIN);
    Serial.println("[M02b] Native USB MIDI device enabled (TinyUSB)");

    Serial.println("[A06] Full UI begin");
    UI::begin();

    for(int i=0; i<40; ++i) {
        UI::service();
        delay(5);
    }
    Serial.println("[A07] UI rendered");

    Serial.println("[A08] SynthEngine / I2S begin");
    if(!SynthEngine::instance().begin()) {
        Serial.println("[A09] FATAL SynthEngine::begin / I2S");
        while(true) {
            UI::service();
            delay(5);
        }
    }
    Serial.println("[A09] I2S ready");

    Serial.println("[M03] create MidiTask core 0");
    BaseType_t mr = xTaskCreatePinnedToCore(
        midiTaskEntry,
        "RTAL_MIDI",
        4096,
        nullptr,
        8,
        &gMidiTask,
        0
    );

    if(mr != pdPASS || gMidiTask == nullptr) {
        Serial.println("[M04] FATAL MidiTask creation failed");
        while(true) {
            UI::service();
            delay(5);
        }
    }

    Serial.println("[M04] MidiTask created");

    Serial.println("[ARP01] create ArpTask core 0");
    BaseType_t ar = xTaskCreatePinnedToCore(
        arpTaskEntry,
        "RTAL_ARP",
        4096,
        nullptr,
        9,
        &gArpTask,
        0
    );

    if(ar != pdPASS || gArpTask == nullptr) {
        Serial.println("[ARP02] FATAL ArpTask creation failed");
        while(true) {
            UI::service();
            delay(5);
        }
    }
    Serial.println("[ARP02] ArpTask created");

    Serial.println("[A10] create AudioTask core 1");
    BaseType_t r = xTaskCreatePinnedToCore(
        audioTaskEntry,
        "RTAL_Audio",
        8192,
        nullptr,
        20,
        &gAudioTask,
        1
    );

    if(r != pdPASS || gAudioTask == nullptr) {
        Serial.println("[0.6] FATAL AudioTask creation failed");
        while(true) {
            UI::service();
            delay(5);
        }
    }

    Serial.println("[0.6] AudioTask created");
    Serial.println("[0.6] READY - UI Completion");
}

void loop()
{
    uint8_t pc=0;
    if(MidiEngine::instance().takeProgramChange(pc))
        UI::requestProgramChange(pc);

    // ARP runs independently in RTAL_ARP on Core 0.
    UI::service();
    PresetManager::instance().service();
    // Give scheduler/idle tasks guaranteed runtime; audio is isolated on Core 1.
    vTaskDelay(pdMS_TO_TICKS(5));

    static uint32_t last = 0;
    static uint32_t lastBlocks = 0;

    if(millis() - last >= 5000) {
        last = millis();

        const uint32_t blocks = SynthEngine::instance().audioBlockCount();
        const uint32_t delta = blocks - lastBlocks;
        lastBlocks = blocks;

        Serial.printf(
                        "RTAL MiniAmused v1.2.3 FINAL alive %lu ms heap=%u minHeap=%u blocks=%lu (+%lu/s) "
            "err=%lu short=%lu gate=%d note=%d "
            "mRx=%lu usbRx=%lu on=%lu off=%lu cc=%lu pb=%lu map=%lu lastCC=%u:%u pc=%lu clk=%lu(d=%lu/u=%lu) run=%d "
            "clkActive=%d bpm=%.2f pEv=%lu param=%s page=%d\n",
            millis(),
            ESP.getFreeHeap(),
            ESP.getMinFreeHeap(),
            (unsigned long)blocks,
            (unsigned long)delta,
            (unsigned long)SynthEngine::instance().audioWriteErrors(),
            (unsigned long)SynthEngine::instance().audioShortWrites(),
            SynthEngine::instance().gateActive() ? 1 : 0,
            SynthEngine::instance().currentNote(),
            (unsigned long)MidiEngine::instance().rxCount(),
            (unsigned long)MidiEngine::instance().usbRxCount(),
            (unsigned long)MidiEngine::instance().noteOnCount(),
            (unsigned long)MidiEngine::instance().noteOffCount(),
            (unsigned long)MidiEngine::instance().ccCount(),
            (unsigned long)MidiEngine::instance().pitchBendCount(),
            (unsigned long)MidiEngine::instance().mappedCCCount(),
            (unsigned)MidiEngine::instance().lastCC(),
            (unsigned)MidiEngine::instance().lastCCValue(),
            (unsigned long)MidiEngine::instance().programChangeCount(),
            (unsigned long)MidiEngine::instance().clockCount(),
            (unsigned long)MidiEngine::instance().dinClockCount(),
            (unsigned long)MidiEngine::instance().usbClockCount(),
            MidiEngine::instance().transportRunning() ? 1 : 0,
            MidiEngine::instance().clockActive() ? 1 : 0,
            MidiEngine::instance().clockBpm(),
            (unsigned long)UI::parameterEventCount(),
            UI::activeParameterName(),
            UI::activePageIndex()
        );
    }
}

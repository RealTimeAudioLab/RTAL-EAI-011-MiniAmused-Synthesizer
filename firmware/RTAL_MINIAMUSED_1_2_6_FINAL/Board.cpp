#include "Board.h"
#include "PINS_JC4827W543.h"
#include "MiniAmusedSplash.h"

#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <SD.h>
#include <TAMC_GT911.h>
#include "esp_heap_caps.h"

static SPIClass sSdSPI(HSPI);

static Arduino_DataBus *sBus = nullptr;
static Arduino_GFX *sGfx = nullptr;
static Arduino_NV3041A *sPanel = nullptr;

static TAMC_GT911 sTouch(
    TOUCH_SDA, TOUCH_SCL,
    TOUCH_INT, TOUCH_RES,
    LCD_WIDTH, LCD_HEIGHT
);

// LVGL 8.4 display/input objects.  These driver structs must remain alive
// for the complete lifetime of LVGL, therefore they are static.
static lv_disp_draw_buf_t sDrawBufDesc;
static lv_disp_drv_t sDispDrv;
static lv_disp_t *sDisplay = nullptr;
static lv_indev_drv_t sIndevDrv;
static lv_indev_t *sIndev = nullptr;
static lv_color_t *sDrawBuf = nullptr;

static bool sSDReady = false;
static volatile bool sTouchPressed = false;
static volatile uint32_t sTouchLastChangeMs = 0;
static volatile uint32_t sGuiFrames = 0;
static volatile uint32_t sGuiFlushes = 0;
static volatile uint64_t sGuiPixels = 0;

// Release display path: QSPI direct RGB565 byte stream retained from tested FIX1.
static constexpr uint16_t LVGL_BUF_LINES = 60;
static constexpr size_t LVGL_BUF_PIXELS = LCD_WIDTH * LVGL_BUF_LINES;
static constexpr size_t LVGL_BUF_BYTES = LVGL_BUF_PIXELS * sizeof(lv_color_t);

static void flushCB(
    lv_disp_drv_t *disp,
    const lv_area_t *area,
    lv_color_t *color_p)
{
    const int32_t w = area->x2 - area->x1 + 1;
    const int32_t h = area->y2 - area->y1 + 1;
    const uint32_t pixels = (uint32_t)w * (uint32_t)h;
    sGuiFlushes++;
    sGuiPixels += pixels;

    sPanel->startWrite();
    sPanel->setAddrWindow(area->x1, area->y1, (uint16_t)w, (uint16_t)h);

    // Proven FIX1 path: byte-swap LVGL RGB565 in-place, transmit directly,
    // then restore the LVGL buffer. This avoids Arduino_GFX writePixels()'
    // per-pixel staging copy while preserving the tested colour order.
    uint16_t *px = reinterpret_cast<uint16_t *>(color_p);
    for(uint32_t i = 0; i < pixels; ++i) {
        const uint16_t v = px[i];
        px[i] = (uint16_t)((v << 8) | (v >> 8));
    }
    sBus->writeBytes(reinterpret_cast<uint8_t *>(px), pixels * 2U);
    for(uint32_t i = 0; i < pixels; ++i) {
        const uint16_t v = px[i];
        px[i] = (uint16_t)((v << 8) | (v >> 8));
    }

    sPanel->endWrite();
    if(lv_disp_flush_is_last(disp)) sGuiFrames++;
    lv_disp_flush_ready(disp);
}

static void touchCB(
    lv_indev_drv_t *indev_drv,
    lv_indev_data_t *data)
{
    (void)indev_drv;

    sTouch.read();

    const bool pressed =
        (sTouch.isTouched && sTouch.touches > 0);

    if (pressed) {
        data->point.x = constrain(
            sTouch.points[0].x,
            0,
            LCD_WIDTH - 1
        );

        data->point.y = constrain(
            sTouch.points[0].y,
            0,
            LCD_HEIGHT - 1
        );

        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }

    if(pressed != sTouchPressed) {
        sTouchPressed = pressed;
        sTouchLastChangeMs = millis();
    }
}

bool Board::begin()
{
    pinMode(GFX_BL, OUTPUT);
    digitalWrite(GFX_BL, LOW);

    // -------------------------------------------------------------------------
    // 1. SD first - exact proven MiniAmused pattern
    // -------------------------------------------------------------------------
    Serial.println("[B01] SD pins");
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    delay(10);

    Serial.println("[B02] HSPI.begin(..., SS=-1)");
    sSdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1);
    delay(20);

    Serial.println("[B03] SD.begin 8 MHz");
    sSDReady = SD.begin(SD_CS, sSdSPI, SD_SPEED);

    if (sSDReady) {
        const uint8_t cardType = SD.cardType();
        const uint64_t cardSizeMB = SD.cardSize() / (1024ULL * 1024ULL);
        Serial.printf("[B04] SD OK type=%u size=%llu MB\n",
                      (unsigned)cardType,
                      (unsigned long long)cardSizeMB);
    } else {
        Serial.println("[B04] SD mount FAILED - continuing");
    }

    // -------------------------------------------------------------------------
    // 2. QSPI / NV3041A after SD
    // -------------------------------------------------------------------------
    Serial.println("[B05] before QSPI constructor");
    sBus = new Arduino_ESP32QSPI(GFX_CS, GFX_SCK, GFX_D0, GFX_D1, GFX_D2, GFX_D3);
    if (!sBus) {
        Serial.println("[B06] FATAL: QSPI allocation failed");
        return false;
    }
    Serial.println("[B06] QSPI constructor returned");

    Serial.println("[B07] before NV3041A constructor");
    sPanel = new Arduino_NV3041A(sBus, GFX_NOT_DEFINED, 0, true);
    sGfx = sPanel;
    if (!sPanel) {
        Serial.println("[B08] FATAL: NV3041A allocation failed");
        return false;
    }
    Serial.println("[B08] NV3041A constructor returned / direct panel fast-flush enabled");

    Serial.println("[B09] gfx->begin");
    if (!sGfx->begin(GFX_SPEED)) {
        Serial.println("[B10] FATAL: gfx->begin failed");
        return false;
    }
    Serial.println("[B10] display OK");
    sGfx->fillScreen(RGB565_BLACK);

    // MiniAmused v1.2.5 startup splash. Draw before LVGL takes ownership of
    // the panel. The bitmap lives in flash (PROGMEM), so it consumes no
    // persistent audio heap/PSRAM.
    Serial.println("[B10a] MiniAmused splash 480x272");
    sGfx->draw16bitRGBBitmap(0, 0,
                             const_cast<uint16_t *>(MINIAMUSED_SPLASH_RGB565),
                             MINIAMUSED_SPLASH_W, MINIAMUSED_SPLASH_H);
    digitalWrite(GFX_BL, HIGH);
    delay(1500);

    // -------------------------------------------------------------------------
    // 3. LVGL 8.4.0
    // -------------------------------------------------------------------------
    Serial.println("[B11] lv_init - LVGL 8.4.0");
    lv_init();

    Serial.println("[B12] allocate 60-line RGB565 buffer");
    sDrawBuf = static_cast<lv_color_t *>(
        heap_caps_malloc(LVGL_BUF_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT)
    );
    if (!sDrawBuf) {
        sDrawBuf = static_cast<lv_color_t *>(malloc(LVGL_BUF_BYTES));
    }
    if (!sDrawBuf) {
        Serial.println("[B13] FATAL: LVGL buffer allocation failed");
        return false;
    }
    Serial.printf("[B13] LVGL buffer OK: %u bytes / %u pixels addr=%p internal=%s dma=%s psram=%s\n",
                  (unsigned)LVGL_BUF_BYTES, (unsigned)LVGL_BUF_PIXELS, sDrawBuf,
                  esp_ptr_internal(sDrawBuf) ? "YES" : "NO",
                  esp_ptr_dma_capable(sDrawBuf) ? "YES" : "NO",
                  esp_ptr_external_ram(sDrawBuf) ? "YES" : "NO");
    Serial.printf("[B13b] LCD=%ux%u RGB565 QSPI=%lu Hz bufferLines=%u\n",
                  (unsigned)LCD_WIDTH, (unsigned)LCD_HEIGHT,
                  (unsigned long)GFX_SPEED, (unsigned)LVGL_BUF_LINES);

    lv_disp_draw_buf_init(&sDrawBufDesc, sDrawBuf, nullptr, LVGL_BUF_PIXELS);

    Serial.println("[B14] lv_disp_drv_register");
    lv_disp_drv_init(&sDispDrv);
    sDispDrv.hor_res = LCD_WIDTH;
    sDispDrv.ver_res = LCD_HEIGHT;
    sDispDrv.flush_cb = flushCB;
    sDispDrv.draw_buf = &sDrawBufDesc;
    sDisplay = lv_disp_drv_register(&sDispDrv);
    if (!sDisplay) {
        Serial.println("[B15] FATAL: lv_disp_drv_register failed");
        return false;
    }
    Serial.println("[B15] LVGL 8.4 display configured");

    // -------------------------------------------------------------------------
    // 4. GT911 + LVGL 8.4 input driver
    // -------------------------------------------------------------------------
    Serial.println("[B16] GT911 begin");
    sTouch.begin(GT911_ADDR1);
    sTouch.setRotation(ROTATION_INVERTED);

    Serial.println("[B17] LVGL 8.4 input register");
    lv_indev_drv_init(&sIndevDrv);
    sIndevDrv.type = LV_INDEV_TYPE_POINTER;
    sIndevDrv.read_cb = touchCB;
    sIndev = lv_indev_drv_register(&sIndevDrv);
    if (!sIndev) {
        Serial.println("[B18] FATAL: lv_indev_drv_register failed");
        return false;
    }
    Serial.println("[B18] GT911 ready");

    digitalWrite(GFX_BL, HIGH);
    Serial.println("[B19] backlight ON");
    return true;
}


void Board::serviceLVGL()
{
    // LVGL 8.x has no lv_tick_set_cb() used by our former LVGL 9 port.
    // Feed the LVGL tick from Arduino millis() on every service call, even if
    // the expensive timer handler itself is throttled to 2 ms.
    static uint32_t lastTickMs = 0;
    static uint32_t lastHandlerMs = 0;
    const uint32_t now = millis();

    if(lastTickMs == 0) lastTickMs = now;
    const uint32_t elapsed = (uint32_t)(now - lastTickMs);
    if(elapsed) {
        lv_tick_inc(elapsed);
        lastTickMs = now;
    }

    if((uint32_t)(now - lastHandlerMs) < 2)
        return;
    lastHandlerMs = now;

    lv_timer_handler();
}

bool Board::sdReady() { return sSDReady; }
bool Board::touchPressed() { return sTouchPressed; }
uint32_t Board::touchLastChangeMs() { return sTouchLastChangeMs; }

uint32_t Board::guiFrameCount() { return sGuiFrames; }
uint32_t Board::guiFlushCount() { return sGuiFlushes; }
uint64_t Board::guiPixelCount() { return sGuiPixels; }

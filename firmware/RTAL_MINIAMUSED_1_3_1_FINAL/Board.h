#pragma once
#include <Arduino.h>
#include <lvgl.h>

namespace Board {
    bool begin();
    void serviceLVGL();
    bool sdReady();
    bool touchPressed();
    uint32_t touchLastChangeMs();
    uint32_t guiFrameCount();
    uint32_t guiFlushCount();
    uint64_t guiPixelCount();
}

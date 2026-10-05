#pragma once
#include <Arduino.h>
#include <lvgl.h>

namespace Board {
    bool begin();
    void serviceLVGL();
    bool sdReady();
    bool touchPressed();
    uint32_t touchLastChangeMs();
}

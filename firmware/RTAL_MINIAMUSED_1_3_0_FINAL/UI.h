#pragma once
#include <stdint.h>

namespace UI {
    void begin();
    void service();
    uint32_t parameterEventCount();
    const char *activeParameterName();
    void printCrashForensics();
    int activePageIndex();
    void requestProgramChange(uint8_t program);
}

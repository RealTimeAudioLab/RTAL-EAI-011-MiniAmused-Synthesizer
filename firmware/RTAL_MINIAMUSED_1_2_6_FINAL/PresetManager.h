#pragma once
#include <Arduino.h>

class PresetManager {
public:
    static PresetManager& instance();

    void begin();
    void service();

    bool ready() const { return _ready; }
    bool busy() const { return _busy; }

    bool save(uint16_t slot, const char *name);
    bool load(uint16_t slot);

    bool exists(uint16_t slot) const;
    bool readName(uint16_t slot, char *dst, size_t n) const;
    bool remove(uint16_t slot);

private:
    PresetManager() = default;

    volatile bool _busy = false;
    bool _ready = false;
};

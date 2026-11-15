#pragma once

#include <atomic>
#include "ParameterIDs.h"

struct ParameterSlot {
    std::atomic<float> target {0.0f};

    // `current` is intentionally not atomic:
    // only AudioTask/Core1 owns and writes the smoothed DSP value.
    float current = 0.0f;
};

class Parameters {
public:
    static Parameters& instance();

    void begin();
    void applyInitPatch();

    void set(ParameterID id, float normalized, ParameterSource source);
    float target(ParameterID id) const;
    float smooth(ParameterID id, float coeff);

    uint32_t revision() const
    {
        return _revision.load(std::memory_order_relaxed);
    }

    // Two 64-bit dirty words allow append-only growth beyond 64 parameters.
    uint64_t takeDirtyMask(unsigned word)
    {
        return word < 2 ? _dirtyMask[word].exchange(0, std::memory_order_acq_rel) : 0;
    }

private:
    ParameterSlot _p[(size_t)ParameterID::COUNT];
    std::atomic<uint32_t> _revision {0};
    std::atomic<uint64_t> _dirtyMask[2] {{0},{0}};

    Parameters() = default;
};

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

    // Returns and clears parameter-change bits accumulated since the previous
    // UI service pass. ParameterID::COUNT is < 64.
    uint64_t takeDirtyMask()
    {
        return _dirtyMask.exchange(0, std::memory_order_acq_rel);
    }

private:
    ParameterSlot _p[(size_t)ParameterID::COUNT];
    std::atomic<uint32_t> _revision {0};
    std::atomic<uint64_t> _dirtyMask {0};

    Parameters() = default;
};

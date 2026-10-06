#include "Parameters.h"
#include <math.h>

Parameters& Parameters::instance()
{
    static Parameters p;
    return p;
}

void Parameters::begin()
{
    for (size_t i = 0; i < (size_t)ParameterID::COUNT; ++i) {
        _p[i].target.store(0.0f, std::memory_order_relaxed);
        _p[i].current = 0.0f;
    }

    set(ParameterID::MASTER_VOLUME,   0.75f, ParameterSource::INTERNAL);
    set(ParameterID::OSC1_RANGE,      0.50f, ParameterSource::INTERNAL);
    set(ParameterID::OSC1_WAVE,       0.40f, ParameterSource::INTERNAL);
    set(ParameterID::OSC1_LEVEL,      0.72f, ParameterSource::INTERNAL);
    set(ParameterID::OSC2_RANGE,      0.50f, ParameterSource::INTERNAL);
    set(ParameterID::OSC2_WAVE,       0.40f, ParameterSource::INTERNAL);
    set(ParameterID::OSC2_TUNE,       0.50f, ParameterSource::INTERNAL);
    set(ParameterID::OSC2_LEVEL,      0.60f, ParameterSource::INTERNAL);
    set(ParameterID::OSC3_RANGE,      0.50f, ParameterSource::INTERNAL);
    set(ParameterID::OSC3_WAVE,       0.40f, ParameterSource::INTERNAL);
    set(ParameterID::OSC3_TUNE,       0.50f, ParameterSource::INTERNAL);
    set(ParameterID::OSC3_LEVEL,      0.00f, ParameterSource::INTERNAL);
    set(ParameterID::OSC3_KEYBOARD_CONTROL, 1.0f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_CUTOFF,   0.68f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_RESONANCE,0.12f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_CONTOUR,  0.35f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_KEYTRACK, 0.33f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_ATTACK,   0.01f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_DECAY,    0.22f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_SUSTAIN,  0.20f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_RELEASE,  0.22f, ParameterSource::INTERNAL);
    set(ParameterID::AMP_ATTACK,      0.005f, ParameterSource::INTERNAL);
    set(ParameterID::AMP_DECAY,       0.20f, ParameterSource::INTERNAL);
    set(ParameterID::AMP_SUSTAIN,     0.75f, ParameterSource::INTERNAL);
    set(ParameterID::AMP_RELEASE,     0.20f, ParameterSource::INTERNAL);
    set(ParameterID::NOISE_LEVEL,      0.00f, ParameterSource::INTERNAL);
    set(ParameterID::NOISE_COLOR,      0.00f, ParameterSource::INTERNAL);
    set(ParameterID::MOD_MIX,          0.50f, ParameterSource::INTERNAL);
    set(ParameterID::MOD_WHEEL,        0.00f, ParameterSource::INTERNAL);
    set(ParameterID::OSC_MOD_ENABLE,   0.00f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_MOD_ENABLE,0.00f, ParameterSource::INTERNAL);
    set(ParameterID::DECAY_ENABLE,     1.00f, ParameterSource::INTERNAL);
    set(ParameterID::FEEDBACK_LEVEL,   0.00f, ParameterSource::INTERNAL);
    set(ParameterID::NOTE_PRIORITY,    0.50f, ParameterSource::INTERNAL);
    set(ParameterID::TRIGGER_MODE,     0.00f, ParameterSource::INTERNAL);
    set(ParameterID::MASTER_TUNE,      0.50f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_ENABLE,      0.00f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_TIME,        0.42f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_FEEDBACK,    0.35f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_MIX,         0.22f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_FILTER,      0.72f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_WIDTH,       0.66f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_SYNC,        0.00f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_DIV,         0.00f, ParameterSource::INTERNAL);
    set(ParameterID::OVERLAY_TIMEOUT,   0.25f, ParameterSource::INTERNAL); // 2 s default in 1..5 s range
    set(ParameterID::DELAY_PING_MODE,    0.00f, ParameterSource::INTERNAL); // LEGACY = exact 0.7.2c
    set(ParameterID::DELAY_PING_AMOUNT,  1.00f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_STEREO_WIDTH, 0.50f, ParameterSource::INTERNAL); // 100% neutral in 0..200% range
    set(ParameterID::ARP_ENABLE,          0.00f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_MODE,            0.00f, ParameterSource::INTERNAL); // UP
    set(ParameterID::ARP_RATE,            0.50f, ParameterSource::INTERNAL); // 1/16
    set(ParameterID::ARP_GATE,            0.70f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_OCTAVES,         0.00f, ParameterSource::INTERNAL); // 1 octave
    set(ParameterID::ARP_MARKOV,          1.00f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_MUTATION,        0.15f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_RANGE,           0.33f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_HOLD,            0.00f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_DENSITY,         1.00f, ParameterSource::INTERNAL); // every step
    set(ParameterID::ARP_SWING,           0.00f, ParameterSource::INTERNAL); // straight
    set(ParameterID::ARP_RATCHET,         0.00f, ParameterSource::INTERNAL); // OFF
    set(ParameterID::ARP_ACCENT,          0.00f, ParameterSource::INTERNAL); // original velocity
    set(ParameterID::ARP_MIDI_SYNC,       0.00f, ParameterSource::INTERNAL); // internal clock default
    set(ParameterID::ARP_TEMPO,           (120.0f-40.0f)/260.0f, ParameterSource::INTERNAL); // 120 BPM
    set(ParameterID::ARP_ACCENT_MODE,     0.00f, ParameterSource::INTERNAL); // 4 STEP
    set(ParameterID::ARP_REPEAT,          0.00f, ParameterSource::INTERNAL); // no forced repeats

    // Start current values at targets so boot has no long ramps.
    for (size_t i = 0; i < (size_t)ParameterID::COUNT; ++i)
        _p[i].current = _p[i].target.load(std::memory_order_relaxed);
}


void Parameters::applyInitPatch()
{
    set(ParameterID::MASTER_VOLUME,   0.75f, ParameterSource::INTERNAL);
    set(ParameterID::OSC1_RANGE,      0.50f, ParameterSource::INTERNAL);
    set(ParameterID::OSC1_WAVE,       0.40f, ParameterSource::INTERNAL);
    set(ParameterID::OSC1_LEVEL,      0.72f, ParameterSource::INTERNAL);
    set(ParameterID::OSC2_RANGE,      0.50f, ParameterSource::INTERNAL);
    set(ParameterID::OSC2_WAVE,       0.40f, ParameterSource::INTERNAL);
    set(ParameterID::OSC2_TUNE,       0.50f, ParameterSource::INTERNAL);
    set(ParameterID::OSC2_LEVEL,      0.60f, ParameterSource::INTERNAL);
    set(ParameterID::OSC3_RANGE,      0.50f, ParameterSource::INTERNAL);
    set(ParameterID::OSC3_WAVE,       0.40f, ParameterSource::INTERNAL);
    set(ParameterID::OSC3_TUNE,       0.50f, ParameterSource::INTERNAL);
    set(ParameterID::OSC3_LEVEL,      0.00f, ParameterSource::INTERNAL);
    set(ParameterID::OSC3_KEYBOARD_CONTROL, 1.0f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_CUTOFF,   0.68f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_RESONANCE,0.12f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_CONTOUR,  0.35f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_KEYTRACK, 0.33f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_ATTACK,   0.01f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_DECAY,    0.22f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_SUSTAIN,  0.20f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_RELEASE,  0.22f, ParameterSource::INTERNAL);
    set(ParameterID::AMP_ATTACK,      0.005f, ParameterSource::INTERNAL);
    set(ParameterID::AMP_DECAY,       0.20f, ParameterSource::INTERNAL);
    set(ParameterID::AMP_SUSTAIN,     0.75f, ParameterSource::INTERNAL);
    set(ParameterID::AMP_RELEASE,     0.20f, ParameterSource::INTERNAL);
    set(ParameterID::NOISE_LEVEL,     0.00f, ParameterSource::INTERNAL);
    set(ParameterID::NOISE_COLOR,     0.00f, ParameterSource::INTERNAL);
    set(ParameterID::MOD_MIX,         0.50f, ParameterSource::INTERNAL);
    set(ParameterID::MOD_WHEEL,       0.00f, ParameterSource::INTERNAL);
    set(ParameterID::OSC_MOD_ENABLE,  0.00f, ParameterSource::INTERNAL);
    set(ParameterID::FILTER_MOD_ENABLE,0.00f,ParameterSource::INTERNAL);
    set(ParameterID::DECAY_ENABLE,    1.00f, ParameterSource::INTERNAL);
    set(ParameterID::FEEDBACK_LEVEL,  0.00f, ParameterSource::INTERNAL);
    set(ParameterID::NOTE_PRIORITY,   0.50f, ParameterSource::INTERNAL);
    set(ParameterID::TRIGGER_MODE,    0.00f, ParameterSource::INTERNAL);
    set(ParameterID::MASTER_TUNE,     0.50f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_ENABLE,     0.00f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_TIME,       0.42f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_FEEDBACK,   0.35f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_MIX,        0.22f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_FILTER,     0.72f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_WIDTH,      0.66f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_SYNC,       0.00f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_DIV,        0.00f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_PING_MODE,   0.00f, ParameterSource::INTERNAL); // LEGACY
    set(ParameterID::DELAY_PING_AMOUNT, 1.00f, ParameterSource::INTERNAL);
    set(ParameterID::DELAY_STEREO_WIDTH,0.50f, ParameterSource::INTERNAL); // 100% neutral
    set(ParameterID::ARP_ENABLE,          0.00f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_MODE,            0.00f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_RATE,            0.50f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_GATE,            0.70f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_OCTAVES,         0.00f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_MARKOV,          1.00f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_MUTATION,        0.15f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_RANGE,           0.33f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_HOLD,            0.00f, ParameterSource::INTERNAL);
    set(ParameterID::ARP_DENSITY,         1.00f, ParameterSource::INTERNAL); // every step
    set(ParameterID::ARP_SWING,           0.00f, ParameterSource::INTERNAL); // straight
    set(ParameterID::ARP_RATCHET,         0.00f, ParameterSource::INTERNAL); // OFF
    set(ParameterID::ARP_ACCENT,          0.00f, ParameterSource::INTERNAL); // original velocity
    set(ParameterID::ARP_MIDI_SYNC,       0.00f, ParameterSource::INTERNAL); // internal clock default
    set(ParameterID::ARP_TEMPO,           (120.0f-40.0f)/260.0f, ParameterSource::INTERNAL); // 120 BPM
    set(ParameterID::ARP_ACCENT_MODE,     0.00f, ParameterSource::INTERNAL); // 4 STEP
    set(ParameterID::ARP_REPEAT,          0.00f, ParameterSource::INTERNAL); // no forced repeats
}

void Parameters::set(ParameterID id, float normalized, ParameterSource source)
{
    (void)source;
    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;

    const size_t idx=(size_t)id;
    _p[idx].target.store(normalized, std::memory_order_relaxed);

    if(idx < 128)
        _dirtyMask[idx >> 6].fetch_or((uint64_t)1ULL << (idx & 63), std::memory_order_release);

    _revision.fetch_add(1, std::memory_order_relaxed);
}

float Parameters::target(ParameterID id) const
{
    return _p[(size_t)id].target.load(std::memory_order_relaxed);
}

float Parameters::smooth(ParameterID id, float coeff)
{
    ParameterSlot &p = _p[(size_t)id];
    const float t = p.target.load(std::memory_order_relaxed);
    p.current += (t - p.current) * coeff;
    return p.current;
}

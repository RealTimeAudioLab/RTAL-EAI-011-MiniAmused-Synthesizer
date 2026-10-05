#pragma once
#include <Arduino.h>

enum class ParameterID : uint16_t {
    MASTER_VOLUME = 0,
    GLIDE_TIME,
    GLIDE_ENABLE,
    MOD_WHEEL,

    OSC1_RANGE,
    OSC1_WAVE,
    OSC1_LEVEL,

    OSC2_RANGE,
    OSC2_WAVE,
    OSC2_TUNE,
    OSC2_LEVEL,

    OSC3_RANGE,
    OSC3_WAVE,
    OSC3_TUNE,
    OSC3_LEVEL,
    OSC3_KEYBOARD_CONTROL,

    NOISE_LEVEL,
    MIXER_DRIVE,

    FILTER_CUTOFF,
    FILTER_RESONANCE,
    FILTER_CONTOUR,
    FILTER_KEYTRACK,
    FILTER_MOD_ENABLE,

    FILTER_ATTACK,
    FILTER_DECAY,
    FILTER_SUSTAIN,
    FILTER_RELEASE,

    AMP_ATTACK,
    AMP_DECAY,
    AMP_SUSTAIN,
    AMP_RELEASE,

    OSC_MOD_ENABLE,
    DECAY_ENABLE,

    NOISE_COLOR,
    MOD_MIX,
    FEEDBACK_LEVEL,
    NOTE_PRIORITY,
    TRIGGER_MODE,

    // v0.5.7: global oscillator tuning, appended to preserve all existing IDs.
    MASTER_TUNE,

    // v0.6.2 DELAY1: append-only to preserve all existing parameter IDs.
    DELAY_ENABLE,
    DELAY_TIME,
    DELAY_FEEDBACK,
    DELAY_MIX,

    // v0.6.5a DELAY-FILTER1: append-only.
    DELAY_FILTER,

    // v0.6.5b DELAY-WIDTH1: append-only.
    DELAY_WIDTH,

    // v0.6.5c DELAY-SYNC1: append-only.
    DELAY_SYNC,

    // v0.6.9 DELAY-SYNC-DIV1: append-only.
    DELAY_DIV,

    // v0.7.1 AUDIT-FIX1: global parameter-overlay inactivity timeout.
    OVERLAY_TIMEOUT,

    // v0.7.3 PINGPONG1: append-only. Legacy preserves the exact 0.7.2c delay.
    DELAY_PING_MODE,
    DELAY_PING_AMOUNT,

    // v0.8.0 MIDI-FINAL1: true post-delay Mid/Side width, 0..200%.
    DELAY_STEREO_WIDTH,

    COUNT
};

enum class ParameterSource : uint8_t {
    INTERNAL,
    TOUCH,
    MIDI,
    PRESET
};

#pragma once
#include "ParameterIDs.h"

struct MidiCCEntry { uint8_t cc; ParameterID id; };

// v1.1.1 MIDI MAP LEARN1
// STANDARD = complete MiniAmused v1.0 CC map. CC32 is a normal controller.
// MOPHO    = v1.1 banked map. CC32 is reserved as 0=MAIN,1=EFX,2=reserved.
static constexpr MidiCCEntry kMidiCCStandard[] = {
 {1,ParameterID::MOD_WHEEL},{30,ParameterID::GLIDE_TIME},{7,ParameterID::MASTER_VOLUME},
 {22,ParameterID::OSC1_WAVE},{20,ParameterID::OSC1_RANGE},{23,ParameterID::OSC1_LEVEL},
 {26,ParameterID::OSC2_WAVE},{24,ParameterID::OSC2_RANGE},{31,ParameterID::OSC2_TUNE},{27,ParameterID::OSC2_LEVEL},
 {86,ParameterID::OSC3_WAVE},{89,ParameterID::OSC3_RANGE},{90,ParameterID::OSC3_TUNE},{77,ParameterID::OSC3_LEVEL},
 {29,ParameterID::NOISE_LEVEL},{28,ParameterID::MIXER_DRIVE},{103,ParameterID::FILTER_RESONANCE},{102,ParameterID::FILTER_CUTOFF},
 {105,ParameterID::FILTER_CONTOUR},{109,ParameterID::FILTER_ATTACK},{110,ParameterID::FILTER_DECAY},{111,ParameterID::FILTER_SUSTAIN},
 {112,ParameterID::FILTER_RELEASE},{118,ParameterID::AMP_ATTACK},{119,ParameterID::AMP_DECAY},{75,ParameterID::AMP_SUSTAIN},
 {76,ParameterID::AMP_RELEASE},{87,ParameterID::GLIDE_ENABLE},{116,ParameterID::MASTER_TUNE},{108,ParameterID::OSC_MOD_ENABLE},
 {107,ParameterID::FILTER_MOD_ENABLE},{78,ParameterID::OSC3_KEYBOARD_CONTROL},{53,ParameterID::MOD_MIX},{115,ParameterID::DECAY_ENABLE},
 {106,ParameterID::FILTER_KEYTRACK},{52,ParameterID::FEEDBACK_LEVEL},{104,ParameterID::NOISE_COLOR},{14,ParameterID::NOTE_PRIORITY},
 {117,ParameterID::TRIGGER_MODE},
 {15,ParameterID::DELAY_ENABLE},{16,ParameterID::DELAY_TIME},{17,ParameterID::DELAY_FEEDBACK},{18,ParameterID::DELAY_MIX},
 {19,ParameterID::DELAY_FILTER},{21,ParameterID::DELAY_WIDTH},{25,ParameterID::DELAY_PING_MODE},{32,ParameterID::DELAY_PING_AMOUNT},
 {33,ParameterID::DELAY_STEREO_WIDTH},{34,ParameterID::DELAY_SYNC},{35,ParameterID::DELAY_DIV}
};

static constexpr MidiCCEntry kMidiCCBank0[] = {
 {1,ParameterID::MOD_WHEEL},{30,ParameterID::GLIDE_TIME},{7,ParameterID::MASTER_VOLUME},
 {22,ParameterID::OSC1_WAVE},{20,ParameterID::OSC1_RANGE},{23,ParameterID::OSC1_LEVEL},
 {26,ParameterID::OSC2_WAVE},{24,ParameterID::OSC2_RANGE},{31,ParameterID::OSC2_TUNE},{27,ParameterID::OSC2_LEVEL},
 {86,ParameterID::OSC3_WAVE},{89,ParameterID::OSC3_RANGE},{90,ParameterID::OSC3_TUNE},{77,ParameterID::OSC3_LEVEL},
 {29,ParameterID::NOISE_LEVEL},{28,ParameterID::MIXER_DRIVE},{103,ParameterID::FILTER_RESONANCE},{102,ParameterID::FILTER_CUTOFF},
 {105,ParameterID::FILTER_CONTOUR},{109,ParameterID::FILTER_ATTACK},{110,ParameterID::FILTER_DECAY},{111,ParameterID::FILTER_SUSTAIN},
 {112,ParameterID::FILTER_RELEASE},{118,ParameterID::AMP_ATTACK},{119,ParameterID::AMP_DECAY},{75,ParameterID::AMP_SUSTAIN},
 {76,ParameterID::AMP_RELEASE},{87,ParameterID::GLIDE_ENABLE},{116,ParameterID::MASTER_TUNE},{108,ParameterID::OSC_MOD_ENABLE},
 {107,ParameterID::FILTER_MOD_ENABLE},{78,ParameterID::OSC3_KEYBOARD_CONTROL},{53,ParameterID::MOD_MIX},{115,ParameterID::DECAY_ENABLE},
 {106,ParameterID::FILTER_KEYTRACK},{52,ParameterID::FEEDBACK_LEVEL},{104,ParameterID::NOISE_COLOR},{14,ParameterID::NOTE_PRIORITY},
 {117,ParameterID::TRIGGER_MODE}
};
static constexpr MidiCCEntry kMidiCCBank1[] = {
 {102,ParameterID::DELAY_ENABLE},{103,ParameterID::DELAY_TIME},{104,ParameterID::DELAY_FEEDBACK},
 {105,ParameterID::DELAY_MIX},{106,ParameterID::DELAY_FILTER},{107,ParameterID::DELAY_WIDTH},
 {108,ParameterID::DELAY_PING_MODE},{109,ParameterID::DELAY_PING_AMOUNT},{110,ParameterID::DELAY_STEREO_WIDTH},
 {111,ParameterID::DELAY_SYNC},{112,ParameterID::DELAY_DIV}
};

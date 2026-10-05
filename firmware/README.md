# RTAL MiniAmused v1.0.0 FINAL - Release Notes

## First stable public release

RTAL MiniAmused v1.0.0 FINAL is the first frozen public release of the monophonic ESP32-S3 MiniAmused synthesizer.

### Main features

- Three-oscillator monophonic subtractive synthesizer
- Ladder-style low-pass filter with 2x oversampling
- Separate Filter and Loudness ADSR envelopes
- Oscillator/noise modulation system
- Glide, note priority and single/multi trigger modes
- DIN MIDI and native USB MIDI
- Persistent OMNI / CH1-CH16 receive-channel selection
- Complete MIDI CC control for the v1.0 parameter set
- Program Change 0-127 -> Preset 000-127
- 128 SD-card preset slots
- Stereo delay with filtered feedback
- Stable free-time and MIDI-clock-synchronized delay transitions
- Eight synchronized delay divisions
- Five ping-pong modes
- Original delay Width plus independent 0-200% Mid/Side Stereo Width
- LVGL 8.4.0 touch UI with six functional pages
- Configurable 1-5 second parameter-overlay timeout, default 2 seconds

### UI pages

Documentation order:

**OSC -> MIX -> FILTER -> MOD -> EFX -> PRESET**

### Stability baseline

v1.0.0 FINAL is based on the validated v0.9.0 RELEASE-CANDIDATE1. The finalization step did not change Synth, Filter, Envelope, MIDI, Preset or Delay DSP behavior.

### MIDI notes

- DIN and native USB MIDI can be used in parallel.
- MIDI realtime messages are handled independently of the selected receive channel.
- MIDI Clock F8 remains usable for delay synchronization even when transport is stopped.
- CC120 and CC123 perform All Notes Off.
- The MIDI receive channel is device configuration and is not stored inside sound presets.

### Presets

- 128 slots: 000-127
- Load / Save / Save As / Init / Delete
- Direct MIDI Program Change mapping
- Transactional save workflow using temporary and backup files

### Important version note

This release documents the **v1.0 MIDI CC map**. The CC32 bank-select architecture planned for MiniAmused v1.1 is not part of v1.0. In v1.0, CC32 controls Delay Ping Amount.

### Release assets

- `RTAL_MINIAMUSED_1.0.0_FINAL.zip` - Arduino source release
- `README.md` - GitHub project documentation
- `RTAL_MiniAmused_v1.0_Handbuch.pdf` - detailed German user manual
- `RTAL_MiniAmused_v1.0_Handbuch.docx` - editable manual

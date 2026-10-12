# RTAL MiniAmused v1.3.0 FINAL — Release Notes

**Project:** RealTimeAudioLab · RTAL-EAI-011  
**Version:** `v1.3.0` · **Status:** FINAL · **Date:** 12 October 2026

## Highlights

This release adds a complete **dual-variation 16-step sequencer** to the established MiniAmused virtual-analog synthesizer. The previously tested **v1.2.7** three-oscillator / nonlinear ladder-filter audio path is preserved.

### New: 16-step sequencer

- Two independent editable patterns: **A and B**, 1–16 steps each, with **A / B / A+B / AABB** playback.
- Per-step pitch, velocity, gate, and **REST / TIE / SLIDE / ACCENT**.
- Large **touch Step Editor**, step matrix and separate PITCH / STEP / GLOBAL controls.
- **STEP REC** via DIN/USB MIDI keyboard; **LIVE REC** with grid quantization.
- Tempo (internal 40–300 BPM), 1/4–1/32 rates, swing, root/keyboard transposition, pattern copy A→B.
- MIDI Clock and transport: **F8 / FA / FB / FC**.
- **Preset V3** stores the complete synthesizer patch *and both patterns* including settings.

### Refinements

- Layout optimized for **480×272**: current-step and selected-step colors, succinct articulation labels, properly spaced ARP/SEQ tabs.
- External MIDI BPM display with hysteresis to reduce one-BPM flicker. This display stabilization does **not** change sequencer clock timing.
- Preset browser success statuses automatically return to READY; new saves display SAVING… → SAVE OK → READY, while errors remain visible.
- Splash screen updated to **v1.3.0**.

### Compatibility

**V1 and V2 presets remain readable by v1.3.0**, with the sequencer reset to defaults. **V3 presets are not backward-readable by v1.2.7**: back up existing `.rtal` files before saving with v1.3.0.

### Hardware and firmware

- ESP32-S3 / Guition JC4827W543, GT911 touchscreen, LVGL 8.4.0.
- External PCM5102A stereo I2S audio, 48 kHz / 128 frames.
- DIN MIDI + native USB MIDI and microSD card with 128 preset slots.
- Download the firmware ZIP and open `RTAL_MINIAMUSED_1_3_0_FINAL.ino` from the identically named sketch folder.

### Testing

The development stages A–J were iteratively verified by the instrument owner on hardware. The renamed FINAL release is promoted from the tested J firmware, with no new functional DSP or sequencer changes. A new independent Arduino compile and flash of the FINAL archive were **not** performed in this documentation workflow.

### Documentation & photos

- [`README.md`](README.md) — full GitHub project overview (English).
- [`docs/RTAL_MiniAmused_v1.3.0_Handbuch.pdf`](docs/RTAL_MiniAmused_v1.3.0_Handbuch.pdf) — detailed German user and reference manual.
- `images/MiniAmused_SEQ_1.jpg`, `MiniAmused_SEQ_2.jpg`, … — sequential naming convention for sequencer photos.

**RealTimeAudioLab · RTAL-EAI-011**

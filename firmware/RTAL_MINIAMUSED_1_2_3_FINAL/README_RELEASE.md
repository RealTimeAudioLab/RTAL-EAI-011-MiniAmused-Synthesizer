# RTAL MiniAmused v1.2.3 FINAL

Release firmware for **RTAL-EAI-011 MiniAmused Synthesizer**.

## Release basis

This release is based on the tested `MINIAMUSED_123_PRESETUI3` development state.
No DSP, synthesis, MIDI, ARP or preset-file-format behavior was changed for the FINAL packaging.
Only release/version strings and package cleanup were applied.

## Main v1.2.3 features

- Monophonic three-oscillator virtual-analog synthesizer
- 4-stage ladder-style low-pass filter
- Filter and loudness envelopes
- DIN MIDI + native USB MIDI
- Seven UI pages: OSC / MIX / FILTER / MOD / EFX / ARP / PRESET
- Stereo delay with MIDI Clock sync
- Arpeggiator modes: UP / DOWN / UP-DOWN / RANDOM / MARKOV
- MARKOV / MUTATION / RANGE / HOLD
- DENSITY / SWING / RATCHET / ACCENT / ACCENT MODE / REPEAT
- Internal ARP tempo 40-300 BPM or external MIDI Clock
- 128 preset slots
- Accelerated press-and-hold preset browsing
- Separate fixed fields for preset number and preset name
- v1.2.3 startup splash screen

## SD card preset structure

```text
/RTAL_MINIAMUSED/
└── presets/
    ├── 000.rtal
    ├── 001.rtal
    ├── ...
    └── 127.rtal
```

The directories are created automatically when required.
Older presets previously stored in `/presets` are not moved automatically and should be copied to `/RTAL_MINIAMUSED/presets` if they are to be retained.

## Arduino sketch

Open:

`RTAL_MINIAMUSED_1_2_3_FINAL/RTAL_MINIAMUSED_1_2_3_FINAL.ino`

The sketch folder and `.ino` filename intentionally match exactly for Arduino IDE compatibility.

## Release note

The historical development/diagnostic README files are intentionally not included in this clean release package. They remain part of the development archives.

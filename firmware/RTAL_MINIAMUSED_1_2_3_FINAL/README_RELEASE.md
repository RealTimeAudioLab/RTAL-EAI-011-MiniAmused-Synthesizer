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


## MIDI CC32 Bank 2 – ARP

In MOPHO/banked MIDI mapping mode, send `CC32 = 2` to select the ARP control bank.

| CC | ARP parameter |
|---:|---|
| 106 | ARP ON/OFF |
| 109 | MODE |
| 110 | RATE |
| 111 | GATE |
| 112 | OCTAVES |
| 115 | MARKOV |
| 118 | MUTATION |
| 119 | RANGE |
| 75 | HOLD |
| 76 | DENSITY |
| 86 | SWING |
| 89 | RATCHET |
| 90 | ACCENT |
| 77 | ACCENT MODE |
| 78 | REPEAT |
| 15 | MIDI SYNC |
| 14 | TEMPO |

Banks 0 and 1 remain unchanged. MIDI realtime F8/FA/FB/FC remains independent of the selected CC32 bank.


## FINAL3 MIDI Sync adjustment

For CC32 Bank 2, ARP MIDI SYNC remains mapped to CC15. The MOPHO controller sends CC15 in the range 1..12, so the switch threshold is evaluated on the raw MIDI value:

- CC15 values 1..5: internal ARP clock
- CC15 values 6..12: MIDI Sync

This replaces the generic 0..127 midpoint behavior for ARP MIDI SYNC only.


## FINAL4 Mopho Bank 1 EFX / Delay mapping

In MOPHO mode, CC32 value 1 selects the EFX / Delay bank:

- CC106 Delay Enable
- CC109 Delay Time
- CC110 Delay Feedback
- CC111 Delay Mix
- CC112 Delay Filter
- CC115 Delay Width
- CC118 Ping / Ping Mode
- CC119 Ping Amount
- CC75 Stereo Width
- CC76 Delay Sync (normal 0..127 switch: 0..63 OFF, 64..127 ON)
- CC86 Delay Division

CC32 value 2 remains the ARP bank from FINAL3. ARP MIDI Sync remains the special Mopho CC15 1..12 mapping with the switch at raw value 6.


## FINAL – CC102 BANK0 GUARD

In MOPHO mode CC102 is reserved exclusively for Bank 0 FILTER CUTOFF.

- CC32=0 + CC102 -> FILTER CUTOFF
- CC32=1 + CC102 -> ignored
- CC32=2 + CC102 -> ignored
- MIDI Learn cannot assign CC102 in MOPHO mode.
- Legacy learned MOPHO CC102 overrides are removed from the persistent map at startup.

All other Bank 0/1/2 factory mappings remain unchanged from FINAL4.

## Final release status

RTAL MiniAmused v1.2.3 FINAL is based on the hardware-tested FINAL5 build.
The CC102 bank guard was verified on hardware: CC102 controls Filter Cutoff only with CC32 Bank 0 and is inactive in Banks 1 and 2.

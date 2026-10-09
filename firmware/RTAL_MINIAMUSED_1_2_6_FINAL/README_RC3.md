# RTAL MiniAmused v1.2.6 RC3 – SD CARD ERROR HANDLING

Based on v1.2.6 RC2; no audio/DSP changes.

- Boot: nonblocking 4-second red warning `SD CARD NOT FOUND` if SD mount fails, or `SD PRESET DIR ERROR` if the card mounts but the preset directory cannot be initialized.
- PRESET page: persistent red error status in either failure case, including after navigation and unsuccessful load/save/delete.
- When SD is ready but a preset is absent: `EMPTY SLOT` while browsing and `PRESET NOT FOUND` after a failed LOAD, not an SD failure.
- Existing preset UI, MIDI and synth operation retained.

Requires Arduino IDE 2.3.x / ESP32 core 3.3.11 and hardware verification. No compile or board test performed here.

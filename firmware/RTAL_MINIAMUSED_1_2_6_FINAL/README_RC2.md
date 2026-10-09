# RTAL MiniAmused v1.2.6 RC2 – GLOBAL TOUCH DEBOUNCE

Based on v1.2.6 RC1 (preset navigation fix) and v1.2.5 FINAL audio engine.

- Existing 180 ms debounce remains active for all stepped UI parameter bindings, including all ON/OFF parameters (ARP, HOLD, GLIDE, DELAY, MOD, SYNC etc.).
- Additional 180 ms click debounce for standalone MIDI MAP and MIDI CH buttons, previously outside the Binding handler.
- Preset UP/DOWN keeps its independent RC1 debounce and 500 ms hold-repeat / acceleration.
- MIDI input, sliders, page navigation, DSP and audio remain unchanged.
- Startup splash remains v1.2.5 until final approval.

Hardware test: Toggle ARP/HOLD/DELAY/GLIDE/MIDI SYNC and MIDI MAP/CH; tap preset UP/DOWN and hold to repeat. Check that legitimate consecutive taps still work.

Not hardware-compiled in this environment.

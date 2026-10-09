# RTAL MiniAmused v1.2.6 RC1 – PRESET NAVIGATION FIX

Based on the hardware-tested v1.2.5 FINAL. Only the PRESET UP/DOWN touch event handler is changed.

- Each accepted short touch advances exactly one preset.
- A 180 ms per-direction tap guard rejects release/re-press bounce.
- A press must be armed by LV_EVENT_PRESSED before LV_EVENT_CLICKED can change a slot.
- Long-press auto-repeat starts at 500 ms; accelerated rates remain unchanged.
- RELEASED / PRESS_LOST stop repeating immediately.
- The splash image intentionally remains v1.2.5 until a FINAL release is approved.
- No changes to audio DSP, resonance, MIDI, ARP, presets on SD, or other touch controls.

Test: tap UP and DOWN repeatedly, then hold each arrow for 2 seconds and release. Confirm single steps, accelerated scrolling and immediate stop.

Note: ZIP integrity checked; Arduino compilation and hardware validation must be performed on the target board.

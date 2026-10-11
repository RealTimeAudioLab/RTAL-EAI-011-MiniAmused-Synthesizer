# RTAL MiniAmused v1.2.7 FINAL

**RealTimeAudioLab / RTAL-EAI-011**  
**Release status:** v1.2.7 FINAL, promoted from the **hardware-tested v1.2.7 PRESET FIX1** source.

## Release highlights

- **Ladder CUTOFF RANGE FIX:** Use the existing extended LUT evaluator `fastExp2Envelope` in `SynthEngine.cpp`, removing the accidental 5,120 Hz computational ceiling of the former `fastExp2` call. The user evaluated the resulting open-filter response through FFT measurements and a listening comparison. The ladder stage has a separate internal 18 kHz safety clamp.
- **PRESET UP/DOWN FIX1:** Hardware-confirmed gesture guard handling the GT911/LVGL touch event sequence. It merges touch bounce and prevents unintended double steps, with a 110 ms release-settling window, 300 ms same-arrow guard, and preserved automatic hold-repeat beginning at 500 ms.
- All other working functions of the established v1.2.6 baseline remain in place: MONO/DUAL, non-linear four-stage ladder with 2× oversampling, resonance-related bass compensation, MIDI DIN and USB, Markov ARP, stereo delay, SD preset management and touch UI.

## Installation

Open `RTAL_MINIAMUSED_1_2_7_FINAL.ino` with Arduino IDE, keeping every associated `.cpp` and `.h` file in the same `RTAL_MINIAMUSED_1_2_7_FINAL` folder. Keep the already tested ESP32-S3 board/Arduino Core, LVGL 8.4.0, Arduino_GFX, MIDI and peripheral configuration.

## Validation scope

- **User hardware tests:** v1.2.7 DEV1 ladder filter measurements and audible improvement; subsequent v1.2.7 PRESET FIX1 touch handling confirmed working.
- **Final promotion:** source unchanged except firmware version strings and sketch filename; preset-touch and DSP implementation are byte-identical to the tested PRESET FIX1 source.
- ZIP and sketch-folder structure checked, desktop preset-gesture tests run. **The renamed FINAL sketch has not been rebuilt with the ESP32-S3 toolchain or reflashed here.**

**Splash note:** The embedded bitmap splash graphics are carried over unmodified from PRESET FIX1; the splash artwork may display an older version number. The serial banner and heartbeat identify `v1.2.7 FINAL`.

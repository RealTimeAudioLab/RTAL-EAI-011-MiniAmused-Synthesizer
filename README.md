# RTAL MiniAmused v1.2.6

## What's New in v1.2.6 FINAL?

**Reliable preset navigation:** The PRESET UP/DOWN touch controls now use a corrected LVGL event sequence with 180 ms debounce. A short tap moves exactly one slot; holding a button starts automatic scrolling after approximately 500 ms, with acceleration. The fix preserves the correct order of RELEASED and CLICKED events. This behaviour was confirmed on the instrument after the RC3 PRESET FIX1 correction.

**Consistent discrete-touch handling:** The 180 ms touch debounce is retained for ON/OFF parameters, including ARP and HOLD. MIDI input and continuous sliders remain independent of touch debounce.

**SD-card diagnostics:** Missing or uninitialized microSD media and preset-directory failures produce visible status information instead of silently failing. An absent card does not prevent synthesis and MIDI operation. Empty preset slots are distinguished from media errors.

**Audio continuity:** MONO/DUAL synthesis, the 2× oversampled four-stage ladder filter with resonance bass compensation, the Markov arpeggiator and all five stereo-delay ping-pong modes remain unchanged from the proven v1.2.5 audio baseline.

**Release basis:** v1.2.6 FINAL is based on the hardware-tested `v1.2.6 RC3 PRESET FIX1` firmware.

---

> **Current release: v1.2.6 FINAL** · Mono/Dual (up to 2 voices × 3 oscillators), fuller ladder resonance, reliable ON/OFF touch controls.

## Evolution through v1.2.5

**v1.2.4 – Dual Voice:** Added the `VOICE MODE` control on the **MOD** page. `MONO` uses one complete three-oscillator voice; `DUAL` permits **two simultaneously sounding three-oscillator voices** (up to six oscillator instances). Both voices share the downstream mixer, four-stage nonlinear ladder filter, filter/loudness envelopes and stereo delay. The second voice has independent oscillator phases and pitch/glide state; voice allocation uses oldest-voice stealing. In DUAL mode the oscillator sum is normalized by **0.65** when both voices are active, instead of 1.0 for a single active voice. The audio engine was optimized for two-voice operation at 48 kHz / 128 frames; the detailed DSP profiler was removed from the final firmware.

**v1.2.5 – Fuller Resonance & Touch Reliability:** Added resonance-dependent bass compensation to the existing four-stage ladder filter, beginning around **20% EMPHASIS**. The tuned RC2 response retains more body at medium-to-high resonance without introducing a direct dry-signal bypass. The filter retains **2× oversampling**. Discrete ON/OFF touch controls now use **180 ms debounce**, addressing double triggers reported on **ARP ON/OFF** and **HOLD ON/OFF**.

**Compatibility and scope:** The seven pages (OSC / MIX / FILTER / MOD / EFX / ARP / PRESET), Markov arpeggiator, MIDI/CC32 banks, 128 microSD preset slots and stereo delay remain. The historical v1.2.3 features below are retained. The new `VOICE MODE` parameter is a UI control on the MOD page; no new MIDI CC assignment is claimed here. Existing presets may need a quick review of their voice-mode setting after upgrading.


### Mono/Dual ESP32-S3 Synthesizer with Touch UI, Stereo Delay and Markov Arpeggiator

**RTAL-EAI-011 – Real Time Audio Lab**

RTAL MiniAmused is a mono/dual virtual-analog synthesizer built around the
ESP32-S3 and the Guition JC4827W543 480×272 touch display.

MiniAmused combines a classic three-oscillator subtractive synthesizer
architecture with a modern embedded DSP platform, touch operation,
preset management, DIN and USB MIDI, a stereo delay engine and an
advanced arpeggiator with probabilistic Markov sequencing.

Version **1.2.6** represents the current stable release of the project.

---

## Highlights

- ESP32-S3 based real-time synthesizer
- Guition JC4827W543 480×272 capacitive touch display
- Selectable MONO/DUAL architecture (3 oscillators per voice)
- Six oscillator waveforms
- Noise generator
- 4-stage nonlinear ladder-style low-pass filter
- 2× oversampled mixer/filter path
- Separate Filter and Loudness ADSR envelopes
- Glide / Portamento
- LOW / LAST / HIGH note priority
- SINGLE / MULTI trigger modes
- Modulation bus with OSC3 and Noise
- Stereo Delay with five Ping-Pong modes
- MIDI Clock synchronized Delay
- Advanced Arpeggiator
- Markov-based note generation
- Density, Swing, Ratchet, Accent and Repeat
- Internal 40–300 BPM ARP clock
- External MIDI Clock synchronization
- DIN MIDI
- Native USB MIDI
- MIDI Learn / extended CC mapping
- 128 presets on microSD
- LVGL 8.4.0 touch interface
- Dedicated real-time Audio, MIDI and ARP processing

---

# Synthesizer Architecture

MiniAmused follows a deliberately straightforward subtractive signal path:

```text
                       MIDI / ARPEGGIATOR
                               |
                               v
                         NOTE / GLIDE
                               |
                               v
OSC 1 ----\
OSC 2 -----+----> MIXER / DRIVE ----> LADDER FILTER ----> LOUDNESS
OSC 3 ----/              ^                    ^                |
NOISE ----/               |                    |                |
                      FEEDBACK             FILTER ADSR          |
                                                               v
                                                        STEREO DELAY
                                                               |
                                                               v
                                                           I2S OUTPUT
```

The instrument supports **MONO** and **DUAL** modes.

All three oscillators form one synthesizer voice rather than three
independent polyphonic voices.

This allows the available ESP32-S3 processing power to be concentrated
on oscillator quality, nonlinear filtering, modulation, envelopes,
delay processing and real-time performance features.

---

# Audio Engine

MiniAmused runs at:

- **48 kHz sample rate**
- **128 audio frames per block**
- ESP32-S3 @ 240 MHz

The audio engine runs independently from the graphical user interface.

Time-critical synthesis and DSP processing is handled by the dedicated
audio task, while MIDI, arpeggiator and UI operations are separated from
the audio rendering path.

This architecture prevents display activity from determining musical
timing.

---

# Oscillators

MiniAmused provides three oscillators per voice (up to six in DUAL mode).

Available waveforms:

- TRI
- SHARK
- SAW
- SAW/SQ
- SQUARE
- PULSE

Available ranges:

- 32'
- 16'
- 8'
- 4'
- 2'

OSC2 and OSC3 provide independent tuning controls.

OSC3 can also operate independently from keyboard tracking and can be
used as a modulation source.

---

# Mixer

The mixer combines:

- OSC1
- OSC2
- OSC3
- Noise

Additional controls include:

- Drive
- Feedback
- Master Level
- Master Tune
- Glide Time
- Glide On/Off

Drive and Feedback allow the otherwise clean oscillator signals to be
pushed into a considerably more aggressive analog-style signal path.

---

# Ladder Filter

MiniAmused uses an independently developed RTAL nonlinear low-pass
filter inspired by the behavior of classic four-stage ladder filters.

The implementation uses:

- four nonlinear one-pole stages
- global resonance feedback
- 2× oversampling in the mixer/filter path
- keyboard tracking
- dedicated filter envelope

Filter controls include:

- Cutoff
- Emphasis / Resonance
- Keyboard Tracking
- Filter Contour
- ADSR

Keyboard tracking modes:

- OFF
- 1/3
- 2/3
- FULL

---

# Envelopes

Two independent ADSR envelopes are provided.

## Filter Contour

- Attack
- Decay
- Sustain
- Release
- Amount

## Loudness Envelope

- Attack
- Decay
- Sustain
- Release
- Decay Switch

The envelope time range extends to approximately **10 seconds**.

---

# Modulation

The modulation system combines OSC3 and Noise into a common modulation
bus.

The modulation bus can control:

- Oscillator Pitch
- Filter Cutoff

The amount can be controlled using the modulation wheel.

MiniAmused supports standard MIDI **CC1 Mod Wheel** control.

---

# Note Priority and Trigger Modes

Three monophonic note-priority modes are available:

### LOW
The lowest currently held note has priority.

### LAST
The most recently played note has priority.

### HIGH
The highest currently held note has priority.

Two trigger modes are available:

### SINGLE
Legato-oriented behavior. Playing another note while a key remains held
does not necessarily retrigger the envelopes.

### MULTI
New notes retrigger the Filter and Loudness envelopes.

---

# Stereo Delay

MiniAmused intentionally uses one integrated effect architecture:
**Stereo Delay**.

Parameters include:

- Delay On/Off
- Delay Time
- Feedback
- Mix
- Delay Filter
- Width
- Stereo Width
- Ping-Pong Mode
- Ping Amount
- MIDI Sync
- Clock Division

Delay time range: **approximately 20–1000 ms**

## Five Ping-Pong Modes

- **LEGACY** – Original MiniAmused crossed-feedback topology
- **CLASSIC** – Clearly alternating left/right repeats
- **SOFT** – More centered and less aggressive stereo movement
- **WIDE** – Strong cross-feedback combined with stereo tap offsets
- **DUAL** – Two largely independent left/right delays with subtle coupling

## Stable Delay Time

Changing the delay time does not continuously move the active delay tap.
Instead, MiniAmused crossfades between the previous and new fixed delay
positions. This substantially reduces unwanted Doppler and flanger
artifacts when changing Delay Time or synchronized delay divisions.

---

# MIDI Clock Delay Sync

Available divisions include:

- 1/4
- 1/8
- 1/8D
- 1/8T
- 1/16
- 1/16D
- 1/16T
- 1/32

MIDI realtime messages:

- F8 – Clock
- FA – Start
- FB – Continue
- FC – Stop

Continuous MIDI Clock can remain present while the external sequencer
transport is stopped.

---

# Arpeggiator

Available modes:

- UP
- DOWN
- UP-DOWN
- RANDOM
- MARKOV

Additional controls include:

- Rate
- Gate
- Octaves
- Markov Amount
- Mutation
- Range
- Hold
- Density
- Swing
- Ratchet
- Accent
- Accent Mode
- Repeat
- MIDI Sync
- Internal Tempo

---

# Markov Arpeggiator

The **MARKOV** mode was introduced before v1.2.5 and remains a central feature.

Unlike a conventional random arpeggiator, the next note is not selected
completely independently. The current position influences the
probability of the next movement.

Possible transitions include:

```text
SAME
+1
-1
+2
-2
RANDOM
```

The **MARKOV** parameter determines how strongly Markov selection is
mixed with conventional deterministic arpeggiator behavior.

## Mutation

MUTATION introduces additional deviations from the normal Markov
movement.

## Range

RANGE controls how locally or widely the Markov engine moves through
the held note pool.

---

# Hold

HOLD can latch the current ARP note pool.

MiniAmused internally distinguishes between physically held notes and
latched notes.

When HOLD is disabled, previously released latched notes are removed
immediately while notes that are still physically held remain active.

Turning the ARP off clears the ARP note pool.

---

# Density

DENSITY determines whether an otherwise valid arpeggiator step actually
produces a note.

At **100 %**, every step can sound. Lower settings introduce real
rhythmic gaps.

---

# Swing

SWING offsets alternating steps and changes the rhythmic feel without
changing the basic sequence.

---

# Ratchet

RATCHET creates additional note triggers inside an arpeggiator step.

The implementation is non-blocking and therefore does not stop the
audio engine or UI while the ratchet sequence is generated.

---

# Accent

Available Accent modes:

- 4 STEP
- 3 STEP
- 2 STEP
- RANDOM
- MARKOV

---

# Repeat

REPEAT determines the probability that the previous chord position is
used again before the normal note-selection process continues.

---

# Internal and External ARP Clock

The arpeggiator can use either:

- Internal Clock: **40–300 BPM**
- External MIDI Clock: F8 with FA Start, FB Continue and FC Stop

Clock reception and transport state are deliberately separated.

---

# Touch User Interface

MiniAmused uses:

- **Guition JC4827W543**
- ESP32-S3
- 480×272 TFT
- capacitive GT911 touch controller
- LVGL 8.4.0

Version 1.2.3 contains seven main pages:

```text
OSC
MIX
FILTER
MOD
EFX
ARP
PRESET
```

---

# Startup Screen

MiniAmused v1.2.6 displays its own 480×272 startup screen:

```text
RTAL
MiniAmused
SYNTHESIZER
v1.2.5
```

After startup, the OSC page is initialized as the active page.

---

# Preset System

MiniAmused provides **128 preset slots**, numbered **000 ... 127**.

Preset actions:

- LOAD
- SAVE
- SAVE AS
- INIT
- DELETE

Program Change values 0–127 correspond directly to preset slots
000–127.

## Preset Storage

```text
/RTAL_MINIAMUSED/
└── presets/
    ├── 000.rtal
    ├── 001.rtal
    ├── ...
    └── 127.rtal
```

The required directories are created automatically when necessary.

Older presets from the former `/presets/` directory are not
automatically migrated. They can be copied manually to
`/RTAL_MINIAMUSED/presets/`.

---

# Improved Preset Browser in v1.2.5

The three-digit slot number and preset name occupy separate display
areas. Long preset names can therefore no longer overlap the slot
number.

Example:

```text
020    WARM ANALOG BASS
```

Preset navigation also supports accelerated button hold.

- Short touch: previous/next preset
- Hold: automatic scrolling with progressively increasing repeat rate

Browsing does **not** automatically load each preset from the SD card.
The selected sound is loaded only when LOAD is requested.

---

# MIDI

MiniAmused supports MIDI simultaneously through:

- DIN MIDI
- Native USB MIDI

Reception can be configured for:

- OMNI
- MIDI Channel 1–16

Supported realtime messages include:

```text
F8  MIDI Clock
FA  Start
FB  Continue
FC  Stop
```

Pitch Bend and standard MIDI controllers are also supported.

---

# MIDI Learn and CC Mapping

The v1.2.x architecture extends the original fixed MIDI CC system with:

- MIDI Learn
- CC mapping
- CC32 bank selection

This makes it possible to control a considerably larger number of
MiniAmused parameters from external MIDI controllers.

---

# Real-Time Architecture

```text
ESP32-S3

CORE 1
└── AUDIO TASK
    ├── Oscillators
    ├── Mixer
    ├── Drive / Feedback
    ├── Ladder Filter
    ├── Envelopes
    ├── Modulation
    ├── Stereo Delay
    └── I2S Output

CORE 0
├── ARP TASK
│   ├── Timing
│   ├── Note Selection
│   ├── Markov
│   ├── Density
│   ├── Swing
│   ├── Ratchet
│   └── Accent
│
└── MIDI TASK
    ├── DIN MIDI
    ├── USB MIDI
    ├── MIDI Clock
    └── Transport

UI / SYSTEM
├── LVGL
├── Touch
├── Preset Manager
└── SD Card
```

The arpeggiator does not depend on LVGL refresh timing.

---

# Why LVGL 8.4.0?

MiniAmused deliberately remains on **LVGL 8.4.0**.

The complete UI, overlay, touch and memory architecture has been
developed and tested around this version. Moving to another LVGL major
version would be a GUI migration rather than a simple library update.

---

# Development Environment

Recommended development environment:

- Arduino IDE 2.x
- ESP32-S3
- Arduino-ESP32 3.x
- LVGL 8.4.0
- Arduino_GFX
- microSD support
- Native USB MIDI

The release was developed around the Guition JC4827W543 ESP32-S3
touch-display platform.

---

# What's New in v1.2.3 (historical)?

Compared with the original MiniAmused v1.0 release:

- dedicated ARP page
- real-time Arpeggiator task
- UP / DOWN / UP-DOWN / RANDOM modes
- Markov Arpeggiator
- Markov Amount
- Mutation
- Range
- Hold
- Density
- Swing
- Ratchet
- Accent
- five Accent modes
- Repeat
- internal ARP clock
- MIDI synchronized ARP
- expanded MIDI mapping
- MIDI Learn
- CC32 bank architecture
- improved startup behavior
- dedicated MiniAmused splash screen
- improved preset browser
- accelerated preset scrolling
- separated preset number/name display
- project-specific preset directory
- numerous UI and stability refinements

The original synthesis controls remain, extended by MONO/DUAL operation and improved resonance.

---

# v1.2.6 Release Status
**RTAL MiniAmused v1.2.6 FINAL**

The release combines the established MiniAmused synthesizer engine with
the new Markov performance architecture while retaining the stable
real-time audio design developed throughout the project.

The v1.2.6 release is intended as the stable reference point for future
MiniAmused development.

---

# Project Philosophy

MiniAmused is not intended to be a component-by-component reproduction
of a historical synthesizer.

Instead, the project combines the clarity and immediacy of a classic
monophonic subtractive synthesizer with modern embedded technology:

- real-time DSP
- touch operation
- preset storage
- USB and DIN MIDI
- synchronized effects
- probabilistic sequencing
- generative performance tools

The goal is to build a compact, playable and musically useful
standalone instrument.

---

# Documentation

The release includes the updated and redesigned detailed:

**RTAL MiniAmused v1.2.6 – Detailliertes Benutzer- und Referenzhandbuch**

The manual covers complete operation, synthesis, stereo delay, Markov
arpeggiator, preset management, MIDI, real-time architecture,
sound-design examples, troubleshooting and parameter reference.

---

# Project

**RTAL-EAI-011 MiniAmused Synthesizer**

Developed by **Real Time Audio Lab**

Version **1.2.6 FINAL**
---

## Acknowledgement

MiniAmused is inspired by the immediacy, signal flow and musical
philosophy of classic monophonic analog synthesizers.

Special appreciation goes to **Moog Music** and the instruments that
helped establish this form of subtractive synthesis and hands-on
electronic music performance.

MiniAmused is an independent Real Time Audio Lab project and is not
affiliated with or endorsed by Moog Music.

---

## License

Please refer to the license information included with the RTAL
MiniAmused repository.

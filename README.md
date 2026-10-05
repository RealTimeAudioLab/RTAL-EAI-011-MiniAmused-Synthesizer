# RTAL-EAI-011 MiniAmused Synthesizer

### Monophonic Virtual-Analog Synthesizer · ESP32-S3 · 4.3" Touch UI · MIDI · Real-Time DSP

![RTAL MiniAmused](docs/MiniAmused_OSC.jpg)

**RTAL MiniAmused** is a monophonic virtual-analog synthesizer developed
by **RealTimeAudioLab (RTAL)** as part of the **RTAL Embedded Audio
Initiative (EAI)**.

MiniAmused explores how the architecture, immediacy and
performance-oriented workflow of a classic monophonic analog synthesizer
can be reinterpreted using modern embedded hardware.

At the heart of the instrument is an **ESP32-S3** running the real-time
synthesis engine, MIDI processing and graphical user interface. The
complete instrument is controlled from a **4.3-inch capacitive
touchscreen** using the **Guition JC4827W543 ESP32-S3 display
platform**.

MiniAmused combines:

-   monophonic virtual-analog synthesis
-   multiple oscillators forming one synthesizer voice
-   real-time subtractive synthesis
-   analog-inspired filter processing
-   envelopes and modulation
-   dedicated touchscreen parameter pages
-   **LVGL 8.4.0** graphical user interface
-   **GT911** capacitive touch
-   DIN MIDI
-   native USB MIDI
-   continuous MIDI Clock synchronization
-   preset management
-   SD-card storage
-   a dedicated **stereo delay** effect
-   external I2S audio conversion
-   optimized ESP32-S3 DSP

The goal is not to create a software clone. The goal is to create a
**real embedded musical instrument**.

------------------------------------------------------------------------

# The Idea

The fundamental question behind MiniAmused was:

> **How much of the experience of a classic monophonic performance
> synthesizer can be recreated and extended using a modern ESP32-S3
> embedded platform?**

Traditional analog synthesizers achieve much of their immediacy through
a physical front panel: one function, one control, immediate
interaction.

MiniAmused translates this philosophy into a modern touchscreen
instrument. Instead of deeply nested menus, the synthesizer is divided
into dedicated functional pages representing the individual sections of
the signal path.

``` text
OSCILLATORS
     │
     ▼
   MIXER
     │
     ▼
   FILTER
     │
     ▼
 AMPLIFIER
     │
     ▼
   DELAY
     │
     ▼
STEREO OUTPUT
```

Modulation sources, envelopes, MIDI, presets and performance functions
interact with this central signal path.

------------------------------------------------------------------------

# Inspired by the Minimoog

![Minimoog Inspiration](docs/Minimoog_Inspiration.jpg)

MiniAmused is strongly inspired by one of the most influential
electronic musical instruments ever created: the **Moog Minimoog**.

The Minimoog demonstrated how the flexibility of modular synthesis could
be transformed into a compact, immediate and highly expressive musical
instrument. Its basic architecture remains remarkably clear:

``` text
OSCILLATORS → MIXER → FILTER → AMPLIFIER
```

That philosophy is an important inspiration for MiniAmused.

MiniAmused is not intended to reproduce the original instrument circuit
by circuit. Instead, it asks what this fundamental concept might look
like when implemented using a modern microcontroller, real-time DSP,
touchscreen control, digital preset storage, USB MIDI, MIDI Clock,
graphical parameter visualization and an integrated digital delay.

The result is an independent RTAL instrument with its own architecture
and development path.

------------------------------------------------------------------------

# A Word of Thanks

RealTimeAudioLab would like to express its respect and gratitude to
**Bob Moog**, the engineers involved in the development of the Minimoog,
and the generations of musicians who demonstrated what this instrument
architecture could become.

The Minimoog established ideas that remain relevant more than half a
century later: **a clear signal path, immediate controls, expressive
interaction and an instrument designed to be played rather than
programmed.**

Those principles are a major inspiration behind MiniAmused.

MiniAmused is an independent **RealTimeAudioLab** project and is not
affiliated with, sponsored by, or endorsed by Moog Music.

Moog and Minimoog are trademarks of their respective owners.

------------------------------------------------------------------------

# Hardware Platform

## Guition JC4827W543

![JC4827W543 Front](docs/JC4827W543_Front.jpg)

The central hardware platform of MiniAmused is the **Guition
JC4827W543**.

This compact module combines an ESP32-S3 microcontroller with a 4.3-inch
touchscreen, making it particularly interesting for standalone embedded
musical instruments.

  Component          MiniAmused Platform
  ------------------ ----------------------------------
  Processor          ESP32-S3
  CPU architecture   Dual-core Xtensa LX7
  CPU clock          up to 240 MHz
  Display            4.3-inch TFT
  Resolution         480 × 272
  Touch              Capacitive
  Touch controller   GT911
  External RAM       PSRAM
  Storage            Flash + microSD
  GUI                **LVGL 8.4.0**
  MIDI               DIN + Native USB MIDI
  Audio interface    External I2S
  Application        Real-time monophonic synthesizer

The display is not simply a status panel. **It is the front panel of
MiniAmused.**

------------------------------------------------------------------------

# JC4827W543 -- Rear View

![JC4827W543 Rear](docs/JC4827W543_Rear.jpg)

The JC4827W543 provides the processing and peripheral interfaces
required for display rendering, capacitive touch, SD-card access, MIDI
communication, USB MIDI, external audio hardware and system control.

This integration makes the board particularly suitable for a compact
standalone synthesizer: UI, touch processing and the main embedded
processor are combined on one platform while external I2S hardware
handles the audio conversion.

------------------------------------------------------------------------

# Why ESP32-S3?

MiniAmused deliberately uses the ESP32-S3 as a real-time musical
instrument processor.

The platform provides a useful combination of dual CPU cores, up to 240
MHz clock frequency, floating-point processing, DSP/vector capabilities,
DMA, I2S, SPI, I²C, UART, native USB and external PSRAM support.

The challenge is not simply generating a waveform. MiniAmused
simultaneously handles:

``` text
Real-Time Audio DSP
        +
MIDI Processing
        +
USB MIDI
        +
Touch Processing
        +
LVGL 8.4.0 Graphics
        +
Preset Management
        +
SD Card
        +
Stereo Delay
        +
System Control
```

All of these systems must coexist without compromising the audio stream.

------------------------------------------------------------------------

# Monophonic Voice Architecture

MiniAmused is deliberately designed as a **monophonic synthesizer**.

The available processing resources are concentrated on a single complete
synthesis voice rather than being divided among several independent
voices. Multiple oscillators form part of this **one voice**.

``` text
                 MIDI NOTE
                     │
                     ▼
              NOTE PROCESSING
                     │
                     ▼
          ┌────────────────────┐
          │     OSCILLATORS    │
          │                    │
          │       OSC 1        │
          │       OSC 2        │
          │       OSC 3        │
          └─────────┬──────────┘
                    │
                    ▼
                  MIXER
                    │
                    ▼
                  FILTER
                    │
                    ▼
                AMPLIFIER
                    │
                    ▼
              STEREO DELAY
                    │
                    ▼
              AUDIO OUTPUT
```

> **Multiple oscillators do not mean multiple voices.**

All oscillators interact inside the same monophonic synthesis path.

This architecture is particularly suited to basses, leads, sequences,
arpeggiated lines, effects and experimental sounds.

------------------------------------------------------------------------

# Touchscreen User Interface

MiniAmused uses a page-oriented touchscreen interface built with **LVGL
8.4.0**. The actual instrument has exactly six permanent pages:

``` text
OSC | MIX | FILTER | MOD | PRESET | EFX
```

There are **no separate AMP, ENV or LFO pages**. Their relevant controls
are integrated into the six functional pages of the instrument. This
keeps the UI compact and preserves a direct, performance-oriented
workflow.

Each screenshot in this documentation follows the real tab name:

``` text
docs/MiniAmused_OSC.jpg
docs/MiniAmused_MIX.jpg
docs/MiniAmused_FILTER.jpg
docs/MiniAmused_MOD.jpg
docs/MiniAmused_PRESET.jpg
docs/MiniAmused_EFX.jpg
```

------------------------------------------------------------------------

# OSC -- Oscillators

![MiniAmused OSC](images/MiniAmused_OSC.jpg)

The **OSC** page defines the raw sound of the monophonic voice.
MiniAmused uses three oscillators. They are not separate voices: all
three feed the same mixer, filter and amplifier path.

## OSC parameters

  -----------------------------------------------------------------------
  Parameter                           Function
  ----------------------------------- -----------------------------------
  **OSC 1 WAVE**                      Selects the waveform generated by
                                      oscillator 1. The implemented
                                      waveform selector provides six
                                      discrete waveform positions;
                                      examples visible in the firmware
                                      include TRI, SQUARE and PULSE.

  **OSC 1 RANGE**                     Selects the octave/register of
                                      oscillator 1 and therefore its
                                      basic pitch range.

  **OSC 2 WAVE**                      Selects the waveform generated by
                                      oscillator 2.

  **OSC 2 RANGE**                     Selects the octave/register of
                                      oscillator 2.

  **OSC 2 TUNE**                      Detunes oscillator 2 relative to
                                      the played note and oscillator 1.
                                      This is central to beating,
                                      thickness and interval
                                      relationships.

  **OSC 3 WAVE**                      Selects the waveform generated by
                                      oscillator 3.

  **OSC 3 RANGE**                     Selects the octave/register of
                                      oscillator 3.

  **OSC 3 TUNE**                      Detunes oscillator 3 relative to
                                      the main pitch.
  
  **OSC 3 KEYBOARD CONTROL**          Determines whether oscillator 3
                                      follows the played keyboard note.
                                      Disabling keyboard control allows
                                      OSC 3 to be used more like an
                                      independent modulation source.                                    
                                      
  -----------------------------------------------------------------------

The three-oscillator architecture follows the classic
performance-synthesizer idea: several tone generators interact inside
one monophonic signal path.

------------------------------------------------------------------------

# MIX -- Mixer and Signal Character

![MiniAmused MIX](images/MiniAmused_MIX.jpg)

The **MIX** page determines how strongly the individual sound sources
enter the following filter stage and also contains parameters that
influence the character of the combined signal.

## MIX parameters

  -----------------------------------------------------------------------
  Parameter                           Function
  ----------------------------------- -----------------------------------
  **OSC 1 LEVEL**                     Sets the contribution of oscillator
                                      1 to the mixer.

  **OSC 2 LEVEL**                     Sets the contribution of oscillator
                                      2 to the mixer.

  **OSC 3 LEVEL**                     Sets the contribution of oscillator
                                      3 to the mixer.

  **NOISE LEVEL**                     Adds the noise generator to the
                                      oscillator mix. Noise can add
                                      attack, breath, dirt and broadband
                                      harmonic energy.

  **NOISE COLOR**                     Changes the tonal character of the
                                      noise source.

  **MIXER DRIVE**                     Controls how strongly the combined
                                      oscillator signal drives the
                                      following nonlinear signal path.
                                      Higher settings increase density
                                      and harmonic character rather than
                                      acting as a simple output-volume
                                      control.

  **FEEDBACK LEVEL**                  Feeds a controlled part of the
                                      synthesizer signal back into the
                                      signal path to increase density,
                                      edge and resonance-like character.

  **MASTER VOLUME**                   Controls the overall synthesizer
                                      output level.
                                      
  -----------------------------------------------------------------------

The individual oscillator levels therefore influence both balance and
the way the following stages are driven.

------------------------------------------------------------------------

# FILTER -- Ladder Filter and Filter Envelope

![MiniAmused FILTER](images/MiniAmused_FILTER.jpg)

The **FILTER** page contains the main subtractive sound-shaping stage
and its dedicated envelope controls.

## FILTER parameters

  -----------------------------------------------------------------------
  Parameter                           Function
  ----------------------------------- -----------------------------------
  **FILTER CUTOFF**                   Sets the low-pass cutoff frequency.
                                      Lower values remove progressively
                                      more upper harmonics; higher values
                                      open the sound.

  **FILTER RESONANCE**                Emphasizes frequencies around the
                                      cutoff point and increases the
                                      characteristic filter peak.

  **FILTER CONTOUR**                  Sets the amount with which the
                                      filter envelope moves the cutoff
                                      frequency.

  **FILTER KEYTRACK**                 Makes cutoff follow the played note
                                      so the tonal balance can remain
                                      more consistent across the keyboard
                                      or deliberately change with pitch.

  **FILTER ATTACK**                   Sets how quickly the filter
                                      envelope rises after Note On.

  **FILTER DECAY**                    Sets the transition time from the
                                      envelope peak toward its sustain
                                      level.

  **FILTER SUSTAIN**                  Sets the filter-envelope level
                                      maintained while the note remains
                                      held.

  **FILTER RELEASE**                  Sets how long the filter envelope
                                      takes to return after Note Off.
                                      
  -----------------------------------------------------------------------

The addition of **FILTER RELEASE** means the filter contour is a
complete ADSR envelope rather than an attack/decay-only contour.

------------------------------------------------------------------------

# MOD -- Performance, Modulation and Loudness

![MiniAmused MOD](images/MiniAmused_MOD.jpg)

The **MOD** page collects the controls that affect performance
behaviour, modulation routing and the amplitude contour. This is why
MiniAmused does not need separate LFO, ENV or AMP tabs.

## MOD parameters

  -----------------------------------------------------------------------
  Parameter                           Function
  ----------------------------------- -----------------------------------
  **MOD WHEEL**                       Represents the current
                                      modulation-wheel amount received
                                      from the performance control/MIDI
                                      path.

  **MOD MIX**                         Determines the balance/relationship
                                      of the modulation sources used by
                                      the modulation system.

  **OSC MOD**                         Enables modulation of oscillator
                                      pitch.

  **FILTER MOD**                      Enables modulation of filter
                                      cutoff.

  **GLIDE**                           Enables portamento between
                                      successive monophonic notes.

  **GLIDE TIME**                      Sets how long the pitch transition
                                      between notes takes when Glide is
                                      enabled.

  **NOTE PRIORITY**                   Selects the monophonic
                                      note-priority behaviour used when
                                      more than one MIDI key is held.

  **DECAY**                           Enables the classic decay behaviour
                                      used by the envelope system.

  **LOUDNESS ATTACK**                 Sets the rise time of the
                                      amplifier/loudness envelope after
                                      Note On.

  **LOUDNESS DECAY**                  Sets the time from the initial
                                      envelope peak toward the sustain
                                      level.

  **LOUDNESS SUSTAIN**                Sets the amplitude maintained while
                                      a note remains held.

  **LOUDNESS RELEASE**                Sets how long the loudness envelope
                                      takes to fade after Note Off.
                                      
  -----------------------------------------------------------------------

The loudness controls are the amplitude-envelope controls of the single
MiniAmused voice. They are intentionally part of **MOD**, not a separate
AMP or ENV page.

------------------------------------------------------------------------

# PRESET -- Preset Management

![MiniAmused PRESET](images/MiniAmused_PRESET.jpg)

The **PRESET** page manages complete MiniAmused sounds on the microSD
card.

## PRESET functions

  -----------------------------------------------------------------------
  Control / Function                  Description
  ----------------------------------- -----------------------------------
  **Preset Browser**                  Browses the available preset slots.

  **000--099**                        MiniAmused provides 100 preset
                                      positions.

  **LOAD**                            Loads the selected preset into the
                                      central parameter system and synth
                                      engine.

  **SAVE**                            Saves the current sound back to its
                                      current preset slot.

  **SAVE AS**                         Stores the current sound in another
                                      slot and supports naming the new
                                      preset.

  **Preset Name**                     Stores and displays the
                                      human-readable name of the sound.

  **Empty Slot Detection**            Distinguishes unused locations from
                                      stored presets.

  **INIT**                            Restores an initialized synthesizer
                                      state as a clean starting point for
                                      a new sound.

  **Program Change**                  MIDI Program Change 0--99 selects
                                      preset slots 000--099.

  **Header Preset Name**              The currently active preset name is
                                      shown in the instrument UI header.
                                      
  -----------------------------------------------------------------------

Preset loading is deliberately deferred out of the MIDI task so that
SD-card access does not interfere with time-critical MIDI or audio
processing.

``` text
PC 0  → Preset 000
PC 1  → Preset 001
...
PC 99 → Preset 099
```

------------------------------------------------------------------------

# EFX -- Stereo Delay

![MiniAmused EFX](images/MiniAmused_EFX.jpg)

The **EFX** page contains exactly one effect: the MiniAmused **Stereo
Delay**. There is no chorus, flanger, phaser or reverb in the current
MiniAmused effect architecture.

## EFX parameters

  -----------------------------------------------------------------------
  Parameter                           Function
  ----------------------------------- -----------------------------------
  **DELAY**                           Enables or bypasses the delay
                                      processor.

  **TIME**                            Sets the free-running delay time
                                      when SYNC is disabled.

  **FEEDBACK**                        Determines how much delayed signal
                                      is returned to the delay input and
                                      therefore how many repeats are
                                      produced.

  **MIX**                             Sets the dry/wet relationship
                                      between the direct synthesizer and
                                      the delayed signal.

  **FILTER**                          Controls the low-pass filter in the
                                      delay feedback/signal path so
                                      repeats can become progressively
                                      darker.

  **WIDTH**                           Controls the stereo spread of the
                                      delay output.

  **PING PONG**                       Continuously controls the
                                      left/right ping-pong behaviour from
                                      **0--100%**. It is independent from
                                      WIDTH; 0% keeps ping-pong behaviour
                                      disabled while higher values
                                      increasingly alternate/spread
                                      repeats between the stereo
                                      channels.

  **SYNC**                            Switches delay timing between FREE
                                      operation and external MIDI Clock
                                      synchronization.

  **DIV**                             Selects the rhythmic delay division
                                      when SYNC is active.
                                      
  -----------------------------------------------------------------------

## FREE mode

With **SYNC = OFF**, **TIME** directly controls the delay time. This is
useful for conventional echoes, slap-style delays, spatial effects and
deliberately unsynchronized repeats.

``` text
SYNC OFF → TIME → DELAY TIME
```

## MIDI Clock SYNC

With **SYNC = ON**, the delay derives its timing from the continuously
arriving MIDI Clock. MIDI Clock provides 24 F8 ticks per quarter note.
MiniAmused measures complete 24-F8 windows rather than reacting directly
to every individual tick.

The synchronized delay supports musical divisions derived from the
current tempo. The delay buffer is approximately 1.2 seconds long, so
very long divisions at low BPM are limited by the maximum available
delay time.

## Stable Delay Time Engine

Changing delay time abruptly can produce clicks, discontinuities and
strong pitch artifacts. MiniAmused therefore uses a stable transition
mechanism.

For synchronized delay changes, the target delay time is quantized to
**0.5 ms**, unnecessary small target changes are rejected by hysteresis,
and a real timing change is performed using two fixed delay taps with a
**24 ms crossfade**:

``` text
OLD TAP ────────┐
                ├── 24 ms CROSSFADE ──► OUTPUT
NEW TAP ────────┘
```

Clock evaluation also uses stability checking so normal MIDI timing
jitter does not constantly move the delay tap.

## FEEDBACK

FEEDBACK returns part of the delayed signal to the delay input:

``` text
INPUT ──► DELAY ──► OUTPUT
            ▲
            └──── FEEDBACK
```

Low values create a small number of echoes. Higher values produce longer
repeating tails.

## FILTER

The delay includes a low-pass filter so repeats do not need to remain
identical copies of the dry signal. Lower FILTER settings remove more
high-frequency content and push the repeats behind the direct
synthesizer sound.

## WIDTH and PING PONG

**WIDTH** and **PING PONG** are deliberately separate parameters.

WIDTH determines the overall stereo width of the delayed signal. PING
PONG determines how strongly successive delay energy is distributed or
alternated between the left and right channels.

This gives the monophonic MiniAmused voice a flexible stereo space after
synthesis without changing its fundamental monophonic architecture.

------------------------------------------------------------------------

# MIDI Clock Synchronized Delay

With **SYNC = ON**, MiniAmused derives the delay time from the incoming
continuous MIDI Clock.

MIDI Clock uses **24 F8 clock pulses per quarter note**. MiniAmused
measures these clock events and derives the current musical tempo from
them.

The selected **DIV** value then determines the rhythmic delay time.

Available divisions are:

``` text
1/4
1/8
1/8D
1/8T
1/16
1/16D
1/16T
1/32
```

This allows the repeats to become a rhythmic part of an externally
sequenced performance.

``` text
MIDI F8 CLOCK
      │
      ▼
TEMPO MEASUREMENT
      │
      ▼
STABILITY PROCESSING
      │
      ▼
DIVISION
      │
      ▼
TARGET DELAY TIME
      │
      ▼
SMOOTH DELAY TRANSITION
```

The synchronization is based on the continuously arriving MIDI Clock.
Transport messages and clock timing are treated separately, allowing the
clock to remain continuously available even when the external sequencer
is stopped.

------------------------------------------------------------------------

## Stable Delay-Time Engine

Changing a delay time abruptly while audio is passing through a delay
buffer normally causes audible discontinuities, clicks or unnatural
pitch jumps.

MiniAmused therefore does not simply move the active delay read position
instantaneously.

The delay engine uses **fixed old and new delay taps** during a time
change and performs a **24 ms crossfade** between them.

Conceptually:

``` text
OLD DELAY TAP ────────┐
                      ├──► 24 ms CROSSFADE ──► OUTPUT
NEW DELAY TAP ────────┘
```

The old delay remains valid while the new delay position becomes active.
The crossfade then transfers the output smoothly from the previous
timing to the new timing.

This mechanism is particularly important when:

-   changing TIME manually
-   changing DIV
-   enabling MIDI synchronization
-   the external tempo changes
-   the measured MIDI Clock timing is updated

The objective is simple:

> **Delay-time changes should not interrupt the musical signal.**

------------------------------------------------------------------------

## MIDI Clock Stability

A hardware MIDI Clock is not mathematically perfect. Individual F8
intervals can vary slightly because of scheduling, transmission timing
and jitter.

Using every single measured interval directly as a new delay time would
make the delay unnecessarily unstable.

MiniAmused therefore evaluates the incoming clock over **24-F8
windows**, corresponding to one quarter-note clock cycle.

The synchronization engine also uses:

-   stability checking
-   hysteresis
-   **0.5 ms delay-time quantization**

These measures prevent insignificant clock fluctuations from constantly
moving the delay tap.

The result is a delay that follows meaningful tempo changes while
avoiding unnecessary modulation caused by small MIDI timing variations.

------------------------------------------------------------------------

## Feedback

**FEEDBACK** determines how much of the delayed output is returned to
the delay input.

``` text
INPUT ─────► DELAY ─────► OUTPUT
               ▲
               │
               └── FEEDBACK
```

At low settings, only a small number of repeats are audible.

Increasing FEEDBACK creates progressively longer echo tails because each
repeat is fed back into the delay line.

Feedback therefore changes the delay from a simple single echo into a
repeating rhythmic structure.

------------------------------------------------------------------------

## Delay Filter

The delay path contains a **low-pass filter**.

Rather than every repeat being an exact copy of the original signal, the
FILTER parameter can progressively reduce high-frequency content in the
delayed signal.

This is useful for moving repeats behind the dry synthesizer sound and
creating a less clinical delay character.

The displayed FILTER value corresponds to the actual low-pass cutoff.
Representative points include approximately:

       FILTER   Approx. cutoff
  ----------- ----------------
           0%           154 Hz
          25%           637 Hz
          50%         2.32 kHz
          72%         5.84 kHz
          75%         6.66 kHz
    near open         \>24 kHz

At low settings, repeats become dark very quickly. At higher settings,
substantially more high-frequency content remains in the delay path.

------------------------------------------------------------------------

## Mix

**MIX** controls the relationship between the original synthesizer
signal and the delayed signal.

``` text
             ┌──── DRY ─────────────┐
INPUT ───────┤                      ├──► MIX ──► OUTPUT
             └──── DELAY ── WET ───┘
```

Low MIX values preserve a strong direct sound with delay in the
background.

Higher MIX values make the echo increasingly prominent.

------------------------------------------------------------------------

## Stereo Width

The **WIDTH** control determines the stereo presentation of the delay.

This is especially useful because the core MiniAmused synthesizer is
monophonic: the delay can expand the final signal spatially without
changing the fundamental one-voice synthesis architecture.

The result is a monophonic synthesizer voice that can produce a
significantly wider stereo output after the delay stage.

------------------------------------------------------------------------

# PRESET -- Preset Management

![MiniAmused PRESET](docs/MiniAmused_PRESET.jpg)

MiniAmused provides preset management for storing complete synthesizer
configurations.

The preset system allows sounds created on the touchscreen to be
recalled without reconstructing every parameter manually. The SD card
provides persistent storage and allows preset data to remain independent
of the main firmware.

------------------------------------------------------------------------

# MIDI

MiniAmused was designed as a MIDI instrument from the beginning.

Two MIDI interfaces are supported:

``` text
             ┌──── DIN MIDI
             │
MIDI ENGINE ◄┤
             │
             └──── Native USB MIDI
```

Traditional DIN MIDI allows MiniAmused to communicate with keyboards,
hardware sequencers, controllers and other instruments. Native USB MIDI
provides a direct connection to compatible computer systems without
requiring a conventional external USB-to-MIDI interface.

------------------------------------------------------------------------

# Parallel DIN + USB MIDI

DIN MIDI and USB MIDI are integrated into the MiniAmused MIDI
architecture.

This enables hybrid configurations:

``` text
Hardware Sequencer
       │
       │ DIN MIDI
       ▼
   MiniAmused
       ▲
       │ USB MIDI
       │
     Computer
```

------------------------------------------------------------------------

# MIDI Clock

MiniAmused supports external MIDI Clock synchronization, particularly
for the integrated delay.

The implementation distinguishes the MIDI real-time messages:

``` text
F8   MIDI CLOCK
FA   START
FB   CONTINUE
FC   STOP
```

Continuous F8 clock can remain present while transport control
independently determines whether the external system is running or
stopped.

This is important for MiniAmused because the synchronized delay can
continue to derive a stable tempo from the available clock independently
of transport state.

------------------------------------------------------------------------

# Real-Time DSP

The most important rule of the MiniAmused software architecture is:

> **Audio has priority.**

An embedded synthesizer cannot temporarily stop processing audio because
the display needs to redraw a control or the SD card is being accessed.

The system architecture therefore separates time-critical DSP work from
lower-priority functions.

``` text
HIGH PRIORITY

    AUDIO DSP
       │
       ▼
    I2S OUTPUT


LOWER PRIORITY

    MIDI
    TOUCH
    LVGL 8.4.0
    DISPLAY
    PRESETS
    SD CARD
    SYSTEM UI
```

The objective is deterministic audio behaviour even while the graphical
interface is active.

------------------------------------------------------------------------

# LVGL 8.4.0 Graphical Interface

The complete MiniAmused graphical user interface is based on **LVGL
8.4.0**.

LVGL provides the framework for synthesis pages, sliders, buttons,
parameter controls, labels, graphical feedback, overlays, dynamic
elements and touch interaction.

The GUI is designed specifically around the JC4827W543's **480 × 272**
display.

> **The touchscreen is the front panel of the instrument.**

------------------------------------------------------------------------

# Why LVGL 8.4.0 instead of LVGL 9?

MiniAmused deliberately uses **LVGL 8.4.0** rather than migrating the
project to LVGL 9.

This is a conscious engineering decision. The MiniAmused user interface
was developed, optimized and tested around the **LVGL 8.4.0 API**
together with the **Guition JC4827W543**, its 480 × 272 display and the
GT911 capacitive touch controller.

For a real-time musical instrument, a newer library version is not
automatically the better choice. Stability, deterministic behaviour and
a thoroughly tested hardware/software combination are more important
than using the latest major API.

## A Proven Software Baseline

The complete MiniAmused graphical architecture is based on LVGL 8.4.0,
including:

-   display initialization
-   display buffers
-   GT911 touch integration
-   input-device handling
-   screen and page management
-   sliders and parameter controls
-   labels
-   event callbacks
-   graphical updates
-   overlays
-   parameter feedback
-   interaction with the synthesizer control layer

``` text
JC4827W543
     │
     ├── 480 × 272 TFT
     │
     ├── GT911 Touch
     │
     ▼
 LVGL 8.4.0
     │
     ▼
MiniAmused UI
     │
     ▼
Parameter System
     │
     ▼
Synth Engine
```

## LVGL 9 is a Major-Version Migration

LVGL 9 is not simply a drop-in replacement for LVGL 8. Its major-version
changes affect parts of the display, rendering, driver and input
architecture.

Migrating MiniAmused would therefore mean adapting already working parts
of the user-interface infrastructure and validating the complete system
again.

For MiniAmused the relevant question is not:

> **Which LVGL version is newer?**

but:

> **Which LVGL version provides the most stable and predictable
> foundation for this instrument?**

For the current MiniAmused architecture, that version is **LVGL 8.4.0**.

## Real-Time Audio has Priority

MiniAmused is first and foremost a synthesizer.

The graphical interface shares the ESP32-S3 with time-critical
functions:

``` text
AUDIO DSP
    +
MIDI
    +
USB MIDI
    +
MIDI CLOCK
    +
STEREO DELAY
    +
TOUCH
    +
DISPLAY
```

The audio engine must continue to meet its real-time deadlines
regardless of what is happening on the display.

Once a graphical software stack has demonstrated stable interaction with
the audio system, replacing it solely to use a newer major release
offers little advantage unless the newer version provides a feature the
instrument actually requires.

MiniAmused currently does not require such a feature.

## Stability over Version Numbers

The RTAL development philosophy for embedded musical instruments is:

> **Do not replace a proven real-time subsystem without a concrete
> technical reason.**

LVGL 8.4.0 already provides everything required by MiniAmused:

-   responsive touchscreen controls
-   graphical parameter pages
-   sliders and controls
-   labels and event handling
-   dynamic graphical updates
-   overlays
-   reliable GT911 integration
-   predictable behaviour
-   proven interaction with the MiniAmused firmware

Moving to LVGL 9 purely for the sake of using the newest major version
would mean modifying a stable subsystem without directly improving the
sound or playability of the instrument.

## Could MiniAmused use LVGL 9?

Technically, the ESP32-S3 and JC4827W543 can support a modern LVGL
implementation. However, moving MiniAmused to LVGL 9 would be a
**GUI-platform migration**, not merely a library-version update.

Display initialization, buffer management, driver integration, input
handling and parts of the GUI code would need to be adapted and then
tested again together with the complete real-time audio system.

For the current MiniAmused generation there is no compelling reason to
introduce this additional complexity.

> **LVGL 8.4.0 is therefore the intentionally selected and validated
> graphical platform for RTAL MiniAmused.**

Future RTAL instruments can use newer LVGL generations when they are
designed around them from the beginning. MiniAmused remains on its
proven LVGL 8.4.0 foundation.

------------------------------------------------------------------------

# GT911 Capacitive Touch

The touchscreen uses a **GT911 capacitive touch controller**.

MiniAmused translates GT911 touch events into LVGL 8.4.0 input events,
allowing synthesis parameters to be manipulated directly on screen.

``` text
FINGER
  │
  ▼
GT911
  │
  ▼
TOUCH DRIVER
  │
  ▼
LVGL 8.4.0
  │
  ▼
PARAMETER SYSTEM
  │
  ▼
SYNTH ENGINE
```

------------------------------------------------------------------------

# SD Card Storage

The SD card provides persistent storage outside the firmware image.

It can be used for presets, sound banks, configuration, graphical
resources and system data. Keeping this information separate from
firmware simplifies development and expansion.

------------------------------------------------------------------------

# External I2S Audio

MiniAmused uses an external digital audio path.

``` text
MiniAmused DSP
      │
      ▼
     I2S
      │
      ├── BCLK
      ├── LRCK
      └── DATA
      │
      ▼
 External DAC
      │
      ▼
 Analog Audio
      │
      ▼
   OUTPUT
```

------------------------------------------------------------------------

# Software Architecture

``` text
┌─────────────────────────────────────┐
│          MiniAmused System          │
├─────────────────────────────────────┤
│            Synth Engine             │
│                 │                   │
│        Oscillator / Mixer           │
│                 │                   │
│              Filter                 │
│                 │                   │
│             Amplifier               │
│                 │                   │
│          Stereo Delay               │
│                 │                   │
│             I2S Audio               │
├─────────────────────────────────────┤
│ MIDI DIN │ USB MIDI │ MIDI CLOCK    │
├─────────────────────────────────────┤
│ Presets │ SD │ Parameter System     │
├─────────────────────────────────────┤
│ Touch │ GT911 │ LVGL 8.4.0 │ TFT    │
└─────────────────────────────────────┘
```

------------------------------------------------------------------------

# Development Philosophy

MiniAmused is developed directly on the physical hardware.

New functionality is introduced incrementally and tested as part of the
complete instrument. Particular attention is paid to sound quality,
audio stability, DSP execution time, filter and oscillator behaviour,
MIDI reliability, MIDI Clock synchronization, delay stability, USB MIDI,
touchscreen responsiveness, GUI performance, preset reliability, SD-card
behaviour, heap usage, PSRAM usage and long-term stability.

The project is not simply a DSP experiment.

It is an attempt to create a **complete standalone embedded
synthesizer**.

------------------------------------------------------------------------

# Screenshot Naming Convention

The screenshots follow the six real MiniAmused page names:

``` text
MiniAmused_OSC.jpg
MiniAmused_MIX.jpg
MiniAmused_FILTER.jpg
MiniAmused_MOD.jpg
MiniAmused_PRESET.jpg
MiniAmused_EFX.jpg
```

There are no `MiniAmused_AMP.jpg`, `MiniAmused_ENV.jpg` or
`MiniAmused_LFO.jpg` page images because those are not independent pages
in MiniAmused.

------------------------------------------------------------------------

# Suggested Repository Structure

``` text
RTAL-EAI-011-MiniAmused-Synthesizer/
│
├── README.md
├── LICENSE
│
├── Firmware/
│   └── MiniAmused/
│
├── Hardware/
│   ├── Schematics/
│   └── Wiring/
│
├── Documentation/
├── Presets/
├── Audio/
├── Video/
│
└── docs/
    ├── MiniAmused_OSC.jpg
    ├── MiniAmused_MIX.jpg
    ├── MiniAmused_FILTER.jpg
    ├── MiniAmused_MOD.jpg
    ├── MiniAmused_PRESET.jpg
    ├── MiniAmused_EFX.jpg
    ├── JC4827W543_Front.jpg
    ├── JC4827W543_Rear.jpg
    └── Minimoog_Inspiration.jpg
```

------------------------------------------------------------------------

# Project Status

**RTAL-EAI-011 MiniAmused is an active development project.**

Development focuses on the complete interaction between:

**Sound Engine + DSP + MIDI + Touch UI + Presets + Stereo Delay +
Hardware**

------------------------------------------------------------------------

# RTAL Embedded Audio Initiative

**RTAL-EAI-011 MiniAmused** is part of the **RealTimeAudioLab Embedded
Audio Initiative**.

The RTAL-EAI projects explore the use of modern embedded processors as
complete real-time musical instruments.

Research and development areas include virtual-analog synthesis,
wavetable synthesis, sample-based synthesis, granular synthesis, filter
modelling, physical modelling, vocoding, MIDI sequencing, real-time
effects, multi-processor audio systems, embedded touch interfaces and
optimized microcontroller DSP.

The emphasis is always on building systems that run on real hardware and
can be used as actual instruments.

------------------------------------------------------------------------

# Acknowledgements

RealTimeAudioLab would like to thank and acknowledge:

### Bob Moog and the Minimoog development team

For creating one of the clearest and most influential synthesizer
architectures in electronic music history.

The Minimoog remains an extraordinary example of how engineering,
interface design and musical thinking can become a single instrument.

### Moog Music

For maintaining and continuing a remarkable synthesizer heritage.

### Espressif Systems

For the ESP32-S3 platform, which provides an unusually capable
foundation for experimental embedded audio and DSP projects.

### Guition

For the JC4827W543 platform that provides the integrated ESP32-S3
touchscreen hardware used by MiniAmused.

### LVGL

For the open-source **LVGL 8.4.0** graphics framework used to create the
MiniAmused touchscreen interface.

### The Open-Source Community

For the tools, libraries, documentation and accumulated knowledge that
make complex embedded projects accessible to independent developers.

------------------------------------------------------------------------

# Disclaimer

MiniAmused is an independent experimental synthesizer developed by
**RealTimeAudioLab**.

It is **not a Minimoog clone** and does not attempt to reproduce the
original instrument at circuit level.

The Minimoog serves as an important historical and conceptual
inspiration for the project.

MiniAmused is not affiliated with, endorsed by, or sponsored by Moog
Music.

All trademarks are the property of their respective owners.

------------------------------------------------------------------------

# RealTimeAudioLab

### Experimental Embedded Audio · DSP · Synthesizers · MIDI · ESP32-S3

**RTAL-EAI-011 MiniAmused Synthesizer**

> **Classic synthesizer philosophy.\
> Modern embedded DSP.\
> One monophonic voice.\
> One complete instrument.**

------------------------------------------------------------------------

**Project:** RTAL-EAI-011\
**Instrument:** MiniAmused Synthesizer\
**Architecture:** Monophonic Virtual Analog\
**Processor:** ESP32-S3\
**Display Platform:** Guition JC4827W543\
**GUI:** LVGL 8.4.0\
**Touch:** GT911 Capacitive Touch\
**MIDI:** DIN MIDI + Native USB MIDI\
**Storage:** microSD\
**Effect:** Stereo Delay -- FREE / MIDI Clock SYNC\
**Audio:** External I2S Audio\
**Development:** RealTimeAudioLab

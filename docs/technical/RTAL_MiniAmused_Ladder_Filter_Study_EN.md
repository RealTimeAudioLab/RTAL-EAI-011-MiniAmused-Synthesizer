# RTAL MiniAmused: Ladder Filter Study and Comparison with Arturia Mini V

### White-noise FFT measurements · DSP fault isolation · Cutoff range correction · Measurements and listening validation

**Real Time Audio Lab · RTAL MiniAmused · Technical report · October 11, 2026**  
**Firmware under test:** **v1.2.6 FINAL** (stable baseline) and **v1.2.7 DEV1 — LADDER CUTOFF RANGE FIX** (experimental branch).  
**Comparison instrument:** **Arturia Mini V**, a Minimoog *software emulation*, **not an original analog Minimoog**.

> **Summary.** A structured set of white-noise FFT measurements revealed that the MiniAmused DSP cutoff calculation stopped increasing at **5,120 Hz**, even though its UI reported values up to **20 kHz**. Source inspection traced the issue to the exponent limits of an optimized lookup-table helper. A one-line change to use an existing range-extended helper restored the intended cutoff **parameter calculation** without altering the ladder topology, resonance law, or bass compensation. Follow-up measurements show a substantially more open high-frequency response and resonance movement into the upper audible range. A listening test confirmed a clearly brighter sound at open cutoff. The published stable release **v1.2.6 FINAL is unchanged**; DEV1 remains an experimental development build.

**Contents:** [Objective](#1-objective) · [Method and limitations](#2-method-and-what-the-measurements-can-prove) · [Arturia reference](#3-arturia-mini-v-reference) · [Stable baseline](#4-miniamused-v126-final-baseline) · [Code-level cause](#5-code-level-root-cause) · [DEV1 verification](#6-dev1-verification-results) · [Interpretation](#7-integrated-results) · [Conclusions](#8-engineering-conclusions) · [Archive](#appendix-a--complete-measurement-archive)

---

## 1. Objective

RTAL MiniAmused is an independent ESP32-S3-based synthesizer. Its four-stage nonlinear ladder-style low-pass filter uses global resonance feedback, **2× oversampling**, and a resonance-dependent input/body compensation curve. This study was designed to examine its behavior against a familiar *software* reference, the Arturia Mini V, without treating that reference as an absolute model of original analog hardware.

The engineering questions were straightforward: How do cutoff and emphasis affect the observed output spectra? Do equal UI values produce similar actual resonance frequencies? Does MiniAmused really open its filter as far as its display suggests? If not, can the discrepancy be corrected **without redesigning a working filter**? Finally, do the electrical/FFT observations produce an audible improvement?

A central caveat is that **equal normalized knob settings are not equal physical cutoff frequencies across different synthesizers**. This is a comparison of instruments and their parameter mappings, not a blind, frequency-matched laboratory comparison of isolated filters.

## 2. Method and what the measurements can prove

### 2.1 Test arrangement

| Factor | Arturia Mini V | RTAL MiniAmused |
|---|---|---|
| Excitation | internal WHITE noise | internal WHITE noise |
| Oscillators | OSC 1–3 off | OSC 1–3 silent |
| Filter contour | CONTOUR = 0 | CONTOUR = 0 |
| Swept controls | CUTOFF and EMPHASIS | CUTOFF and EMPHASIS |
| Other modulation | excluded | KEY TRACK OFF, FILTER MOD OFF |
| Drive and separate mixer feedback | kept neutral for intended comparison | DRIVE 0, mixer FEEDBACK 0 |
| Delay | not included in the filter measurement | OFF |
| Processing path | Mini V software audio output | ESP32-S3 / I2S, 48 kHz output, 128-frame blocks; filter path 2× oversampled |
| Recorded evidence | FFT screenshots | FFT screenshots |

The output was captured on logarithmic-frequency FFT displays with dB amplitude scales. Constant note/envelope and level conditions were targeted. The archive comprises **10 Arturia screenshots, 11 MiniAmused v1.2.6 screenshots, and 14 DEV1 screenshots: 35 images in total**.

All original screenshot bytes are included under [`filter_assets/originals/`](filter_assets/originals/), with a searchable [`measurement_index.csv`](measurement_index.csv). Presentation figures concatenate original screenshots and add captions only; they do **not** modify the underlying spectral traces.

### 2.2 Important measurement limitations

These are **output noise spectra**, not calibrated filter transfer functions. For an ideal linear time-invariant system, the power spectra satisfy `S_out(f) = |H(f)|² S_in(f)`. The noise sources of the two instruments were **different internal generators** and their pre-filter spectra were not separately measured. The filters are nonlinear, making the idealized equation only an orientation.

Consequently, the screenshots can support **qualitative spectral differences**, approximate resonance locations, obvious changes after a code fix, and trends of a growing/moving peak. They do **not** establish a precise −3 dB corner, an experimentally calibrated 24 dB/octave slope, an exact passband attenuation or bass-compensation value in dB, THD/aliasing figures, or authenticity relative to original analog hardware. Exact FFT averaging/window settings, gain calibration and numerical spectrum data were not available; none are invented in this report. Spectral details at the displayed noise floor must also be interpreted with care.

## 3. Arturia Mini V reference

The complete Mini V series was:

| CUTOFF | EMPHASIS settings | Number |
|---:|---|---:|
| 0 | 0, 64, 127 | 3 |
| 64 | 0, 64, 96, 127 | 4 |
| 127 | 0, 64, 127 | 3 |

**Closed cutoff (0).** The output is strongly attenuated. Increasing emphasis produces a pronounced low-frequency peak in the outgoing spectrum, broadly around **60–70 Hz** at maximum emphasis. This is an approximate *output peak*, not a calibrated cutoff frequency.

<details>
<summary><strong>Mini V FFT series: CUTOFF 0</strong></summary>

![Arturia Mini V CUTOFF 0](filter_assets/figures/06_arturia_cut0_series.jpg)

</details>

**Mid cutoff (64).** Resonance is increasingly prominent between emphasis 64 and 127. At high emphasis the visible peak is broadly near **2 kHz**. The rest of the output spectrum changes as well, illustrating that resonance is not simply a narrow EQ boost added after an otherwise unchanged low-pass filter.

![Arturia Mini V CUTOFF 64 comparison](filter_assets/figures/02_arturia_cut64_series.jpg)

**Open cutoff (127).** At zero emphasis the output remains relatively open into the upper audible range. Maximum emphasis produces a strong, narrow high-frequency feature near the upper visible frequency limit. The exact origin and frequency cannot be established without isolated input measurements and knowledge of the Mini V's internal processing.

<details>
<summary><strong>Mini V FFT series: CUTOFF 127</strong></summary>

![Arturia Mini V CUTOFF 127](filter_assets/figures/08_arturia_cut127_series.jpg)

</details>

**Reference observation:** cutoff changes the observed peak location over a large range, while emphasis affects spectral energy throughout the output, not solely at the peak.

## 4. MiniAmused v1.2.6 FINAL baseline

The stable baseline contributed 11 recordings: cutoff 0 at emphasis 0/64/127, cutoff 64 at emphasis 0/32/64/112/127, and cutoff 127 at emphasis 0/64/127.

At **cutoff 64** the MiniAmused UI showed approximately **615 Hz**. Its resonance peak developed in roughly the **550–650 Hz** range as emphasis increased, demonstrating an operational resonant response.

![Stable MiniAmused cutoff 64 with progressive emphasis](filter_assets/figures/03_miniamused_v126_cut64_series.jpg)

A Mini V cutoff setting of 64 yields a considerably higher resonance peak (broadly near 2 kHz). This does **not** alone imply an inferior filter design: the instruments use different knob-to-frequency mappings and processing architectures.

<details>
<summary><strong>At the same control numbers, not physically frequency matched</strong></summary>

![Arturia Mini V vs MiniAmused v1.2.6 at control value 64](filter_assets/figures/11_arturia_vs_rtal_at_same_control_value.jpg)

</details>

The principal anomaly appeared at **cutoff 127**: the UI read **20 kHz**, but the actual output spectrum was already strongly rolled off at far lower frequencies. At maximum emphasis the prominent resonance also appeared much lower than the displayed setting would suggest.

<details>
<summary><strong>Stable MiniAmused FFT series: cutoff 127</strong></summary>

![MiniAmused v1.2.6 cutoff 127](filter_assets/figures/09_miniamused_v126_cut127_series.jpg)

</details>

At minimum cutoff (0), the output remained highly attenuated. The very low-frequency peak observed in the Mini V reference was not equivalently distinct in the MiniAmused output, but these conditions do not distinguish among differences in parameter mapping, generator spectra, filtering and measurement floor. No change was made on the basis of this low-frequency observation.

<details>
<summary><strong>Stable MiniAmused FFT series: cutoff 0</strong></summary>

![MiniAmused v1.2.6 cutoff 0](filter_assets/figures/07_miniamused_v126_cut0_series.jpg)

</details>

## 5. Code-level root cause

### 5.1 Intended logarithmic mapping

The intended base cutoff parameter conversion in `SynthEngine.cpp` is:

```text
f_cutoff(n) = 20 Hz × 2^(9.965784 × n),  n ∈ [0, 1]
```

This maps a normalized control from about **20 Hz** to **20 kHz**. The v1.2.6 code, however, called:

```cpp
static inline float cutoffFromNorm(float n)
{
    return 20.0f * fastExp2(9.965784f * n);
}
```

`fastExp2()` is a fast, lookup-table-interpolated helper with an allowed exponent domain of **−8 … +8**. Inputs larger than +8 are clamped to +8. Therefore its largest output here is `2^8 = 256`, yielding:

```text
maximum requested base cutoff = 20 × 256 = 5,120 Hz
```

The cutoff mapping saturates at approximately `n = 8 / 9.965784 = 0.803`, or **80% of the normalized control travel**. The UI used a separate calculation and continued displaying a full-scale **20 kHz**, concealing the DSP discrepancy.

**This is a code-verified root cause**, not an inference from the plots alone.

### 5.2 Minimal DEV1 correction

The firmware already had a range-extended helper, `fastExp2Envelope()`, originally used to support longer envelope time constants. DEV1 substituted this existing helper at precisely the problematic call site:

```diff
 static inline float cutoffFromNorm(float n)
 {
-    return 20.0f * fastExp2(9.965784f * n);
+    return 20.0f * fastExp2Envelope(9.965784f * n);
 }
```

The requested cutoff value can now span approximately **20 Hz … 20 kHz** without introducing a new `powf()`/`expf()` evaluation in the time-critical render loop.

**An independent safeguard remains:** `ladderG()` clamps the coefficient-design input frequency at **18 kHz**. Thus a 20-kHz screen value must **not** be described as an experimentally verified −3-dB corner at 20 kHz. The nonlinear ladder and numerical coefficient mapping also influence the measured curve.

| Property | v1.2.6 FINAL | v1.2.7 DEV1 |
|---|---|---|
| Maximum cutoff displayed | 20 kHz | 20 kHz |
| Base cutoff computed | clamps to **5.12 kHz** | allows **20 kHz** |
| Additional coefficient guard | 18 kHz | unchanged, **18 kHz** |
| Four-pole nonlinear ladder | existing | unchanged |
| Resonance law | existing | unchanged |
| Resonance-dependent body compensation | existing | unchanged |
| Measured open-cutoff output | limited high frequencies | much broader spectrum |

The synthesis topology, four nonlinear stages, 2× oversampling, resonance implementation, bass/body compensation, UI, MIDI, presets and other functional systems were not deliberately modified by this single-fix experiment.

## 6. DEV1 verification results

### 6.1 Before/after at CUTOFF 127, EMPHASIS 0

This is the clearest visual check. Both traces use the same displayed settings; DEV1 leaves high-frequency noise substantially less attenuated than v1.2.6.

![MiniAmused v1.2.6 vs v1.2.7 DEV1, CUTOFF 127 EMPHASIS 0](filter_assets/figures/01_cutoff127_before_after.jpg)

The aligned conclusion from code and FFT is that the cutoff-range correction works. This does not assign a numerically calibrated physical corner frequency.

An initial DEV1 batch also contained cutoff settings 64, 96, 102 and 110 at zero emphasis. **Two screenshots were accidentally exchanged in the original handoff**, as the experimenter later confirmed. To avoid erroneous numerical labels, the main conclusions here rely on the unambiguous before/after and resonance sequences. All files are retained in the archive with their received names; the measurement index flags the ambiguous initial naming.

### 6.2 Emphasis sweep around 7.5–8 kHz

At the high cutoff setting labelled **110 / approximately 7.51 kHz**, emphasis was stepped through **0, 64, 96 and 127**. A growing resonance feature becomes clearly visible around **8 kHz** at high emphasis.

![MiniAmused DEV1 cutoff 110 emphasis series](filter_assets/figures/04_miniamused_dev1_cut110_series.jpg)

This demonstrates that the fix does more than pass additional treble: resonance tracking remains effective above the old 5.12-kHz parameter limit.

### 6.3 Upper-range sweep at maximum emphasis

| CUTOFF control | UI frequency in filename | Qualitative observation |
|---:|---:|---|
| 114 | 9.34 kHz | pronounced rise around 9–10 kHz |
| 118 | 12.26 kHz | feature shifts toward 12–13 kHz |
| 122 | 15.24 kHz | feature moves into the upper treble |
| 125 | 17.94 kHz | peak no longer wholly resolved within visible range |

![MiniAmused DEV1 resonance location vs high cutoff setting](filter_assets/figures/05_miniamused_dev1_upper_resonance.jpg)

The resonant feature moves **progressively upward** in these screenshots. At the highest cutoff settings the peak may reach the plot boundary or be reshaped by the numerical filter and output band limit. Neither instability nor aliasing has been established by these white-noise FFT images.

### 6.4 Highest setting: CUTOFF 127

At cutoff 127, the emphasis 0/64/127 spectra remain comparatively open. Unlike the Arturia reference at its own maximum cutoff, a similarly narrow, strong extreme-high-frequency peak is **not** evident in the DEV1 visible spectrum.

![MiniAmused DEV1 cutoff 127 resonance series](filter_assets/figures/10_miniamused_dev1_cut127_series.jpg)

This difference by itself is not an actionable defect. It may involve dissimilar frequency mappings, an upper-range peak shifted past the plot's practical resolution, the **18-kHz coefficient guard**, the digital integrator, and different generator and output chains. The experiment does not justify forcing a maximum 20-kHz resonance spike.

<details>
<summary><strong>Arturia Mini V vs MiniAmused DEV1 at maximum displayed controls</strong></summary>

![Arturia Mini V vs RTAL DEV1 at control 127/127](filter_assets/figures/12_arturia_vs_rtal_dev1_cut127.jpg)

</details>

### 6.5 Listening verification

After the FFT tests, the experimenter directly compared the sound with the filter wide open, particularly using a **sawtooth waveform**. The report from the developer was unambiguous: **DEV1 sounds noticeably brighter and more open than v1.2.6**.

This is a **subjective listening result**, but an important musical acceptance check and fully consistent with the direction of the FFT data. It marked the completion of the focused cutoff-range investigation.

## 7. Integrated results

| Question | Finding | Evidence |
|---|---|---|
| Why did the old cutoff stop opening? | `fastExp2()` clamps to exponent +8 and yields a 5,120-Hz base-cutoff ceiling | **Source code: confirmed** |
| Did the DEV1 code change work? | Extended parameter range and much more open treble output | **Code + FFT: strong** |
| Does resonance still work at high cutoff? | Upper-spectrum resonance rises and tracks increasing cutoff through much of the high range | **FFT: qualitative** |
| Is the measured −3-dB frequency 20 kHz? | Not measured; 18-kHz coefficient guard remains | **Not established** |
| Is the slope precisely 24 dB/octave? | Not measured under calibrated transfer-function conditions | **Not established** |
| Did the bass compensation improve? | Compensation was **unchanged**, isolated effect not quantified | **Code confirmed; dB effect unmeasured** |
| Is it identical to Mini V or an analog Minimoog? | No such claim follows from this setup | **Not established** |
| Is the improvement audible? | Developer explicitly confirmed more treble/brilliance | **Listening validation: confirmed** |

### 7.1 Bass/body compensation in context

The MiniAmused ladder contains resonance-dependent compensation **inside the nonlinear input path**, not a parallel unfiltered dry signal:

```cpp
const float bassComp = (r > 0.20f) ? (r - 0.20f) : 0.0f;
const float body = 1.0f + 0.08f * r
                 + 1.15f * bassComp
                 + 0.35f * bassComp * bassComp;
```

Here `r` is normalized resonance. This expression adjusts the **input drive of the nonlinear ladder** as resonance increases; it is not a frequency-specific bass EQ with a fixed gain. The plots alone cannot isolate its effect in dB. DEV1 did **not** alter it, and these measurements provide no sufficient reason to retune it.

### 7.2 Upper-band digital behavior

MiniAmused outputs at **48 kHz** and processes the ladder at **2×** that rate. `ladderG()` limits the coefficient-design frequency to 18 kHz. After the oversampled calculations, the code uses a **simple average of two samples** for downsampling rather than a full half-band anti-alias filter. These implementation details are relevant near the top of the audible band, but the present screenshots are **not an aliasing test**, and they establish no specific aliasing defect.

### 7.3 What a more controlled comparison would require

To compare the isolated filter responses quantitatively, use the **same measured input stimulus**, calibrated levels and FFT conditions, observe pre/post-filter spectra separately, and match the **physical cutoff or resonance frequency** rather than the nominal control value. The existing results are excellent for locating the coding issue and validating the targeted fix, but not sufficient for full model-to-model transfer-function identification.

## 8. Engineering conclusions

1. **Root cause identified at the parameter-to-DSP interface.** The main weakness was not a fundamentally unsuitable ladder topology; it was a numerically clamped exponential lookup.
2. **A one-line change was enough.** Existing DSP structures, nonlinear pole processing, resonance feedback, envelope behavior and bass compensation remained intact.
3. **Measurement and listening agree.** Both the high-frequency FFT response and the developer's direct listening test support the expected increase in brightness.
4. **Resonance is available over a wider range.** The measured peaks move to approximately 8–15+ kHz after the correction; reproducing an extreme narrow 20-kHz spike is not a meaningful standalone target.
5. **No speculative redesign.** The existing resonant ladder and body compensation remain suitable pending a different, properly isolated test question.
6. **Arturia is a reference, not a specification.** MiniAmused is intentionally an independent RTAL instrument rather than a component-by-component analog replica.
7. **Keep stable and experimental branches distinct.** The officially stable firmware is still **v1.2.6 FINAL**. **v1.2.7 DEV1** is a tested development branch for the cutoff-range investigation, not yet a declared final release.

**Final assessment:** The investigation is complete for its defined goal. Starting from a spectral mismatch, the team obtained consistent **FFT evidence, a code-level root cause, an isolated correction, post-change spectral verification and audible confirmation**. No exact calibrated transfer function, aliasing value, 24-dB/octave measurement or proof of analog-model equivalence is claimed.

> **Engineering lesson:** A physically plausible value shown on screen does not guarantee that the DSP actually receives that value. Comparing interface mapping, numerical helpers, processing coefficients, FFT results and audible output is essential.

---

## Appendix A — Complete measurement archive

The package contains **35 unmodified screenshots** under [`filter_assets/originals/`](filter_assets/originals/) and [`measurement_index.csv`](measurement_index.csv) recording the control labels and original filenames.

| Platform | Measurement settings | Images |
|---|---|---:|
| **Arturia Mini V** | CUT 0: EMP 0/64/127; CUT 64: EMP 0/64/96/127; CUT 127: EMP 0/64/127 | 10 |
| **MiniAmused v1.2.6 FINAL** | CUT 0: EMP 0/64/127; CUT 64: EMP 0/32/64/112/127; CUT 127: EMP 0/64/127 | 11 |
| **MiniAmused v1.2.7 DEV1** | CUT 64/96/102/110/127: EMP 0; CUT 110: EMP 64/96/127; CUT 127: EMP 64/127; CUT 114/118/122/125: EMP 127 | 14 |
| **Total** | | **35** |

The 12 figure composites only add explanatory headers to original screenshots. They contain **no artificially generated frequency curves**.

## Appendix B — Firmware references

Relevant archive filenames:

- `RTAL_MINIAMUSED_1_2_6_FINAL.zip`: validated, unchanged stable baseline.
- `RTAL_MINIAMUSED_1_2_7_DEV1_LADDER_CUTOFF_RANGE_FIX.zip`: experimental isolated cutoff-range change.

Relevant `SynthEngine.cpp` functions: `fastExp2()`, `fastExp2Envelope()`, `cutoffFromNorm()`, `ladderG()`, and `Ladder::process()`.

**GitHub installation:** Keep this Markdown file **and the entire `filter_assets/` directory** together under `docs/technical/`, or its relative image links will break.

---

*© 2026 Real Time Audio Lab · Independent RTAL MiniAmused engineering study. Arturia Mini V is identified solely as a comparison product; no affiliation with or endorsement by Arturia or Moog is implied.*

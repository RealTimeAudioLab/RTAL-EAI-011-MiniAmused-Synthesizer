#include "SynthEngine.h"
#include "RTAL_IO_PINS.h"
#include "Parameters.h"
#include "DelayEngine.h"

#include "driver/i2s.h"
#include <math.h>

namespace {

DelayEngine sDelay;

constexpr i2s_port_t I2S_PORT = I2S_OUTPUT_NUM;
constexpr float SR = (float)RTAL_SAMPLE_RATE;
constexpr float OSR = 2.0f;
constexpr float SR_OS = SR * OSR;

// ---------- Fast lookup tables ----------
constexpr int LUT_N = 1024;
float sExp2Lut[LUT_N + 1];
float sTanhLut[LUT_N + 1];
bool sLutReady = false;

static void buildLUTs()
{
    if (sLutReady) return;

    for (int i = 0; i <= LUT_N; ++i) {
        const float x = (float)i / (float)LUT_N;

        // exp2 range: [-8, +8] mapped to 0..1
        const float e = -8.0f + 16.0f * x;
        sExp2Lut[i] = powf(2.0f, e);

        // tanh range: [0, 4]
        const float t = 4.0f * x;
        sTanhLut[i] = tanhf(t);
    }

    sLutReady = true;
}

static inline float lutInterp(const float *lut, float x01)
{
    if (x01 <= 0.0f) return lut[0];
    if (x01 >= 1.0f) return lut[LUT_N];

    float p = x01 * LUT_N;
    int i = (int)p;
    float f = p - (float)i;
    return lut[i] + (lut[i + 1] - lut[i]) * f;
}

static inline float fastExp2(float x)
{
    if (x <= -8.0f) return sExp2Lut[0];
    if (x >=  8.0f) return sExp2Lut[LUT_N];
    return lutInterp(sExp2Lut, (x + 8.0f) * (1.0f / 16.0f));
}

// v0.7.2b: Envelope timing needs exp2 up to ~9.97; the general LUT helper
// clamps at +8. Range reduction extends it without powf/expf.
static inline float fastExp2Envelope(float x)
{
    if (x <= 8.0f) return fastExp2(x);
    return 256.0f * fastExp2(x - 8.0f);
}

static inline float fastTanh(float x)
{
    float ax = fabsf(x);
    if (ax >= 4.0f) return x < 0.0f ? -0.9993293f : 0.9993293f;
    float y = lutInterp(sTanhLut, ax * 0.25f);
    return x < 0.0f ? -y : y;
}

// ---------- Oscillator ----------
struct Osc {
    float phase = 0.0f;

    static inline float polyBlep(float t, float dt)
    {
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0f;
        }

        if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }

        return 0.0f;
    }

    // v0.5.6a1: conservative oscillator output calibration.
    // Do not RMS-normalise every waveform; preserve useful analogue-style
    // level differences while avoiding excessive mixer loading.
    static inline float waveGain(int wave)
    {
        switch (wave) {
            case 0: return 1.06f; // triangle
            case 1: return 1.02f; // shark / tri-saw
            case 2: return 1.00f; // saw = reference
            case 3: return 0.96f; // saw-square
            case 4: return 0.86f; // square
            default:return 0.90f; // narrow pulse
        }
    }

    inline float process(float hz, int wave, float sr)
    {
        float dt = hz / sr;
        if (dt > 0.45f) dt = 0.45f;

        const float t = phase;
        float y;

        switch (wave) {
            case 0: // triangle
                y = 1.0f - 4.0f * fabsf(t - 0.5f);
                break;

            case 1: { // shark / tri-saw
                const float tri = 1.0f - 4.0f * fabsf(t - 0.5f);
                float saw = 2.0f * t - 1.0f;
                saw -= polyBlep(t, dt);
                y = 0.62f * tri + 0.38f * saw;
                break;
            }

            case 2: // saw
                y = 2.0f * t - 1.0f;
                y -= polyBlep(t, dt);
                break;

            case 3: { // saw-square
                float saw = 2.0f * t - 1.0f;
                saw -= polyBlep(t, dt);

                float sq = (t < 0.5f) ? 1.0f : -1.0f;
                sq += polyBlep(t, dt);
                float t2 = t + 0.5f;
                if (t2 >= 1.0f) t2 -= 1.0f;
                sq -= polyBlep(t2, dt);

                y = 0.5f * (saw + sq);
                break;
            }

            case 4: { // square
                y = (t < 0.5f) ? 1.0f : -1.0f;
                y += polyBlep(t, dt);
                float t2 = t + 0.5f;
                if (t2 >= 1.0f) t2 -= 1.0f;
                y -= polyBlep(t2, dt);
                break;
            }

            default: { // narrow pulse
                constexpr float pw = 0.25f;
                y = (t < pw) ? 1.0f : -1.0f;
                y += polyBlep(t, dt);
                float t2 = t + (1.0f - pw);
                if (t2 >= 1.0f) t2 -= 1.0f;
                y -= polyBlep(t2, dt);
                break;
            }
        }

        phase += dt;
        if (phase >= 1.0f) phase -= 1.0f;

        return y * waveGain(wave);
    }
};

// ---------- Envelope ----------
struct Envelope {
    enum Stage { IDLE, ATTACK, DECAY, SUSTAIN, RELEASE } stage = IDLE;
    float value = 0.0f;

    void gateOn()  { stage = ATTACK; }
    void gateOff() { if (stage != IDLE) stage = RELEASE; }

    inline float process(float aInc, float dCoeff, float sustain, float rCoeff)
    {
        switch (stage) {
            case ATTACK:
                // v0.5.6a4: analog-style RC rise instead of a linear ramp.
                value += (1.0f - value) * aInc;
                if (value >= 0.999f) {
                    value = 1.0f;
                    stage = DECAY;
                }
                break;

            case DECAY:
                value += (sustain - value) * dCoeff;
                if (fabsf(value - sustain) < 0.0005f) {
                    value = sustain;
                    stage = SUSTAIN;
                }
                break;

            case SUSTAIN:
                value = sustain;
                break;

            case RELEASE:
                value += (0.0f - value) * rCoeff;
                if (value < 0.00005f) {
                    value = 0.0f;
                    stage = IDLE;
                }
                break;

            default:
                value = 0.0f;
                break;
        }

        return value;
    }
};

// ---------- RTAL nonlinear four-pole ladder v0.5.6a2c ----------
// Architecture study inspired by the published Huovilainen ladder principles:
// four nonlinear one-pole sections, global negative feedback, 2x oversampling
// and a half-oversample feedback delay. This is an independent RTAL
// implementation and does not copy the LGPL implementation from MoogLadders.
struct Ladder {
    float s1 = 0.0f;
    float s2 = 0.0f;
    float s3 = 0.0f;
    float s4 = 0.0f;

    // Previous final-stage value. Together with s4 this forms the
    // half-oversample feedback tap used for phase compensation.
    float z4 = 0.0f;

    inline float process(float x, float g, float resonance)
    {
        const float r = resonance;

        // The previous ladder used g directly. For a cascade expressed as
        // dy/dt = tanh(input)-tanh(state), that was too lossy around the loop.
        // Convert the bilinear one-pole coefficient to a stronger integration
        // step while keeping it bounded at high cutoff.
        float a = 2.0f * g;
        if (a > 0.92f) a = 0.92f;

        // Half-oversample feedback tap. This reduces the excess phase error
        // that made regeneration difficult in the previous implementation.
        const float fbTap = 0.5f * (s4 + z4);
        z4 = s4;

        // Frequency-dependent loop compensation. Digital cascaded poles lose
        // loop gain progressively toward high cutoff. This mild RTAL curve
        // restores part of it without using the coefficient fit from the
        // reference implementation.
        const float gn = (g < 0.5f) ? (g * 2.0f) : 1.0f;
        const float loopComp = 1.0f + 0.38f * gn + 0.10f * gn * gn;

        // Resonance law: useful lower/mid travel, then enough loop gain for
        // genuine regeneration in the upper range. The nonlinear stages limit
        // the amplitude naturally once oscillation has started.
        const float r2 = r * r;
        const float resGain = (4.05f * r + 0.55f * r2) * loopComp;

        // Keep a little body at high emphasis, but retain the characteristic
        // ladder bass loss.
        const float body = 1.0f + 0.08f * r;

        // Nonlinearity is present in every pole, not only at the ladder input.
        const float u0 = fastTanh(x * body - resGain * fbTap);
        const float q1 = fastTanh(s1);
        s1 += a * (u0 - q1);

        const float u1 = fastTanh(s1);
        const float q2 = fastTanh(s2);
        s2 += a * (u1 - q2);

        const float u2 = fastTanh(s2);
        const float q3 = fastTanh(s3);
        s3 += a * (u2 - q3);

        const float u3 = fastTanh(s3);
        const float q4 = fastTanh(s4);
        s4 += a * (u3 - q4);

        // Guard only against numerical corruption; normal saturation is
        // performed by the nonlinear stages themselves.
        if (!(s4 == s4) || fabsf(s4) > 8.0f) {
            s1 = s2 = s3 = s4 = z4 = 0.0f;
            return 0.0f;
        }

        return s4;
    }
};

Osc o1, o2, o3;
Envelope ampEnv, filtEnv;
Ladder ladder;

bool lastGate = false;
// v0.5.6a6: glide operates in keyboard pitch-CV space (semitones), not Hz.
float glideNote = 60.0f;
bool glideInitialized = false;
float prevOversampleIn = 0.0f;
uint32_t noiseState = 0x13579BDFu;
// v0.5.6a9: lightweight multi-pole pink-noise state.
// The former single low-pass state sounded more like dark/brown noise than pink.
float pinkB0 = 0.0f;
float pinkB1 = 0.0f;
float pinkB2 = 0.0f;
float osc3ModPhase = 0.0f;

static inline float whiteNoise()
{
    noiseState ^= noiseState << 13;
    noiseState ^= noiseState >> 17;
    noiseState ^= noiseState << 5;
    return ((noiseState & 0x00FFFFFFu) * (2.0f / 16777215.0f)) - 1.0f;
}

static inline float noiseSample(float color)
{
    const float w = whiteNoise();

    // v0.5.6a9 Noise Calibration:
    // compact three-state pink approximation.  Unlike the old one-pole
    // low-pass, this distributes energy over several time constants and gives
    // a much more useful pink/dark noise character without expensive DSP.
    pinkB0 = 0.99765f * pinkB0 + w * 0.0990460f;
    pinkB1 = 0.96300f * pinkB1 + w * 0.2965164f;
    pinkB2 = 0.57000f * pinkB2 + w * 1.0526913f;
    float pink = (pinkB0 + pinkB1 + pinkB2 + w * 0.1848f) * 0.23f;

    // Equal-power-ish crossfade keeps the perceived source level much more
    // consistent while COLOR moves from WHITE to PINK.  No sqrtf in render.
    const float c = color;
    const float whiteGain = 1.0f - 0.50f * c;
    const float pinkGain  = 0.50f + 0.50f * c;
    return w * (1.0f - c) * whiteGain + pink * c * pinkGain * 1.85f;
}

static inline float noteHz(float note)
{
    return 440.0f * fastExp2((note - 69.0f) * (1.0f / 12.0f));
}

static inline int quant6(float n)
{
    int v = (int)(n * 6.0f);
    if (v < 0) v = 0;
    if (v > 5) v = 5;
    return v;
}

static inline float rangeFactor(float n)
{
    int r = (int)(n * 5.0f);
    if (r < 0) r = 0;
    if (r > 4) r = 4;

    static const float factors[5] = {
        0.25f, 0.5f, 1.0f, 2.0f, 4.0f
    };

    return factors[r];
}

static inline float cutoffFromNorm(float n)
{
    // 20 Hz .. 20 kHz = 20 * 2^(~9.96578*n)
    return 20.0f * fastExp2(9.965784f * n);
}

static inline float tuneFactor(float n)
{
    // ±7 semitones
    const float semis = (n - 0.5f) * 14.0f;
    return fastExp2(semis * (1.0f / 12.0f));
}

static inline float normTimeSeconds(float n)
{
    // ~1 ms .. 10 s via 2^13.2877 range
    return 0.001f * fastExp2(13.287712f * n);
}

struct BlockParams {
    float master;
    float masterTuneSemis;
    float glideCoeff;

    float osc1Level;
    float osc2Level;
    float osc3Level;
    float osc1Range;
    float osc2Range;
    float osc3Range;
    float osc2Tune;
    float osc3Tune;
    int osc1Wave;
    int osc2Wave;
    int osc3Wave;

    float mixerDrive;
    float noiseLevel;
    float noiseColor;
    float modWheel;
    float modMix;
    bool oscMod;
    bool filterMod;
    bool osc3Keyboard;
    bool decayEnable;
    float keyTrack;
    float feedback;

    float cutoffHz;
    float resonance;
    float contour;

    float filtAttackInc;
    float filtDecayCoeff;
    float filtSustain;
    float filtReleaseCoeff;

    float ampAttackInc;
    float ampDecayCoeff;
    float ampSustain;
    float ampReleaseCoeff;
};

static inline float coeffFromSeconds(float sec, float sr)
{
    if (sec <= 0.00001f) return 1.0f;
    // Stable approximation for 1-exp(-1/x), good for envelope smoothing.
    const float x = 1.0f / (sec * sr);
    return x / (1.0f + x);
}

// v0.5.6a4 filter-contour coefficient.
// The front-panel time is treated as the practical 0.1% settling time.
// A one-pole RC contour reaches 99.9% after about 6.9 time constants.
static inline float contourCoeffFromSeconds(float sec, float sr)
{
    if (sec <= 0.00001f) return 1.0f;
    const float tau = sec * (1.0f / 6.907755f);
    const float x = 1.0f / (tau * sr);
    return x / (1.0f + x);
}

// Original Model-D filter contour timing range: approximately 10 ms .. 10 s.
// Logarithmic mapping gives useful resolution over the full knob travel.
static inline float filterContourTimeSeconds(float n)
{
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return 0.010f * fastExp2Envelope(9.965784f * n); // 10 ms .. ~10 s
}

static inline float loudnessContourTimeSeconds(float n)
{
    if (n < 0.0f) n = 0.0f;
    if (n > 1.0f) n = 1.0f;
    return 0.010f * fastExp2Envelope(9.965784f * n); // 10 ms .. ~10 s
}

static BlockParams prepareBlock(Parameters &p)
{
    BlockParams b{};

    // Continuous parameters smoothed ONCE PER BLOCK.
    b.master = p.smooth(ParameterID::MASTER_VOLUME, 0.20f);
    // v0.5.7: global TUNE control. Center is exact concert pitch;
    // full travel is +/-12 semitones. Applied in pitch-CV space so all
    // keyboard-controlled oscillators move together without changing glide.
    b.masterTuneSemis = (p.smooth(ParameterID::MASTER_TUNE, 0.20f) - 0.5f) * 24.0f;

    // v0.5.6a6 Glide calibration.
    // The classic keyboard glide is a proportional slew in pitch-CV space.
    // Keep the proven useful 2 ms .. ~2.0 s control span, logarithmically
    // distributed, but calibrate the RC coefficient as a practical 99.9%
    // settling time rather than the old single-time-constant interpretation.
    float glideN = p.smooth(ParameterID::GLIDE_TIME, 0.20f);
    float glideSec = 0.002f * fastExp2(9.965784f * glideN);
    b.glideCoeff = contourCoeffFromSeconds(glideSec, SR);
    if (p.target(ParameterID::GLIDE_ENABLE) < 0.5f)
        b.glideCoeff = 1.0f;

    b.osc1Level = p.smooth(ParameterID::OSC1_LEVEL, 0.20f);
    b.osc2Level = p.smooth(ParameterID::OSC2_LEVEL, 0.20f);
    b.osc3Level = p.smooth(ParameterID::OSC3_LEVEL, 0.20f);

    b.osc1Range = rangeFactor(p.target(ParameterID::OSC1_RANGE));
    b.osc2Range = rangeFactor(p.target(ParameterID::OSC2_RANGE));
    b.osc3Range = rangeFactor(p.target(ParameterID::OSC3_RANGE));

    b.osc2Tune = tuneFactor(p.smooth(ParameterID::OSC2_TUNE, 0.20f));
    b.osc3Tune = tuneFactor(p.smooth(ParameterID::OSC3_TUNE, 0.20f));

    b.osc1Wave = quant6(p.target(ParameterID::OSC1_WAVE));
    b.osc2Wave = quant6(p.target(ParameterID::OSC2_WAVE));
    b.osc3Wave = quant6(p.target(ParameterID::OSC3_WAVE));

    {
        const float driveN = p.smooth(ParameterID::MIXER_DRIVE, 0.20f);
        // Finer resolution in the useful low/mid overdrive range.
        b.mixerDrive = 1.0f + 4.0f * driveN * driveN;
    }
    b.noiseLevel = p.smooth(ParameterID::NOISE_LEVEL, 0.20f);
    b.noiseColor = p.smooth(ParameterID::NOISE_COLOR, 0.20f);
    b.modWheel = p.smooth(ParameterID::MOD_WHEEL, 0.20f);
    b.modMix = p.smooth(ParameterID::MOD_MIX, 0.20f);
    b.oscMod = p.target(ParameterID::OSC_MOD_ENABLE) >= 0.5f;
    b.filterMod = p.target(ParameterID::FILTER_MOD_ENABLE) >= 0.5f;
    b.osc3Keyboard = p.target(ParameterID::OSC3_KEYBOARD_CONTROL) >= 0.5f;
    b.decayEnable = p.target(ParameterID::DECAY_ENABLE) >= 0.5f;
    // v0.5.6a3: Model-D keyboard control states: OFF, 1/3, 2/3, FULL.
    // Quantization here also makes MIDI CC106 follow the same four states.
    {
        const float kt = p.target(ParameterID::FILTER_KEYTRACK);
        if      (kt < (1.0f/6.0f)) b.keyTrack = 0.0f;
        else if (kt < 0.5f)         b.keyTrack = 1.0f/3.0f;
        else if (kt < (5.0f/6.0f))  b.keyTrack = 2.0f/3.0f;
        else                        b.keyTrack = 1.0f;
    }
    {
        const float fbN = p.smooth(ParameterID::FEEDBACK_LEVEL, 0.20f);
        // v0.5.6a10: external-input style feedback calibration.
        // Squared law gives fine control over the useful low/mid range while
        // retaining a strong upper range for classic overloaded feedback.
        b.feedback = 1.15f * fbN * fbN;
    }

    const float cutoffN = p.smooth(ParameterID::FILTER_CUTOFF, 0.20f);
    b.cutoffHz = cutoffFromNorm(cutoffN);
    b.resonance = p.smooth(ParameterID::FILTER_RESONANCE, 0.20f);

    // Five-octave maximum remains available, with finer low/mid control.
    {
        const float c = p.smooth(ParameterID::FILTER_CONTOUR, 0.20f);
        b.contour = c * (0.40f + 0.60f * c);
    }

    float t;

    // v0.5.6a4 Filter Contour calibration.
    // Attack/Decay/Release use the Model-D-like 10 ms .. 10 s time range.
    // Sustain remains a direct 0..100% contour level.
    t = filterContourTimeSeconds(p.target(ParameterID::FILTER_ATTACK));
    b.filtAttackInc = contourCoeffFromSeconds(t, SR);
    b.filtDecayCoeff = contourCoeffFromSeconds(
        filterContourTimeSeconds(p.target(ParameterID::FILTER_DECAY)), SR);
    b.filtSustain = p.target(ParameterID::FILTER_SUSTAIN);
    b.filtReleaseCoeff = contourCoeffFromSeconds(
        filterContourTimeSeconds(p.target(ParameterID::FILTER_RELEASE)), SR);

    // v0.5.6a5 Loudness Contour / VCA envelope calibration.
    t = loudnessContourTimeSeconds(p.target(ParameterID::AMP_ATTACK));
    b.ampAttackInc = contourCoeffFromSeconds(t, SR);
    b.ampDecayCoeff = contourCoeffFromSeconds(
        loudnessContourTimeSeconds(p.target(ParameterID::AMP_DECAY)), SR);
    b.ampSustain = p.target(ParameterID::AMP_SUSTAIN);
    b.ampReleaseCoeff = contourCoeffFromSeconds(
        loudnessContourTimeSeconds(p.target(ParameterID::AMP_RELEASE)), SR);

    return b;
}

static inline float ladderG(float cutoffHz)
{
    // Bilinear-ish one-pole coefficient. Computed per sample only because
    // contour can move cutoff with the envelope.
    float fc = cutoffHz;
    if (fc < 20.0f) fc = 20.0f;
    // Keep the nonlinear four-pole ladder away from the extreme Nyquist
    // region of the 2x oversampled path.
    if (fc > 18000.0f) fc = 18000.0f;

    // tan(pi*fc/fs) approximation via rational form for modest normalized fc.
    // At 2x OSR this remains stable and inexpensive.
    const float x = 3.14159265358979323846f * fc / SR_OS;
    const float x2 = x * x;
    const float tanx = x * (1.0f + 0.3333333f * x2) /
                       (1.0f - 0.1333333f * x2);
    return tanx / (1.0f + tanx);
}

} // namespace

SynthEngine& SynthEngine::instance()
{
    static SynthEngine s;
    return s;
}

bool SynthEngine::begin()
{
    Serial.printf("[I2S 01] build LUTs heap=%u\n", ESP.getFreeHeap());
    buildLUTs();
    Serial.printf("[I2S 02] LUTs ready heap=%u\n", ESP.getFreeHeap());

    const i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = RTAL_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = RTAL_DMA_BUF_COUNT,
        .dma_buf_len = RTAL_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    const i2s_pin_config_t pins = {
        .mck_io_num = I2S_MCLK,
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRCK,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    Serial.printf("[I2S 03] install port=%d sr=%d dma=%dx%d\n",
                  (int)I2S_PORT,
                  (int)RTAL_SAMPLE_RATE,
                  (int)RTAL_DMA_BUF_COUNT,
                  (int)RTAL_DMA_BUF_LEN);

    esp_err_t e = i2s_driver_install(I2S_PORT, &cfg, 0, nullptr);
    if (e != ESP_OK) {
        Serial.printf("i2s_driver_install failed: %d\n", (int)e);
        return false;
    }

    Serial.printf("[I2S 04] driver OK; pins BCLK=%d LRCK=%d DOUT=%d\n",
                  I2S_BCLK, I2S_LRCK, I2S_DOUT);

    e = i2s_set_pin(I2S_PORT, &pins);
    if (e != ESP_OK) {
        Serial.printf("i2s_set_pin failed: %d\n", (int)e);
        return false;
    }

    Serial.println("[I2S 05] pins OK");
    i2s_zero_dma_buffer(I2S_PORT);

    _audioBlocks = 0;
    _audioWriteErrors = 0;
    _audioShortWrites = 0;

    Serial.println("[I2S 06] DMA zeroed / ready");
    if(!sDelay.begin()) {
        Serial.println("[DELAY] FATAL PSRAM allocation failed");
        return false;
    }
    return true;
}

void SynthEngine::selectHeldNote()
{
    Parameters &p = Parameters::instance();
    const float mode = p.target(ParameterID::NOTE_PRIORITY);

    int selected = -1;

    if (mode < 0.333f) { // LOW
        for (int n = 0; n < 128; ++n)
            if (_heldCount[n]) { selected = n; break; }
    } else if (mode > 0.666f) { // HIGH
        for (int n = 127; n >= 0; --n)
            if (_heldCount[n]) { selected = n; break; }
    } else { // LAST
        uint32_t newest = 0;
        for (int n = 0; n < 128; ++n) {
            if (_heldCount[n] && _age[n] >= newest) {
                newest = _age[n];
                selected = n;
            }
        }
    }

    if (selected >= 0) {
        _note = selected;
        _gate = true;
    } else {
        _gate = false;
    }
}

void SynthEngine::noteOn(uint8_t note, uint8_t velocity)
{
    const bool wasGate = _gate;
    // v0.5.6a12: count repeated Note-Ons for the same pitch. This keeps the
    // monophonic held-note stack correct when overlapping/repeated MIDI events
    // arrive (including parallel DIN/USB paths) until matching Note-Offs occur.
    if (_heldCount[note] < 255) ++_heldCount[note];
    _age[note] = _ageCounter++;
    if (_ageCounter == 0) {
        // Extremely rare wrap guard: preserve ordering without a zero sentinel.
        _ageCounter = 1;
        for (int n = 0; n < 128; ++n)
            if (_heldCount[n]) _age[n] = 1;
    }
    _velocity = velocity;
    selectHeldNote();

    // v0.5.6a8 MULTI trigger: every Note-On received while a note was
    // already held requests a fresh Filter + Loudness contour trigger.
    // SINGLE leaves the classic legato behavior unchanged.
    if (wasGate &&
        Parameters::instance().target(ParameterID::TRIGGER_MODE) >= 0.5f) {
        _triggerSerial.fetch_add(1, std::memory_order_release);
    }
}

void SynthEngine::noteOff(uint8_t note)
{
    if (_heldCount[note] > 0) --_heldCount[note];
    if (_heldCount[note] == 0) _age[note] = 0;
    selectHeldNote();
}




void SynthEngine::allNotesOff()
{
    for(int i=0; i<128; ++i) {
        _heldCount[i] = 0;
        _age[i] = 0;
    }

    _gate = false;
}

void SynthEngine::pitchBend(int bend)
{
    _pitchBend = bend;
}

void SynthEngine::runAudioTask()
{
    constexpr int N = RTAL_DMA_BUF_LEN;
    static int32_t out[N * 2];

    Parameters &p = Parameters::instance();
    float feedbackSample = 0.0f;
    uint32_t handledTriggerSerial =
        _triggerSerial.load(std::memory_order_acquire);

    for (;;) {
        const BlockParams b = prepareBlock(p);
        sDelay.prepareBlock(p);

        const bool gate = _gate;
        const uint32_t triggerSerial =
            _triggerSerial.load(std::memory_order_acquire);
        const bool multiRetrigger = (triggerSerial != handledTriggerSerial);
        handledTriggerSerial = triggerSerial;

        if ((gate && !lastGate) || (gate && multiRetrigger)) {
            ampEnv.gateOn();
            filtEnv.gateOn();
        }
        if (!gate && lastGate) {
            ampEnv.gateOff();
            filtEnv.gateOff();
        }
        lastGate = gate;

        // v0.5.6a12: keyboard glide and performance pitch bend are separated.
        // Portamento follows the keyboard CV; the pitch wheel remains immediate
        // and does not inherit the glide time. Bend range remains +/-2 semitones.
        const float bendSemis = ((float)_pitchBend / 8192.0f) * 2.0f;
        const float targetNote = (float)_note;

        // Do not create an artificial portamento from an arbitrary boot pitch
        // to the first note ever played. Afterwards the last keyboard CV is
        // retained, so a new staccato note can glide from the previous pitch
        // just like the classic monophonic behavior.
        if (gate && !glideInitialized) {
            glideNote = targetNote;
            glideInitialized = true;
        }

        for (int i = 0; i < N; ++i) {
            glideNote += (targetNote - glideNote) * b.glideCoeff;
            const float glideHz = noteHz(glideNote + b.masterTuneSemis + bendSemis);

            // OSC3 can be disconnected from keyboard control and becomes
            // a low-frequency modulation source, as on the classic instrument.
            float f3Base;
            if (b.osc3Keyboard)
                f3Base = glideHz * b.osc3Range * b.osc3Tune;
            else
                // v0.5.6a8: calibrated OSC3 low-frequency mode.
                // The five range positions now span nominally 0.5/1/2/4/8 Hz
                // (before OSC3 tune), giving useful slow sweeps through vibrato.
                f3Base = 2.0f * b.osc3Range * b.osc3Tune;

            // v0.5.6a8 calibrated classic modulation bus. MOD MIX remains a
            // true source balance (OSC3 <-> noise). The Mod Wheel gets a
            // squared performance law: fine vibrato control in the lower half
            // while preserving a deliberately wide full-wheel modulation range.
            // v0.5.6a9: one physical noise source feeds both the modulation
            // bus and the audio mixer, matching the shared-source topology.
            const float noise = noiseSample(b.noiseColor);
            const float modNoise = noise;
            const float osc3Mod = o3.process(f3Base, b.osc3Wave, SR);
            const float wheel = b.modWheel * b.modWheel;
            const float modBusRaw = (1.0f - b.modMix) * osc3Mod +
                                    b.modMix * modNoise;
            const float modBus = modBusRaw * wheel;

            // Full wheel: approximately +/-7 semitones for oscillator pitch.
            // Lower wheel positions remain much finer because of wheel^2.
            float pitchModSemis = b.oscMod ? (modBus * 7.0f) : 0.0f;
            float pitchMod = fastExp2(pitchModSemis * (1.0f / 12.0f));

            const float f1 = glideHz * b.osc1Range * pitchMod;
            const float f2 = glideHz * b.osc2Range * b.osc2Tune * pitchMod;
            const float f3 = f3Base * pitchMod;

            // OSC3 audio uses its already generated modulation sample to avoid
            // advancing the oscillator twice in one output sample.
            const float n = noise;
            float mix =
                o1.process(f1, b.osc1Wave, SR) * b.osc1Level +
                o2.process(f2, b.osc2Wave, SR) * b.osc2Level +
                osc3Mod * b.osc3Level +
                n * b.noiseLevel;

            // v0.5.6a1 calibrated source gain into nonlinear mixer.
            mix *= 0.30f;

            // v0.5.6a10 External-Input Character:
            // feed the previous post-VCA synth signal back into the mixer.
            // The branch itself is softly saturated, like an overloaded
            // external-input path, and remains bounded at extreme settings.
            const float fbDriveNorm = fastTanh(1.60f);
            const float fbReturn = fastTanh(feedbackSample * 1.60f) /
                                   (fbDriveNorm > 0.001f ? fbDriveNorm : 1.0f);
            mix += fbReturn * b.feedback;

            // Gain-compensated soft saturation: DRIVE primarily changes
            // harmonic compression instead of simply changing output level.
            const float driveNorm = fastTanh(b.mixerDrive);
            const float driven = fastTanh(mix * b.mixerDrive) /
                                 (driveNorm > 0.001f ? driveNorm : 1.0f);

            const float fe = filtEnv.process(
                b.filtAttackInc, b.filtDecayCoeff,
                b.filtSustain, b.decayEnable ? b.filtReleaseCoeff : 1.0f);

            const float ae = ampEnv.process(
                b.ampAttackInc, b.ampDecayCoeff,
                b.ampSustain, b.decayEnable ? b.ampReleaseCoeff : 1.0f);

            // Contour, keyboard tracking and classic modulation bus.
            // Contour and keyboard tracking operate in octave space.
            const float contourOct = b.contour * fe * 5.0f;
            // FULL = exactly one cutoff octave per played keyboard octave.
            const float keyOct = (((float)_note - 60.0f) * (1.0f/12.0f)) *
                                 b.keyTrack;
            // Full wheel: up to roughly +/-3 cutoff octaves.
            const float modOct = b.filterMod ? (modBus * 3.0f) : 0.0f;
            const float cutoff = b.cutoffHz *
                                 fastExp2(contourOct + keyOct + modOct);
            const float g = ladderG(cutoff);

            // 2x oversampling for mixer/filter path.
            // Linear interpolation between previous/current driven sample.
            const float osIn0 = 0.5f * (prevOversampleIn + driven);
            const float osIn1 = driven;
            prevOversampleIn = driven;

            const float y0 = ladder.process(osIn0, g, b.resonance);
            const float y1 = ladder.process(osIn1, g, b.resonance);

            // Simple two-sample average downsample. This is intentionally
            // lightweight; a proper half-band stage is a future refinement.
            float y = 0.5f * (y0 + y1);

            // v0.5.6a10: the classic external-output -> external-input trick
            // includes the loudness contour. Keep MASTER outside the loop so
            // output-volume changes do not retune the feedback character.
            feedbackSample = y * ae;
            if (!(feedbackSample == feedbackSample) || fabsf(feedbackSample) > 4.0f)
                feedbackSample = 0.0f;

            // v0.5.6a11 VCA / Output Character Calibration.
            // Keep the proven a5 contour timing untouched. The VCA control
            // law is only mildly curved: full envelope remains unity while
            // the middle of the contour is a little less ideal/linear.
            const float vcaGain = ae * (0.90f + 0.10f * ae);
            float vcaOut = y * vcaGain;

            // Gentle output-stage compression with unity small-signal gain.
            // This adds headroom/rounding at high internal levels without
            // putting MASTER into the a10 feedback loop.
            const float outputDrive = 0.65f;
            vcaOut = fastTanh(vcaOut * outputDrive) / outputDrive;
            y = vcaOut * b.master;

            if (!(y == y) || fabsf(y) > 4.0f) y = 0.0f;
            if (y >  0.98f) y =  0.98f;
            if (y < -0.98f) y = -0.98f;

            float outL, outR;
            sDelay.process(y,outL,outR);
            if (!(outL == outL) || fabsf(outL) > 8.0f) outL=0.0f;
            if (!(outR == outR) || fabsf(outR) > 8.0f) outR=0.0f;
            if(outL>0.98f) outL=0.98f; if(outL<-0.98f) outL=-0.98f;
            if(outR>0.98f) outR=0.98f; if(outR<-0.98f) outR=-0.98f;
            out[i * 2]     = (int32_t)(outL * 2147483647.0f);
            out[i * 2 + 1] = (int32_t)(outR * 2147483647.0f);
        }

        size_t written = 0;
        esp_err_t wr = i2s_write(
            I2S_PORT,
            out,
            sizeof(out),
            &written,
            portMAX_DELAY
        );

        if(wr != ESP_OK) {
            _audioWriteErrors++;
        } else {
            _audioBlocks++;
            if(written != sizeof(out))
                _audioShortWrites++;
        }
    }
}

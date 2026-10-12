/*
 mi-UGens - SuperCollider UGen Library
 MiBeads: Copyright (c) 2026 Caleb Crittenden.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see http://www.gnu.org/licenses/ .
 */

/*

 MiBeads - based on the 'Beads' eurorack module by
 https://mutable-instruments.net/

 Beads was designed by Émilie Gillet. All of the signal processing used
 here (eurorack/beads, eurorack/stmlib) is her original code,
 Copyright 2017 Emilie Gillet, released under the MIT license on
 October 12, 2026: https://github.com/pichenettes/eurorack
 Thank you, Émilie, for more than a decade of publishing your instruments' code and
 hardware, which is what made ports like this one possible.

 Wrapper structure follows Volker Böhm's MiClouds (mi-UGens).
 Not affiliated with or endorsed by Mutable Instruments.

 How it maps onto the hardware:
 Beads changes its codec clock with the quality setting
   quality 0  hi-fi digital    codec 48 kHz, 16-bit
   quality 1  vintage digital  codec 32 kHz, 12-bit
   quality 2  clean tape       codec 48 kHz, 12-bit, processed at 24 kHz
   quality 3  dirty tape       codec 24 kHz, 8-bit mu-law, wow and flutter
 and processes blocks of (codec rate / 4000) frames.

 This UGen keeps an internal codec running at that rate. Audio is
 resampled into it with a windowed-sinc filter, processed by the Beads
 Processor in hardware-sized blocks, and resampled back. When the server
 rate equals the codec rate (48 kHz server with quality 0 or 2) both
 resamplers are bypassed.

 The resampler stands in for the codec's converters; it does not model
 them. The input stage follows the module's manual-gain branch only.

 Changes to the Beads sources (marked "MiBeads:"):
 - variable length arrays replaced by fixed kMaxBlockSize arrays (MSVC)
 - 12-bit delay line items packed explicitly into 3 bytes; the original
   packed bitfields are 4 bytes on MSVC and MinGW and overran the buffer
 - a debug printf on the audio thread disabled
 */

#include "SC_PlugIn.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

#include "beads/dsp/processor.h"
#include "beads/dsp/synth.h"
#include "stmlib/dsp/dsp.h"
#include "stmlib/utils/gate_flags.h"

static InterfaceTable* ft;

using namespace beads;

// ---------------------------------------------------------------------------
// Inputs
enum {
    IN_PITCH,      // semitones
    IN_PITCH_RND,  // attenuverter, -1..1
    IN_TIME,
    IN_TIME_RND,
    IN_SIZE,
    IN_SIZE_RND,
    IN_SHAPE,
    IN_SHAPE_RND,
    IN_DENSITY,
    IN_FEEDBACK,
    IN_DRYWET,
    IN_REVERB,
    IN_FREEZE,     // gate
    IN_SEED,       // gate/trigger; scalar constant == unpatched
    IN_BUTTON,     // 0 off, 1 pressed, 2 locked
    IN_QUALITY,    // 0..3
    IN_GAIN,       // input gain multiplier
    IN_SYNTH,      // >0.5: wavetable synth mode (input ignored)
    kNumArgs
};

// ---------------------------------------------------------------------------
// Windowed-sinc kernel table (shared, built at load time, read-only after)
static const int kZeroCrossings = 16;
static const int kKernelRes = 256;
static const int kKernelSize = kZeroCrossings * kKernelRes;
static float g_kernel[kKernelSize + 2];

static double BesselI0(double x) {
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 50; ++k) {
        term *= (x / (2.0 * k)) * (x / (2.0 * k));
        sum += term;
        if (term < 1e-12 * sum) break;
    }
    return sum;
}

static void BuildKernel() {
    const double beta = 6.5;  // ~ -65 dB stopband
    const double i0b = BesselI0(beta);
    for (int i = 0; i <= kKernelSize + 1; ++i) {
        double x = double(i) / kKernelRes;  // in zero-crossing units
        double s = (x == 0.0) ? 1.0 : sin(M_PI * x) / (M_PI * x);
        double r = x / kZeroCrossings;
        double w = (r >= 1.0) ? 0.0 : BesselI0(beta * sqrt(1.0 - r * r)) / i0b;
        g_kernel[i] = float(s * w);
    }
}

static const int kRingSize = 16384;  // power of two
static const int kRingMask = kRingSize - 1;
static const int kMaxProcBlock = 16;  // media rate / 4000 <= 12

// ---------------------------------------------------------------------------
struct Resampler {
    // Reads a ring buffer sampled at the source rate at fractional
    // positions. cutoff_ is in cycles per source sample.
    bool identity;
    float cutoff2;      // 2 * cutoff
    float half_width;   // kernel half width in source samples
    float table_scale;  // source samples -> table index

    void Init(double src_rate, double dst_rate) {
        identity = (src_rate == dst_rate);
        double lowest = src_rate < dst_rate ? src_rate : dst_rate;
        double cutoff = 0.46 * lowest / src_rate;
        cutoff2 = float(2.0 * cutoff);
        half_width = identity ? 0.0f : float(kZeroCrossings / (2.0 * cutoff));
        table_scale = cutoff2 * kKernelRes;
    }

    // Highest source index needed to read position t.
    inline int64 last_needed(double t) const {
        return (int64)floor(t + half_width);
    }

    inline void Read(const float* l, const float* r, double t,
                     float* out_l, float* out_r) const {
        if (identity) {
            int64 n = (int64)t;
            *out_l = l[n & kRingMask];
            *out_r = r[n & kRingMask];
            return;
        }
        int64 n0 = (int64)ceil(t - half_width);
        int64 n1 = (int64)floor(t + half_width);
        float acc_l = 0.0f, acc_r = 0.0f;
        for (int64 n = n0; n <= n1; ++n) {
            float x = fabsf(float(t - double(n))) * table_scale;
            int i = int(x);
            if (i >= kKernelSize) continue;
            float f = x - float(i);
            float w = g_kernel[i] + (g_kernel[i + 1] - g_kernel[i]) * f;
            int idx = int(n & kRingMask);
            acc_l += l[idx] * w;
            acc_r += r[idx] * w;
        }
        *out_l = acc_l * cutoff2;
        *out_r = acc_r * cutoff2;
    }
};

// Manual-gain branch of beads/gain_control.cc, at the media rate.
struct InputGain {
    float peak, agc_peak, soft_amount, one_hertz;
    int mode;  // 0 off, 1 on, 2 dynamic

    void Init(float sr, int m) {
        peak = 0.0f;
        agc_peak = 0.5f;
        soft_amount = 0.0f;
        one_hertz = 1.0f / sr;
        mode = m;
    }

    void Process(float gain, FloatFrame* f, size_t size) {
        const float hz = float(size) * one_hertz;
        SLOPE(agc_peak, peak, 5.0f * hz, 0.5f * hz);
        const float optimal = std::min(0.9f / std::min(std::max(agc_peak, 1e-4f), 0.9f), 40.0f);
        float target;
        if (mode == 0) target = 0.0f;
        else if (mode == 1) target = 1.0f;
        else {
            target = 2.0f * (gain / optimal - 0.75f);
            CONSTRAIN(target, 0.0f, 1.0f);
        }
        stmlib::ParameterInterpolator amt(&soft_amount, target, size);
        for (size_t i = 0; i < size; ++i) {
            float l = f[i].l, r = f[i].r;
            float pk = std::max(fabsf(l), fabsf(r));
            SLOPE(peak, pk, 0.1f, 2.0f * one_hertz);
            l *= gain;
            r *= gain;
            const float a = amt.Next();
            const float bias = 0.05f;
            float sl = (1.0f - bias) * stmlib::SoftClip(l + bias) - bias;
            float sr = (1.0f - bias) * stmlib::SoftClip(r + bias) - bias;
            CONSTRAIN(l, -1.0f, 1.0f);
            CONSTRAIN(r, -1.0f, 1.0f);
            f[i].l = mode == 0 ? l : stmlib::Crossfade(l, sl, a);
            f[i].r = mode == 0 ? r : stmlib::Crossfade(r, sr, a);
        }
    }
};

// ---------------------------------------------------------------------------
static const size_t kDelayMemSize = 768 * 1024;
static const size_t kReverbMemSize = 32768;  // uint16 words

enum FadeState { FADE_NONE, FADE_OUT, FADE_IN };

struct MiBeads : public Unit {
    Processor* processor;
    Synth* synth;
    uint8_t* delay_mem;
    uint16_t* reverb_mem;
    float* ring;  // 4 rings of kRingSize: in L, in R, mid L, mid R
    uint8_t* gate_ring;  // bit0 freeze, bit1 seed (server rate)

    bool mono;
    bool seed_patched;
    int media;
    bool synth_mode;

    double server_rate;
    double media_rate;
    int block;  // processor block size in media frames

    Resampler down, up;
    double t_down, step_down;  // in server samples
    double t_up, step_up;      // in media samples
    int64 w_in, w_mid;
    int64 last_gate_idx;

    FloatFrame proc_in[kMaxProcBlock];
    FloatFrame proc_out[kMaxProcBlock];
    bool freeze_buf[kMaxProcBlock];
    stmlib::GateFlags seed_buf[kMaxProcBlock];
    stmlib::GateFlags previous_seed;
    int proc_count;

    InputGain input_gain;

    FadeState fade_state;
    float fade;
};

static void MiBeads_next(MiBeads* unit, int inNumSamples);
static void MiBeads_Ctor(MiBeads* unit);
static void MiBeads_Dtor(MiBeads* unit);

static inline float InAt(Unit* unit, int index, int k) {
    return INRATE(index) == calc_FullRate ? IN(index)[k] : IN0(index);
}

static void Reconfigure(MiBeads* unit, int media, bool synth_mode) {
    unit->media = media;
    unit->synth_mode = synth_mode;

    memset(unit->reverb_mem, 0, kReverbMemSize * sizeof(uint16_t));
    if (synth_mode) {
        unit->synth->Init(unit->reverb_mem);
        unit->media_rate = 48000.0;
    } else {
        unit->processor->Configure(Processor::Media(media), unit->mono);
        unit->media_rate = unit->processor->sample_rate();
    }
    unit->block = int(unit->media_rate) / 4000;

    unit->down.Init(unit->server_rate, unit->media_rate);
    unit->up.Init(unit->media_rate, unit->server_rate);
    unit->step_down = unit->server_rate / unit->media_rate;
    unit->step_up = unit->media_rate / unit->server_rate;

    memset(unit->ring, 0, 4 * kRingSize * sizeof(float));
    memset(unit->gate_ring, 0, kRingSize);
    unit->w_in = 0;
    unit->w_mid = 0;
    unit->last_gate_idx = -1;
    unit->proc_count = 0;
    unit->previous_seed = stmlib::GATE_FLAG_LOW;

    // First media frame is centred on server sample 0; it becomes
    // available once half_width server samples have arrived.
    unit->t_down = 0.0;
    // Output read position lags enough that the up-resampler never
    // reaches past the last processed frame.
    double latency = unit->up.half_width + unit->block
        + unit->down.half_width * unit->step_up + 4.0;
    unit->t_up = -ceil(latency);

    int soft = media == Processor::MEDIA_DIRTY_TAPE ? 1
        : (media == Processor::MEDIA_CLEAN_TAPE ? 2 : 0);
    unit->input_gain.Init(float(unit->media_rate), soft);
}

static void MiBeads_Ctor(MiBeads* unit) {
    unit->processor = nullptr;
    unit->synth = nullptr;
    unit->delay_mem = nullptr;
    unit->reverb_mem = nullptr;
    unit->ring = nullptr;
    unit->gate_ring = nullptr;

    if (BUFLENGTH > 4096) {
        Print("MiBeads ERROR: block size larger than 4096 is not supported\n");
        SETCALC(*ft->fClearUnitOutputs);
        ClearUnitOutputs(unit, 1);
        return;
    }

    World* w = unit->mWorld;
    unit->delay_mem = (uint8_t*)RTAlloc(w, kDelayMemSize);
    unit->reverb_mem = (uint16_t*)RTAlloc(w, kReverbMemSize * sizeof(uint16_t));
    unit->ring = (float*)RTAlloc(w, 4 * kRingSize * sizeof(float));
    unit->gate_ring = (uint8_t*)RTAlloc(w, kRingSize);
    void* pmem = RTAlloc(w, sizeof(Processor));
    void* smem = RTAlloc(w, sizeof(Synth));

    if (!unit->delay_mem || !unit->reverb_mem || !unit->ring
        || !unit->gate_ring || !pmem || !smem) {
        Print("MiBeads ERROR: RT memory allocation failed, "
              "increase s.options.memSize (needs ~1 MB per instance)\n");
        if (pmem) RTFree(w, pmem);
        if (smem) RTFree(w, smem);
        SETCALC(*ft->fClearUnitOutputs);
        ClearUnitOutputs(unit, 1);
        return;
    }

    memset(unit->delay_mem, 0, kDelayMemSize);
    memset(pmem, 0, sizeof(Processor));
    memset(smem, 0, sizeof(Synth));
    unit->processor = new (pmem) Processor();
    unit->synth = new (smem) Synth();

    uint8_t* buffers[3];
    buffers[0] = unit->delay_mem;
    buffers[1] = unit->delay_mem + 256 * 1024;
    buffers[2] = unit->delay_mem + 512 * 1024;
    memset(unit->reverb_mem, 0, kReverbMemSize * sizeof(uint16_t));
    unit->processor->Init(buffers, unit->reverb_mem);

    int num_audio_inputs = unit->mNumInputs - kNumArgs;
    unit->mono = num_audio_inputs < 2;
    unit->seed_patched = INRATE(IN_SEED) != calc_ScalarRate;
    unit->server_rate = SAMPLERATE;

    int quality = int(IN0(IN_QUALITY));
    CONSTRAIN(quality, 0, 3);
    Reconfigure(unit, 3 - quality, IN0(IN_SYNTH) > 0.5f);

    unit->fade_state = FADE_IN;
    unit->fade = 0.0f;

    SETCALC(MiBeads_next);
    ClearUnitOutputs(unit, 1);
}

static void MiBeads_Dtor(MiBeads* unit) {
    World* w = unit->mWorld;
    if (unit->processor) {
        unit->processor->~Processor();
        RTFree(w, unit->processor);
    }
    if (unit->synth) {
        unit->synth->~Synth();
        RTFree(w, unit->synth);
    }
    if (unit->delay_mem) RTFree(w, unit->delay_mem);
    if (unit->reverb_mem) RTFree(w, unit->reverb_mem);
    if (unit->ring) RTFree(w, unit->ring);
    if (unit->gate_ring) RTFree(w, unit->gate_ring);
}

// Runs one hardware-sized block. k = server sample index (within the
// current SC block) used to read audio-rate parameter inputs.
static void RunBlock(MiBeads* unit, int k) {
    const int size = unit->block;

    Parameters p;
    p.time.base = InAt(unit, IN_TIME, k);
    p.time.attenuverter = InAt(unit, IN_TIME_RND, k);
    p.size.base = InAt(unit, IN_SIZE, k);
    p.size.attenuverter = InAt(unit, IN_SIZE_RND, k);
    p.shape.base = InAt(unit, IN_SHAPE, k);
    p.shape.attenuverter = InAt(unit, IN_SHAPE_RND, k);
    p.pitch.base = InAt(unit, IN_PITCH, k);
    p.pitch.attenuverter = InAt(unit, IN_PITCH_RND, k);

    CONSTRAIN(p.time.base, 0.0f, 1.0f);
    CONSTRAIN(p.size.base, 0.0f, 1.0f);
    CONSTRAIN(p.shape.base, 0.0f, 1.0f);
    CONSTRAIN(p.pitch.base, -84.0f, 84.0f);
    CONSTRAIN(p.time.attenuverter, -1.0f, 1.0f);
    CONSTRAIN(p.size.attenuverter, -1.0f, 1.0f);
    CONSTRAIN(p.shape.attenuverter, -1.0f, 1.0f);
    CONSTRAIN(p.pitch.attenuverter, -1.0f, 1.0f);

    p.time.cv = p.size.cv = p.shape.cv = p.pitch.cv = 0.0f;
    p.time.has_cv = p.size.has_cv = p.shape.has_cv = p.pitch.has_cv = false;
    p.monophonic_mode = false;

    ModulatedParameter d;
    d.base = InAt(unit, IN_DENSITY, k);
    CONSTRAIN(d.base, 0.0f, 1.0f);
    d.cv = 0.0f;

    Processor::MixParameters m;
    m.feedback = InAt(unit, IN_FEEDBACK, k);
    m.dry_wet = InAt(unit, IN_DRYWET, k);
    m.reverb_amount = InAt(unit, IN_REVERB, k);
    CONSTRAIN(m.feedback, 0.0f, 1.0f);
    CONSTRAIN(m.dry_wet, 0.0f, 1.0f);
    CONSTRAIN(m.reverb_amount, 0.0f, 1.0f);
    m.output_gate = false;

    int button = int(InAt(unit, IN_BUTTON, k) + 0.5f);
    SeedButtonState bs = button >= 2 ? SEED_BUTTON_STATE_LOCKED
        : (button == 1 ? SEED_BUTTON_STATE_ON : SEED_BUTTON_STATE_OFF);

    const stmlib::GateFlags* seed = unit->seed_patched ? unit->seed_buf : NULL;

    if (unit->synth_mode) {
        unit->synth->Render(bs, seed, unit->freeze_buf,
            Synth::Resolution(unit->media), d, p, m, unit->proc_out, size);
    } else {
        float gain = InAt(unit, IN_GAIN, k);
        CONSTRAIN(gain, 0.0f, 40.0f);
        unit->input_gain.Process(gain, unit->proc_in, size);
        unit->processor->Process(bs, seed, unit->freeze_buf, d, p, m,
            unit->proc_in, unit->proc_out, size);
    }

    float* mid_l = unit->ring + 2 * kRingSize;
    float* mid_r = unit->ring + 3 * kRingSize;
    for (int i = 0; i < size; ++i) {
        float l = unit->proc_out[i].l;
        float r = unit->proc_out[i].r;
        CONSTRAIN(l, -1.0f, 1.0f);  // codec full scale
        CONSTRAIN(r, -1.0f, 1.0f);
        int idx = int(unit->w_mid & kRingMask);
        mid_l[idx] = l;
        mid_r[idx] = r;
        ++unit->w_mid;
    }
}

static void MiBeads_next(MiBeads* unit, int inNumSamples) {
    const int n = inNumSamples;
    float* out_l = OUT(0);
    float* out_r = OUT(1);

    // Quality / synth changes: fade out, reconfigure, fade in.
    int quality = int(IN0(IN_QUALITY));
    CONSTRAIN(quality, 0, 3);
    int want_media = 3 - quality;
    bool want_synth = IN0(IN_SYNTH) > 0.5f;
    if ((want_media != unit->media || want_synth != unit->synth_mode)
        && unit->fade_state != FADE_OUT) {
        // Restarts the fade out from the current level if a fade in
        // was still running.
        unit->fade_state = FADE_OUT;
    }

    const float* in_l = IN(kNumArgs);
    const float* in_r = unit->mono ? in_l : IN(kNumArgs + 1);

    float* ring_l = unit->ring;
    float* ring_r = unit->ring + kRingSize;
    float* mid_l = unit->ring + 2 * kRingSize;
    float* mid_r = unit->ring + 3 * kRingSize;

    // 1. Push server-rate input and gates.
    const int64 block_start = unit->w_in;
    const bool freeze_audio = INRATE(IN_FREEZE) == calc_FullRate;
    const bool seed_audio = INRATE(IN_SEED) == calc_FullRate;
    const float* freeze_in = IN(IN_FREEZE);
    const float* seed_in = IN(IN_SEED);
    for (int i = 0; i < n; ++i) {
        int idx = int(unit->w_in & kRingMask);
        ring_l[idx] = in_l[i];
        ring_r[idx] = in_r[i];
        uint8_t g = 0;
        if ((freeze_audio ? freeze_in[i] : freeze_in[0]) > 0.0f) g |= 1;
        if ((seed_audio ? seed_in[i] : seed_in[0]) > 0.0f) g |= 2;
        unit->gate_ring[idx] = g;
        ++unit->w_in;
    }

    // 2. Produce media-rate frames and run the processor block by block.
    while (unit->down.last_needed(unit->t_down) <= unit->w_in - 1) {
        float l, r;
        unit->down.Read(ring_l, ring_r, unit->t_down, &l, &r);

        // Gates: OR over all server samples since the previous frame so
        // single-sample triggers are never skipped.
        int64 gi = (int64)floor(unit->t_down + 0.5);
        if (gi > unit->w_in - 1) gi = unit->w_in - 1;
        uint8_t g = 0;
        int64 from = unit->last_gate_idx + 1;
        if (from < gi - kRingSize / 2) from = gi - kRingSize / 2;
        for (int64 j = from; j <= gi; ++j) g |= unit->gate_ring[j & kRingMask];
        if (gi > unit->last_gate_idx) unit->last_gate_idx = gi;

        const int c = unit->proc_count;
        unit->proc_in[c].l = l;
        unit->proc_in[c].r = r;
        unit->freeze_buf[c] = g & 1;
        unit->previous_seed = stmlib::ExtractGateFlags(
            unit->previous_seed, (g & 2) != 0);
        unit->seed_buf[c] = unit->previous_seed;
        unit->proc_count = c + 1;

        unit->t_down += unit->step_down;

        if (unit->proc_count >= unit->block) {
            int k = int(gi - block_start);
            CONSTRAIN(k, 0, n - 1);
            RunBlock(unit, k);
            unit->proc_count = 0;
        }
    }

    // 3. Resample processed audio back to the server rate.
    const float fade_out_step = float(1.0 / (0.005 * unit->server_rate));
    const float fade_in_step = float(1.0 / (0.02 * unit->server_rate));
    for (int i = 0; i < n; ++i) {
        float l = 0.0f, r = 0.0f;
        double t = unit->t_up;
        if (t >= 0.0 && unit->up.last_needed(t) <= unit->w_mid - 1) {
            unit->up.Read(mid_l, mid_r, t, &l, &r);
        }
        unit->t_up += unit->step_up;

        if (unit->fade_state == FADE_OUT) {
            unit->fade -= fade_out_step;
            if (unit->fade < 0.0f) unit->fade = 0.0f;
        } else if (unit->fade_state == FADE_IN) {
            unit->fade += fade_in_step;
            if (unit->fade >= 1.0f) {
                unit->fade = 1.0f;
                unit->fade_state = FADE_NONE;
            }
        }
        out_l[i] = l * unit->fade;
        out_r[i] = r * unit->fade;
    }

    if (unit->fade_state == FADE_OUT && unit->fade <= 0.0f) {
        // Only reset the engine if the request still differs once the
        // fade has finished (a quick flip back costs a dip, not the buffer).
        if (want_media != unit->media || want_synth != unit->synth_mode) {
            Reconfigure(unit, want_media, want_synth);
        }
        unit->fade_state = FADE_IN;
    }
}

PluginLoad(MiBeads) {
    ft = inTable;
    BuildKernel();
    DefineDtorUnit(MiBeads);
}

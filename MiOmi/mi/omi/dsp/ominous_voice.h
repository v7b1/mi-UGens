// Copyright 2014 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// "Ominous" is a dark 2x2-op FM synth (based on Braids' FM and FBFM modes).

#ifndef ELEMENTS_DSP_OMINOUS_VOICE_H_
#define ELEMENTS_DSP_OMINOUS_VOICE_H_

#include "stmlib/stmlib.h"

#include <algorithm>

#include "stmlib/dsp/dsp.h"
#include "stmlib/dsp/filter.h"

#include "omi/dsp/dsp.h"
#include "omi/dsp/multistage_envelope.h"
#include "omi/dsp/patch.h"
#include "omi/resources.h"

namespace omi {

const size_t kOversamplingUp = 8;

const size_t kNumOscillators = 2;


class Spatializer {
 public:
  Spatializer() { }
  ~Spatializer() { }
  void Init(float fixed_position);

  inline void Rotate(float rotation_speed) {
    angle_ += rotation_speed;
    if (angle_ >= 1.0f) {
      angle_ -= 1.0f;
    }
    if (angle_ < 0.0f) {
      angle_ += 1.0f;
    }
  }
  
  inline void set_distance(float distance) {
    distance_ = distance;
  }

  void Process(float* source, float* center, float* sides, size_t size);

 private:
  float behind_[kMaxBlockSize];
  float left_;
  float right_;
  float angle_;
  float distance_;
  float fixed_position_;
  
  stmlib::NaiveSvf behind_filter_;
  
  DISALLOW_COPY_AND_ASSIGN(Spatializer);
};

class FmOscillator {
 public:
  FmOscillator() { }
  ~FmOscillator() { }
  void Init(float srFactor) {
      intervalCorrection_ = logf(srFactor)/logf(2.0f)*12.0f;        // vb
    fm_amount_ = 0.0f;
    previous_sample_ = 0.0f;
      lp_filter[0].Init();
      lp_filter[1].Init();
      // vb: use two onepole filters in the feedback path to avoid aliasing and ringing
      lp_filter[0].set_f<stmlib::FREQUENCY_DIRTY>(0.08f);
      lp_filter[1].set_f<stmlib::FREQUENCY_DIRTY>(0.08f);
  }

  void Process(float frequency,
      float ratio,
      float feedback_amount,
      float target_fm_amount,
      const float* external,
               float *cross_fm,        // vb
               float cross_fb_amount, // vb
      float* destination,
      size_t size);

 private:
  inline float midi_to_increment(float midi_pitch) const {
    int32_t pitch = static_cast<int32_t>(midi_pitch * 256.0f);
    pitch = 32768 + stmlib::Clip16(pitch - 20480);
    float increment = lut_midi_to_increment_high[pitch >> 8] * \
        lut_midi_to_f_low[pitch & 0xff];
    return increment;
  }
  
  inline float Sine(uint32_t phase) const {
    uint32_t integral = phase >> 20;
    float fractional = static_cast<float>(phase << 12) / 4294967296.0f;
    float a = lut_sine[integral];
    float b = lut_sine[integral + 1];
    return a + (b - a) * fractional;
  }
  
  inline float SineFm(uint32_t phase, float fm) const {
    phase += static_cast<uint32_t>(fm * 2147483648.0f);
    uint32_t integral = phase >> 20;
    float fractional = static_cast<float>(phase << 12) / 4294967296.0f;
    float a = lut_sine[integral];
    float b = lut_sine[integral + 1];
    return a + (b - a) * fractional;
  }
  
  float fm_amount_;
  float previous_sample_;
  uint32_t phase_carrier_;
  uint32_t phase_mod_;
    
    float intervalCorrection_;
    
    stmlib::OnePole lp_filter[2];       // vb
   
  DISALLOW_COPY_AND_ASSIGN(FmOscillator);
};

class OminousVoice {
 public:
  OminousVoice() { }
  ~OminousVoice() { }
  
  void Init(float srFactor);
  void Process(
      const Patch& patch,
      float frequency,
      float strength,
      const bool gate_in,
      const float* audio_in,
      float* center,
      float* sides,
      size_t size);
  
 private:
  void ConfigureEnvelope(const Patch& patch);

  template<int up>
  void Upsample(
      float* state,
      const float* source,
      float* destination,
      size_t source_size) {
    const float down = 1.0f / float(up);
    float s = *state;
    for (size_t i = 0; i < source_size; ++i) {
      float increment = (source[i] - s) * down;
      for (size_t j = 0; j < up; ++j) {
        *destination++ = s;
        s += increment;
      }
    }
    *state = s;
  }

    inline uint8_t GetGateFlags(bool gate_in) {
        uint8_t flags = 0;
        if (gate_in) {
            if (!previous_gate_) {
                flags |= ENVELOPE_FLAG_RISING_EDGE;
        }
        flags |= ENVELOPE_FLAG_GATE;
        } else if (previous_gate_) {
            flags = ENVELOPE_FLAG_FALLING_EDGE;
        }
        previous_gate_ = gate_in;
        return flags;
    }

    inline float midi_to_frequency(float midi_pitch) const {
        if (midi_pitch < -12.0f) {
          midi_pitch = -12.0f;
        }
        int32_t pitch = static_cast<int32_t>(midi_pitch * 256.0f);
        pitch = 32768 + stmlib::Clip16(pitch - 20480);

        return lut_midi_to_f_high[pitch >> 8] * lut_midi_to_f_low[pitch & 0xff] * srFactor_;  // vb
    }
  
    float cross_fm_[kMaxBlockSize];     //vb
    float osc_[kMaxBlockSize];

    bool previous_gate_;
    MultistageEnvelope envelope_;

    float level_[kMaxBlockSize];
    float level_state_;
    float damping_;

    float feedback_;

    float srFactor_;

    float osc_level_[kNumOscillators];

    float external_fm_state_[kNumOscillators];

    FmOscillator oscillator_[kNumOscillators];

    stmlib::Svf filter_[kNumOscillators];

    Spatializer spatializer_[kNumOscillators];

    DISALLOW_COPY_AND_ASSIGN(OminousVoice);
};

}  // namespace omi

#endif  // ELEMENTS_DSP_OMINOUS_VOICE_H_

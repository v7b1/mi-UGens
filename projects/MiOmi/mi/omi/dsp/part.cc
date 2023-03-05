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
// Group of voices.

#include "omi/dsp/part.h"
#include "omi/resources.h"

namespace omi {

using namespace std;
using namespace stmlib;

void Part::Init(float sr) {
    // vb
    sr_ = sr;
    float srFactor = 32000.0f / sr;
    
    patch_.exciter_envelope_shape = 1.0f;
    patch_.exciter_bow_level = 0.0f;
    patch_.exciter_bow_timbre = 0.5f;
    patch_.exciter_blow_level = 0.0f;
    patch_.exciter_blow_meta = 0.5f;
    patch_.exciter_blow_timbre = 0.5f;
    patch_.exciter_strike_level = 0.8f;
    patch_.exciter_strike_meta = 0.5f;
    patch_.exciter_strike_timbre = 0.5f;
    patch_.exciter_signature = 0.0f;
    patch_.resonator_geometry = 0.2f;
    patch_.resonator_brightness = 0.5f;
    patch_.resonator_damping = 0.25f;
    patch_.resonator_position = 0.3f;
    patch_.resonator_modulation_frequency = 0.5f / sr;
    patch_.resonator_modulation_offset = 0.1f;
    patch_.reverb_diffusion = 0.625f;
    patch_.reverb_lp = 0.7f;
    patch_.space = 0.5f;
    
    patch_.resonance = 0.0f;    // vb
    patch_.cross_fb = 0.0f;     // vb

  
    ominous_voice_.Init(srFactor);


    // vb init buffers
    fill(&sides_buffer_[0], &sides_buffer_[kMaxBlockSize], 0.0f);
    fill(&center_buffer_[0], &center_buffer_[kMaxBlockSize], 0.0f);
    
}

void Part::Seed(uint32_t* seed, size_t size) {
  // Scramble all bits from the serial number.
  uint32_t signature = 0xf0cacc1a;
  for (size_t i = 0; i < size; ++i) {
      signature ^= seed[i];
      signature = signature * 1664525L + 1013904223L;
  }
  double x;

  x = static_cast<double>(signature & 7) / 8.0f;
  signature >>= 3;
  patch_.resonator_modulation_frequency = (0.4f + 0.8f * x) / sr_;
  
  x = static_cast<double>(signature & 7) / 8.0f;
  signature >>= 3;
  patch_.resonator_modulation_offset = 0.05f + 0.1f * x;

  x = static_cast<double>(signature & 7) / 8.0f;
  signature >>= 3;
  patch_.exciter_signature = x;
}

void Part::Process(
    const PerformanceState& performance_state,
    const float* audio_in,
    float* left,
    float* right,
    size_t size) {

    float note = performance_state.note;
    float spread = patch_.space;
    
    ominous_voice_.Process(
      patch_,
      note,
      performance_state.strength,
      performance_state.gate,
      audio_in,
      center_buffer_,
      sides_buffer_,
      size);


    // Mixdown.
    
    for (size_t j = 0; j < size; ++j) {
        float side = sides_buffer_[j] * spread;
        left[j] = center_buffer_[j] - side;
        right[j] = center_buffer_[j] + side;
    }
}

}  // namespace omi

// Copyright 2014 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
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
// Reverb.

#ifndef REVERB_H_
#define REVERB_H_

#include "stmlib/stmlib.h"

#include "fx_engine.h"


class Reverb {
 public:
  Reverb() { }
  ~Reverb() { }
  
  void Init(uint16_t* buffer) {
    engine_.Init(buffer);
    //engine_.SetLFOFrequency(LFO_1, 0.5 / 32000.0);
    //engine_.SetLFOFrequency(LFO_2, 0.3 / 32000.0);
    engine_.SetLFOFrequency(LFO_1, 0.5 / 44100.0);
    engine_.SetLFOFrequency(LFO_2, 0.3 / 44100.0);
    lp_ = 0.7;
    hp_ = 0.1;
    lp_decay_1_ = lp_decay_2_ = 0.0;      //vb
    hp_decay_1_ = hp_decay_2_ = 0.0;
      hp1_state_1_ = hp1_state_2_ = 0.0;
      hp2_state_1_ = hp2_state_2_ = 0.0;
    diffusion_ = 0.625;
      diffusion_lp = diffusion_;
  }
  
  void Process(double* left, double* right, size_t size) {
    // This is the Griesinger topology described in the Dattorro paper
    // (4 AP diffusers on the input, then a loop of 2x 2AP+1Delay).
    // Modulation is applied in the loop of the first diffuser AP for additional
    // smearing; and to the two long delays for a slow shimmer/chorus effect.
    typedef E::Reserve<150,
      E::Reserve<214,
      E::Reserve<319,
      E::Reserve<527,
      E::Reserve<2182,
      E::Reserve<2690,
      E::Reserve<4501,
      E::Reserve<2525,
      E::Reserve<2197,
      E::Reserve<6312> > > > > > > > > > Memory;
    E::DelayLine<Memory, 0> ap1;
    E::DelayLine<Memory, 1> ap2;
    E::DelayLine<Memory, 2> ap3;
    E::DelayLine<Memory, 3> ap4;
    E::DelayLine<Memory, 4> dap1a;
    E::DelayLine<Memory, 5> dap1b;
    E::DelayLine<Memory, 6> del1;
    E::DelayLine<Memory, 7> dap2a;
    E::DelayLine<Memory, 8> dap2b;
    E::DelayLine<Memory, 9> del2;
    E::Context c;

    //const double kap = diffusion_;
      
    const double klp = lp_;
      const double khp = hp_;
    const double krt = reverb_time_;
    const double amount = amount_;
    const double gain = input_gain_;

    double lp_1 = lp_decay_1_;
    double lp_2 = lp_decay_2_;
    double hp_1 = hp_decay_1_;
    double hp_2 = hp_decay_2_;
      
      double hp1_state1 = hp1_state_1_;
      double hp1_state2 = hp1_state_2_;
      double hp2_state1 = hp2_state_1_;
      double hp2_state2 = hp2_state_2_;
     

    while (size--) {
        // vb, interpolate diffusion parameter
        diffusion_lp += 0.005 * (diffusion_ - diffusion_lp);
        double kap = diffusion_lp;
        
      double wet;
      double apout = 0.0;
      engine_.Start(&c);
      
      // Smear AP1 inside the loop.
      c.Interpolate(ap1, 10.0, LFO_1, 80.0, 1.0);
      c.Write(ap1, 100, 0.0);
      
      c.Read(*left + *right, gain);

      // Diffuse through 4 allpasses.
      c.Read(ap1 TAIL, kap);
      c.WriteAllPass(ap1, -kap);
      c.Read(ap2 TAIL, kap);
      c.WriteAllPass(ap2, -kap);
      c.Read(ap3 TAIL, kap);
      c.WriteAllPass(ap3, -kap);
      c.Read(ap4 TAIL, kap);
      c.WriteAllPass(ap4, -kap);
      c.Write(apout);
      
      // Main reverb loop.
      c.Load(apout);
      c.Interpolate(del2, 6211.0, LFO_2, 100.0, krt);
      c.Lp(lp_1, klp);
      c.Read(dap1a TAIL, -kap);
      c.WriteAllPass(dap1a, kap);
      c.Read(dap1b TAIL, kap);
        
        
      c.WriteAllPass(dap1b, -kap);
        //c.Hp(hp_1, khp);
        
      c.Write(del1, 2.0);
        c.dcblock(hp1_state1, hp1_state2, khp);
      c.Write(wet, 0.0);

      *left += (wet - *left) * amount;

      c.Load(apout);
      // c.Interpolate(del1, 4450.0f, LFO_1, 50.0f, krt);
      c.Read(del1 TAIL, krt);
      c.Lp(lp_2, klp);
      c.Read(dap2a TAIL, kap);
      c.WriteAllPass(dap2a, -kap);
      c.Read(dap2b TAIL, -kap);
        
        
      c.WriteAllPass(dap2b, kap);
        //c.Hp(hp_2, khp);
        
      c.Write(del2, 2.0);
        c.dcblock(hp2_state1, hp2_state2, khp);
      c.Write(wet, 0.0);

      *right += (wet - *right) * amount;
      
      ++left;
      ++right;
    }
    
    lp_decay_1_ = lp_1;
    lp_decay_2_ = lp_2;
      hp_decay_1_ = hp_1;
      hp_decay_2_ = hp_2;
      
      hp1_state_1_ = hp1_state1;
      hp1_state_2_ = hp1_state2;
      hp2_state_1_ = hp2_state1;
      hp2_state_2_ = hp2_state2;
  }
  
  inline void set_amount(double amount) {
    amount_ = amount;
  }
  
  inline void set_input_gain(double input_gain) {
    input_gain_ = input_gain;
  }

  inline void set_time(double reverb_time) {
    reverb_time_ = reverb_time;
  }
  
  inline void set_diffusion(double diffusion) {
    diffusion_ = diffusion;
  }
  
  inline void set_lp(double lp) {
    lp_ = lp;
  }
    
    inline void set_hp(double hp) {
        hp_ = hp;
    }
    
    // vb
    inline void set_lfo_1(double f) {
        engine_.SetLFOFrequency(LFO_1, f/44100.0);
    }
    inline void set_lfo_2(double f) {
        engine_.SetLFOFrequency(LFO_2, f/44100.0);
    }
  
 private:
  typedef FxEngine<32768, FORMAT_16_BIT> E;
  E engine_;
  
  double amount_;
  double input_gain_;
  double reverb_time_;
  double diffusion_;
    double diffusion_lp;        //vb
  double lp_, hp_;
  
  double lp_decay_1_;
  double lp_decay_2_;
    
    double hp_decay_1_;
    double hp_decay_2_;
    
    double hp1_state_1_;
    double hp1_state_2_;
    double hp2_state_1_;
    double hp2_state_2_;
  
  DISALLOW_COPY_AND_ASSIGN(Reverb);
};

//}  // namespace elements

#endif  // FX_REVERB_H_

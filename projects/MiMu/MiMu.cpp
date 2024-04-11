/*
 *  MiMu.cpp
 *  Plugins
 *
 *  Created by vboehm on 12.04.20
 *
 *
 */

// Mu-law encoding, copyright 2014 Emilie Gillet.
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



#include "SC_PlugIn.h"

static InterfaceTable *ft;


struct MiMu : public Unit {
    
    //float   gain;
    //bool    bypass;
    
};


static void MiMu_Ctor(MiMu *unit);
static void MiMu_next(MiMu *unit, int inNumSamples);



static void MiMu_Ctor(MiMu *unit) {
    
    SETCALC(MiMu_next);        // tells sc synth the name of the calculation function
    MiMu_next(unit, 1);
}


#pragma mark ----- mµ law stuff -----

inline unsigned char Lin2MuLaw(int16_t pcm_val) {
    int16_t mask;
    int16_t seg;
    uint8_t uval;
    pcm_val = pcm_val >> 2;
    if (pcm_val < 0) {
        pcm_val = -pcm_val;
        mask = 0x7f;
    } else {
        mask = 0xff;
    }
    if (pcm_val > 8159) pcm_val = 8159;
    pcm_val += (0x84 >> 2);
    
    if (pcm_val <= 0x3f) seg = 0;
    else if (pcm_val <= 0x7f) seg = 1;
    else if (pcm_val <= 0xff) seg = 2;
    else if (pcm_val <= 0x1ff) seg = 3;
    else if (pcm_val <= 0x3ff) seg = 4;
    else if (pcm_val <= 0x7ff) seg = 5;
    else if (pcm_val <= 0xfff) seg = 6;
    else if (pcm_val <= 0x1fff) seg = 7;
    else seg = 8;
    if (seg >= 8)
        return static_cast<uint8_t>(0x7f ^ mask);
    else {
        uval = static_cast<uint8_t>((seg << 4) | ((pcm_val >> (seg + 1)) & 0x0f));
        return (uval ^ mask);
    }
}

inline short MuLaw2Lin(uint8_t u_val) {
    int16_t t;
    u_val = ~u_val;
    t = ((u_val & 0xf) << 3) + 0x84;
    t <<= ((unsigned)u_val & 0x70) >> 4;
    return ((u_val & 0x80) ? (0x84 - t) : (t - 0x84));
}


#pragma mark ----- soft clipping -----

inline float SoftLimit(float x) {
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

inline float SoftClip(float x) {
    if (x < -3.0f) {
        return -1.0f;
    } else if (x > 3.0f) {
        return 1.0f;
    } else {
        return SoftLimit(x);
    }
}


#pragma mark ----- dsp loop -----

void MiMu_next( MiMu *unit, int inNumSamples )
{
    float   *in = IN(0);
    float   *gain_in = IN(1);
    bool    bypass = IN0(2) != 0.0f;
    
    float *out1 = OUT(0);
    float scale = 1.0f/32767.0f;

    if(bypass) {
        //Print("bypass ON!\n");
        std::copy(&in[0], &in[inNumSamples], &out1[0]);
        return;
    }

    if(INRATE(1) == calc_FullRate) {
        for (size_t i = 0; i < inNumSamples; ++i) {
            float input = SoftClip(in[i] * gain_in[i]);
            int16_t pcm_in = input * 32767.0;
            uint8_t compressed = Lin2MuLaw(pcm_in);
            
            int16_t pcm_out = MuLaw2Lin(compressed);
            out1[i] = pcm_out * scale;
        }
    }
    else {
        float gain = gain_in[0];
        for (size_t i = 0; i < inNumSamples; ++i) {
            float input = SoftClip(in[i] * gain);
            int16_t pcm_in = input * 32767.0;
            uint8_t compressed = Lin2MuLaw(pcm_in);
            
            int16_t pcm_out = MuLaw2Lin(compressed);
            out1[i] = pcm_out * scale;
        }
    }
    
}


PluginLoad(MiMu) {
    ft = inTable;
    DefineSimpleUnit(MiMu);
}

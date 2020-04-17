/*
 mi-UGens - SuperCollider UGen Library
 Copyright (c) 2020 Volker Böhm. All rights reserved.
 
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
 
 MiVerb - based on the reverb unit of the eurorack modules by
 https://mutable-instruments.net/
 
 Original code by Émilie Gillet
 
 https://vboehm.net
 
 */



#include "SC_PlugIn.h"

#include "rings/dsp/fx/reverb.h"

static InterfaceTable *ft;


struct MiVerb : public Unit {
    
    float      sr;
    bool        bypass;
    
    rings::Reverb      *reverb_;
    uint16_t    *reverb_buffer;
    float      reverb_amount;
    float      reverb_time;
    float      reverb_diffusion;
    float      reverb_lp;
    float      space;
    float      input_gain;
    
};


static void MiVerb_Ctor(MiVerb *unit);
static void MiVerb_Dtor(MiVerb *unit);
static void MiVerb_next(MiVerb *unit, int inNumSamples);



static void MiVerb_Ctor(MiVerb *unit) {
    
    // alloc mem
    unit->reverb_buffer = (uint16_t*)RTAlloc(unit->mWorld, 32768*sizeof(uint16_t)); // 65536
    memset(unit->reverb_buffer, 0, 32768*sizeof(uint16_t));
    
    if(unit->reverb_buffer == NULL) {
        Print( "mem alloc failed!" );
        unit = NULL;
        return;
    }
    
    unit->bypass = false;
    
    unit->reverb_ = new rings::Reverb;
    unit->reverb_->Init(unit->reverb_buffer);
    
    unit->reverb_amount = 0.5;
    unit->reverb_time = 0.5;
    unit->reverb_diffusion = 0.625;
    unit->reverb_lp = 0.7;
    unit->input_gain = 0.5;
    
    unit->reverb_->set_time(unit->reverb_time);
    unit->reverb_->set_input_gain(unit->input_gain);
    unit->reverb_->set_lp(unit->reverb_lp);
    unit->reverb_->set_amount(unit->reverb_amount);
    unit->reverb_->set_diffusion(unit->reverb_diffusion);

    //unit->reverb_->set_hp(0.995);
    
    SETCALC(MiVerb_next);        // tells sc synth the name of the calculation function
    MiVerb_next(unit, 1);
}



static void MiVerb_Dtor(MiVerb *unit) {
    
    if(unit->reverb_buffer) {
        RTFree(unit->mWorld, unit->reverb_buffer);
    }
    if(unit->reverb_) delete unit->reverb_;
}


#pragma mark ---- untility functions ----

void myObj_space(MiVerb *unit, float input) {
    CONSTRAIN(input, 0., 1.); // * 2.0;
    unit->space = input;
    
    // Compute reverb parameters from the "space" metaparameter.
    
    //double space = unit->space >= 1.0 ? 1.0 : unit->space;
    //space = space >= 0.1 ? space - 0.1 : 0.0;
    //unit->reverb_amount = space >= 0.5 ? 1.0 * (space - 0.5) : 0.0;
    unit->reverb_amount = unit->space*0.7;
    unit->reverb_time = 0.3 + unit->reverb_amount;
    
    bool freeze = unit->space >= 0.98;   //1.75;
    if (freeze) {
        unit->reverb_->set_time(1.0);
        unit->reverb_->set_input_gain(0.0);
        unit->reverb_->set_lp(1.0);
    } else {
        unit->reverb_->set_time(unit->reverb_time);
        unit->reverb_->set_input_gain(0.2);
        unit->reverb_->set_lp(unit->reverb_lp);
    }
    
    unit->reverb_->set_amount(unit->reverb_amount);
}


void myObj_freeze(MiVerb *unit, long input) {
    if(input != 0) {
        unit->reverb_->set_time(1.0);
        unit->reverb_->set_input_gain(0.0);
        unit->reverb_->set_lp(1.0);
    } else {
        unit->reverb_->set_time(unit->reverb_time);
        unit->reverb_->set_input_gain(0.2);
        unit->reverb_->set_lp(unit->reverb_lp);
    }
}


inline void SoftClip_block(float *inout, size_t size) {
    while(size--) {
        float x = *inout;
        if (x < -3.0f) {
            *inout = -1.0f;
        } else if (x > 3.0f) {
            *inout = 1.0f;
        }
        inout++;
    }
}

inline void SoftLimit_block(float *inout, size_t size) {
    while(size--) {
        float x2 = (*inout)*(*inout);
        *inout *= (27.0f + x2) / (27.0f + 9.0f * x2);
        inout++;
    }
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

void MiVerb_next( MiVerb *unit, int inNumSamples )
{
    float   *inL = IN(0);
    float   time = IN0(1);
    float   amount = IN0(2);
    float   diffusion = IN0(3);
    float   lp = IN0(4);
    
    float   *outL = OUT(0);
    float   *outR = OUT(1);
    

    size_t size = inNumSamples;;
    
    rings::Reverb *reverb = unit->reverb_;
    
    
    std::copy(&inL[0], &inL[size], &outL[0]);
    //std::copy(&inR[0], &inR[size], &outR[0]);
    // TODO: wants stereo input!
    // copy left to right for now
    std::copy(&inL[0], &inL[size], &outR[0]);
    
    
    if(unit->bypass)
        return;
    
    
    CONSTRAIN(time, 0.f, 1.1f);         // allow for a little distortion :)
    CONSTRAIN(amount, 0.f, 1.f);
    CONSTRAIN(diffusion, 0.f, 0.95f);   // TODO: check parameter range
    CONSTRAIN(lp, 0.f, 1.f);

    
    reverb->set_time(time);
    reverb->set_amount(amount);
    reverb->set_diffusion(diffusion);
    reverb->set_lp(lp);
    
    //unit->reverb_->set_input_gain(unit->input_gain);
    
    reverb->Process(outL, outR, size);
    
    SoftClip_block(outL, size);
    SoftClip_block(outR, size);
    SoftLimit_block(outL, size);
    SoftLimit_block(outR, size);

}




PluginLoad(MiVerb) {
    ft = inTable;
    DefineDtorUnit(MiVerb);
}

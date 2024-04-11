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

#include "reverb.h"

static InterfaceTable *ft;

const uint16 kNumArgs = 6;


struct MiVerb : public Unit {
    
    Reverb      reverb_;
    uint16_t    *reverb_buffer;
    float       input_gain;
    
};


static void MiVerb_Ctor(MiVerb *unit);
static void MiVerb_Dtor(MiVerb *unit);
static void MiVerb_next(MiVerb *unit, int inNumSamples);


static void MiVerb_Ctor(MiVerb *unit) {
    
    // alloc mem
    unit->reverb_buffer = (uint16_t*)RTAlloc(unit->mWorld, 32768*sizeof(uint16_t));
    memset(unit->reverb_buffer, 0, 32768*sizeof(uint16_t));
    
    if(unit->reverb_buffer == NULL) {
        Print( "mem alloc failed!" );
        unit = NULL;
        ClearUnitOutputs(unit, BUFLENGTH); // ??
        return;
    }

    
    memset(&unit->reverb_, 0, sizeof(unit->reverb_));
    unit->reverb_.Init(unit->reverb_buffer, SAMPLERATE);
    
    unit->input_gain = 0.2f;
    
    unit->reverb_.set_time(0.7f);
    unit->reverb_.set_input_gain(unit->input_gain);
    unit->reverb_.set_lp(0.3f);
    unit->reverb_.set_amount(0.5f);
    unit->reverb_.set_diffusion(0.625f);       // original value form Dattorro paper

    unit->reverb_.set_hp(0.995f);
    
    //uint16 numAudioInputs = unit->mNumInputs - kNumArgs;
    //Print("MiVerb > numAudioIns: %d\n", numAudioInputs);
    
    SETCALC(MiVerb_next);
    ClearUnitOutputs(unit, BUFLENGTH);
    //MiVerb_next(unit, BUFLENGTH);
    
}


static void MiVerb_Dtor(MiVerb *unit) {
    
    if(unit->reverb_buffer) {
        RTFree(unit->mWorld, unit->reverb_buffer);
    }
    
}



void unit_freeze(MiVerb *unit) {
   
    unit->reverb_.set_time(1.0);
    unit->reverb_.set_input_gain(0.0);
    unit->reverb_.set_lp(1.0);
    
}


#pragma mark ----- soft clipping -----

inline void SoftClip_block(float *inout, size_t size) {
    while(size--) {
        float x = *inout;
        if (x < -3.0f) {
            *inout = -1.0f;
        } else if (x > 3.0f) {
            *inout = 1.0f;
        } else {
            float x2 = x * x;
            *inout *= (27.0f + x2) / (27.0f + 9.0f * x2);
        }
        inout++;
    }
}


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
    float   time = IN0(0);
    float   drywet = IN0(1);
    float   damp = IN0(2);
    float   hp = IN0(3);
    bool    freeze = IN0(4) > 0.f;
    float   diffusion = IN0(5);
    
    float   *outL = OUT(0);
    float   *outR = OUT(1);
    
    size_t size = inNumSamples;
    Reverb *reverb = &unit->reverb_;
    
    // find out number of audio inputs
    uint16 numAudioInputs = unit->mNumInputs - kNumArgs;
    
    
    if(numAudioInputs == 1)
        Copy(inNumSamples, OUT(1), IN(kNumArgs));
    
    else if (numAudioInputs > 2) {
        for(int i=2; i<numAudioInputs; ++i) {
            uint16 out_chn = i & 1;
            Accum(inNumSamples, OUT(out_chn), IN(i+kNumArgs));
        }
    }

    
    if(freeze) {
        unit_freeze(unit);
        // if 'freeze' is on, we want no direct signal // hm, rather not...
        //memset(outL, 0, size*sizeof(float));
        //memset(outR, 0, size*sizeof(float));
    }
    else {
        CONSTRAIN(time, 0.f, 1.25f);        // allow for a little distortion :)
        damp = 1.01f - damp;
        CONSTRAIN(damp, 0.f, 1.f);
        
        reverb->set_time(time);
        reverb->set_lp(damp);
        reverb->set_input_gain(unit->input_gain);
    }
    
    CONSTRAIN(drywet, 0.f, 1.f);
    CONSTRAIN(diffusion, 0.f, 0.95f);   // TODO: check parameter range
    CONSTRAIN(hp, 0.f, 1.f);
    hp *= hp;
    hp = 1.f - hp;
    
    reverb->set_amount(drywet);
    reverb->set_diffusion(diffusion);
    reverb->set_hp(hp);
    
    
    reverb->Process(outL, outR, size);
    
    SoftClip_block(outL, size);
    SoftClip_block(outR, size);

}




PluginLoad(MiVerb) {
    ft = inTable;
    DefineDtorUnit(MiVerb);
}

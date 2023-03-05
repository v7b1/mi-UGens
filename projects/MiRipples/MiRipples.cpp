
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


// A clone of mutable instruments' 'Ripples' Eurorack module
// by volker böhm, july 2020, https://vboehm.net
// based on the virtual analog model provided by
// Alright Devices https://www.alrightdevices.com/
// done for VCVrack https://vcvrack.com/

// Original module conceived by Émilie Gillet, https://mutable-instruments.net/





#include "SC_PlugIn.h"

#include "Ripples/ripples.hpp"

static InterfaceTable *ft;


struct MiRipples : public Unit {
    
    ripples::RipplesEngine  *engine;
    ripples::RipplesEngine::Frame frame;
    float                   sr;
    float                   drive;
    
};


static void MiRipples_Ctor(MiRipples *unit);
static void MiRipples_Dtor(MiRipples *unit);
static void MiRipples_next(MiRipples *unit, int inNumSamples);



static void MiRipples_Ctor(MiRipples *unit) {
    
    unit->engine = new ripples::RipplesEngine;
    unit->engine->setSampleRate(SAMPLERATE);
    
    unit->drive = 1.0;
    
    unit->frame.freq_cv = 0.f;
    unit->frame.res_cv = 0.f;
    unit->frame.fm_cv = 0.f;
    unit->frame.fm_knob = 0.f;
    unit->frame.gain_cv = 0.f;
    unit->frame.gain_cv_present = false;
    
    SETCALC(MiRipples_next);        // tells sc synth the name of the calculation function
    MiRipples_next(unit, 1);
}


static void MiRipples_Dtor(MiRipples *unit) {
    if(unit->engine)
        delete unit->engine;
}



inline void SoftLimit_block(float *inout, float drive, size_t size) {
    while(size--) {
        *inout *= drive;
        float x2 = (*inout)*(*inout);
        *inout *= (27.f + x2) / (27.f + 9.f * x2);
        inout++;
    }
}


#pragma mark ----- dsp loop -----

void MiRipples_next( MiRipples *unit, int inNumSamples )
{
    float   *in = IN(0);
    float   *cf = IN(1);
    float   reson = IN0(2);
    float   drive = IN0(3);
    
    float   *out1 = OUT(0);
    
    ripples::RipplesEngine  *engine = unit->engine;
    ripples::RipplesEngine::Frame *frame = &unit->frame;

    frame->res_knob = reson;

    if(INRATE(1) == calc_FullRate) {
        
        for (size_t i = 0; i < inNumSamples; ++i) {
            frame->input = in[i] * 10.f;   // vb, scale up to -10 .. +10 input range
            frame->freq_knob = cf[i];
            engine->process(*frame);
            
            out1[i] = frame->lp4;
        }
    }
    
    else {
        
        frame->freq_knob = cf[0];
        
        for (size_t i = 0; i < inNumSamples; ++i) {
            frame->input = in[i] * 10.f;   // vb, scale up to -10 .. +10 input range

            engine->process(*frame);
            
            out1[i] = frame->lp4;
        }
    }
    
    if(drive > 1.0)
        SoftLimit_block(out1, drive, inNumSamples);
    
}


PluginLoad(MiRipples) {
    ft = inTable;
    DefineDtorUnit(MiRipples);
}


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
 
 MiOmi - based on Elements' easteregg mode:
 ominouse: "a dark 2x2-op FM synth" by
 https://mutable-instruments.net/
 
 Original code by Émilie Gillet
 
 https://vboehm.net
 
 */



#include "SC_PlugIn.h"

#include "omi/dsp/part.h"

static InterfaceTable *ft;


const uint16 kAudioBlockSize = omi::kMaxBlockSize;   // sig vs can't be smaller than this!



struct MiOmi : public Unit {
    
    omi::Part      *part;
    omi::PerformanceState ps;
    float           *silence;
    
};



static void MiOmi_Ctor(MiOmi *unit);
static void MiOmi_Dtor(MiOmi *unit);
static void MiOmi_next(MiOmi *unit, int inNumSamples);


static void MiOmi_Ctor(MiOmi *unit) {
    
    if(BUFLENGTH < kAudioBlockSize) {
        Print("MiOmi ERROR: Block Size can't be smaller than %d samples\n", kAudioBlockSize);
        unit = NULL;
        return;
    }
    
    unit->ps.note = 48;
    unit->ps.strength = 0.5;
    unit->ps.modulation = 0.0;
    unit->ps.gate = 0;
    
    unit->silence = (float*)RTAlloc(unit->mWorld, kAudioBlockSize*sizeof(float));
    memset(unit->silence, 0, kAudioBlockSize*sizeof(float));
    
    // Init and seed the random parameters and generators with the serial number.
    unit->part = new omi::Part;
    unit->part->Init(SAMPLERATE);
    
    uint32_t mySeed = 0x1fff7a10;
    unit->part->Seed(&mySeed, 3);
    
    SETCALC(MiOmi_next);
    ClearUnitOutputs(unit, 1);
    //MiOmi_next(unit, 1);
    
}


static void MiOmi_Dtor(MiOmi *unit) {
    
    delete unit->part;
    RTFree(unit->mWorld, unit->silence);
}


inline void SoftLimit_block(MiOmi *unit, float *inout, size_t size)
{
    while(size--) {
        float x = *inout * 0.5f;
        float x2 = x * x;
        *inout = x * (27.0f + x2) / (27.0f + 9.0f * x2);
        inout++;
    }
}


#pragma mark ----- dsp loop -----

void MiOmi_next( MiOmi *unit, int inNumSamples )
{
    float   *audio_in = IN(0);
    float   *gate_in = IN(1);

    float   pitch = IN0(2);
    float   contour = IN0(3);
    float   detune = IN0(4);
    float   level1 = IN0(5);
    float   level2 = IN0(6);
    float   ratio1 = IN0(7);
    float   ratio2 = IN0(8);
    float   fm1 = IN0(9);
    float   fm2 = IN0(10);
    float   fb = IN0(11);
    float   xfb = IN0(12);
    
    float   filter_mode = IN0(13);
    float   cutoff = IN0(14);
    float   resonance = IN0(15);
    float   strength = IN0(16);
    float   filter_env = IN0(17);
    float   rotate = IN0(18);
    float   space = IN0(19);

    
    float   *outL = OUT(0);
    float   *outR = OUT(1);
    
    int vs = inNumSamples;
    size_t size = omi::kMaxBlockSize;
    omi::PerformanceState *ps = &unit->ps;
    omi::Patch *p = unit->part->mutable_patch();
    
    
    CONSTRAIN(contour, 0.f, 1.f);
    p->exciter_envelope_shape = contour;
    
    CONSTRAIN(detune, 0.f, 1.f);
    p->exciter_bow_level = detune;
    
    CONSTRAIN(level1, 0.f, 1.f);
    p->exciter_blow_level = level1;
    
    CONSTRAIN(level2, 0.f, 1.f);
    p->exciter_strike_level = level2;
    
    CONSTRAIN(ratio1, 0.f, 1.f);
    p->exciter_blow_meta = ratio1;
    
    CONSTRAIN(ratio2, 0.f, 1.f);
    p->exciter_strike_meta = ratio2;
    
    CONSTRAIN(fm1, 0.f, 1.f);
    p->exciter_blow_timbre = fm1;
    
    CONSTRAIN(fm2, 0.f, 1.f);
    p->exciter_strike_timbre = fm2;
    
    CONSTRAIN(fb, 0.f, 1.f);
    p->exciter_bow_timbre = fb;
    
    CONSTRAIN(xfb, 0.f, 1.f);
    p->cross_fb = xfb;
    
    
    CONSTRAIN(filter_mode, 0.f, 1.f);
    p->resonator_geometry = filter_mode;
    
    CONSTRAIN(cutoff, 0.f, 1.f);
    p->resonator_brightness = cutoff;
    
    CONSTRAIN(resonance, 0.f, 1.f);
    p->resonance = resonance * 10.f;
    
    CONSTRAIN(filter_env, 0.f, 1.f);
    p->resonator_damping = filter_env;
    
    CONSTRAIN(rotate, 0.f, 1.f);
    p->resonator_position = rotate;
    
    CONSTRAIN(space, 0.f, 1.f);
    p->space = space;

    
    CONSTRAIN(strength, 0.f, 1.f);
    ps->strength = strength;
    
    CONSTRAIN(pitch, 0.f, 127.f);
    ps->note = pitch;

    

    float sum = 0.f;
    if( INRATE(1) == calc_FullRate) {
        for(int i=0; i<vs; i++)
            sum += gate_in[i];
    }
    else
        sum = gate_in[0];
    
    ps->gate = ( sum > 0.f );
    
    if(INRATE(0) == calc_FullRate) {
        for(int count = 0; count < vs; count += size) {
            unit->part->Process(*ps, audio_in+count, outL+count, outR+count, size);
        }
    }
    else {
        for(int count = 0; count < vs; count += size) {
            unit->part->Process(*ps, unit->silence, outL+count, outR+count, size);
        }
    }
    

    SoftLimit_block(unit, outL, vs);
    SoftLimit_block(unit, outR, vs);
}




PluginLoad(MiOmi) {
    ft = inTable;
    DefineDtorUnit(MiOmi);
}

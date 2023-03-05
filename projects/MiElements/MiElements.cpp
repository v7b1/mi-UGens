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
 
 MiElements - a clone of the eurorack module 'Elements' by
 https://mutable-instruments.net/
 
 Original code by Émilie Gillet
 
 https://vboehm.net
 
 */


// TODO: FM


#include "SC_PlugIn.h"

#include "elements/dsp/dsp.h"
#include "elements/dsp/part.h"


float elements::Dsp::kSampleRate = 32000.0f; 
float elements::Dsp::kSrFactor = 32000.0f / kSampleRate;
float elements::Dsp::kIntervalCorrection = logf(kSrFactor)/logf(2.0f)*12.0f;


static InterfaceTable *ft;


struct MiElements : public Unit {
    
    elements::Part              *part;
    elements::PerformanceState  ps;
    elements::Patch             *p;
    
    float               *blow_in;
    float               *strike_in;
    float               *silence;

    uint16_t            *reverb_buffer;
    float               *out, *aux;     // output buffers
    double              sr;
    long                sigvs;
    
    short               blockCount;
    
};


static void MiElements_Ctor(MiElements *unit);
static void MiElements_Dtor(MiElements *unit);
static void MiElements_next(MiElements *unit, int inNumSamples);


static void MiElements_Ctor(MiElements *unit) {
    
    if(BUFLENGTH < elements::kMaxBlockSize) {
        Print("MiElements ERROR: Block Size can't be smaller than %d samples\n", elements::kMaxBlockSize);
        unit = NULL;
        return;
    }
    
    elements::Dsp::setSr(SAMPLERATE);
    
    
    // allocate memory
    unit->reverb_buffer = (uint16_t*)RTAlloc(unit->mWorld, 32768*sizeof(uint16_t));
    
    if(unit->reverb_buffer == NULL) {
        Print("MiElements ERROR: mem alloc failed!\n");
        unit = NULL;
        return;
    }
    
    unit->out = (float *)RTAlloc(unit->mWorld, BUFLENGTH*sizeof(float));
    unit->aux = (float *)RTAlloc(unit->mWorld, BUFLENGTH*sizeof(float));
    
    unit->silence = (float *)RTAlloc(unit->mWorld, BUFLENGTH*sizeof(float));
    memset(unit->silence, 0, BUFLENGTH*sizeof(float));

    
    // Init and seed the random parameters and generators with the serial number.
    unit->part = new elements::Part;
    memset(unit->part, 0, sizeof(*unit->part));
    unit->part->Init(unit->reverb_buffer);
    uint32_t mySeed = 0x1fff7a10;
    unit->part->Seed(&mySeed, 3);
    
    unit->part->set_easter_egg(false);   
    unit->p = unit->part->mutable_patch();
    
    unit->blockCount = 0;
    
    unit->p->exciter_envelope_shape = 0.1f;
    unit->p->exciter_bow_level = 0.f;
    unit->p->exciter_blow_level = 0.f;
    unit->p->exciter_strike_level = 0.5f;
    unit->p->exciter_blow_meta = 0.0f;
    unit->p->exciter_strike_meta = 0.5f;
    unit->p->exciter_bow_timbre = 0.f;
    unit->p->exciter_blow_timbre = 0.f;
    unit->p->exciter_strike_timbre = 0.5f;
    
    unit->p->resonator_geometry = 0.2f;
    unit->p->resonator_brightness = 0.3f;
    unit->p->resonator_damping = 0.7f;
    
    unit->ps.strength = 0.5f;
    unit->ps.gate = 0;
    
    unit->part->set_resonator_model(elements::RESONATOR_MODEL_MODAL);
    
    
    SETCALC(MiElements_next);
    //MiElements_next(unit, 64);       // do we reallly need this?
    
}


static void MiElements_Dtor(MiElements *unit) {
    
    if(unit->reverb_buffer) {
        RTFree(unit->mWorld, unit->reverb_buffer);
    }
    if(unit->part)
        delete unit->part;
    if(unit->silence)
       RTFree(unit->mWorld, unit->silence);
    if(unit->out)
        RTFree(unit->mWorld, unit->out);
    if(unit->aux)
        RTFree(unit->mWorld, unit->aux);
}


inline void SoftLimit_block(MiElements *unit, float *inout, size_t size)
{
    while(size--) {
        float x = *inout * 0.5f;
        float x2 = x * x;
        *inout = x * (27.f + x2) / (27.f + 9.f * x2);
        inout++;
    }
}

inline void SoftLimit_block2(MiElements *unit, float *in, float *out, size_t size)
{
    while(size--) {
        float x = *in++ * 0.5f;
        float x2 = x * x;
        *out++ = x * (27.f + x2) / (27.f + 9.f * x2);
    }
}

#pragma mark ----- dsp loop -----

void MiElements_next( MiElements *unit, int inNumSamples)
{
    float   *in0 = IN(0);
    float   *in1 = IN(1);
    float   *gate_in = IN(2);
    
    float   voct_in = IN0(3);
    float   strength = IN0(4);
    float   contour = IN0(5);
    float   bow_level = IN0(6);
    float   blow_level = IN0(7);
    float   strike_level = IN0(8);
    float   flow = IN0(9);
    float   mallet = IN0(10);
    float   bow_timbre = IN0(11);
    float   blow_timbre = IN0(12);
    float   strike_timbre = IN0(13);
    float   geometry = IN0(14);
    float   brightness = IN0(15);
    float   damping = IN0(16);
    float   position = IN0(17);
    float   space = IN0(18);
    int     model = IN0(19);
    bool    easter_egg = IN0(20) > 0.f;
    
    float   *outL = OUT(0);
    float   *outR = OUT(1);
    
    size_t  size = elements::kMaxBlockSize;
    
    float   *blow_in = unit->blow_in;
    float   *strike_in = unit->strike_in;
    float   *out = unit->out;
    float   *aux = unit->aux;
    
    elements::PerformanceState ps = unit->ps;
    elements::Patch            *p = unit->p;

    
    // set resonator model:
    // RESONATOR_MODEL_MODAL, RESONATOR_MODEL_STRING, RESONATOR_MODEL_STRINGS
    CONSTRAIN(model, 0, 2);
    unit->part->set_resonator_model(static_cast<elements::ResonatorModel>(model));
    
    // set pitch
    CONSTRAIN(voct_in, 8.f, 120.f);
    ps.note = voct_in;
    
    CONSTRAIN(strength, 0.f, 1.f);
    ps.strength = strength;
    
    // set level params
    CONSTRAIN(contour, 0.0f, 0.9995f);
    p->exciter_envelope_shape = contour;

    CONSTRAIN(bow_level, 0.0f, 0.9995f);
    p->exciter_bow_level = bow_level;
    
    CONSTRAIN(blow_level, 0.0f, 0.9995f);
    p->exciter_blow_level = blow_level;
    
    CONSTRAIN(strike_level, 0.0f, 0.9995f);
    p->exciter_strike_level = strike_level;
    
    // set meta params
    CONSTRAIN(flow, 0.0f, 0.9995f);
    p->exciter_blow_meta = flow;
    
    CONSTRAIN(mallet, 0.0f, 0.9995f);
    p->exciter_strike_meta = mallet;
    
    // set timbre params
    CONSTRAIN(bow_timbre, 0.0f, 0.9995f);
    p->exciter_bow_timbre = bow_timbre;
    
    CONSTRAIN(blow_timbre, 0.0f, 0.9995f);
    p->exciter_blow_timbre = blow_timbre;
    
    CONSTRAIN(strike_timbre, 0.0f, 0.9995f);
    p->exciter_strike_timbre = strike_timbre;
    
    // set resonator params
    CONSTRAIN(geometry, 0.0f, 0.9995f);
    p->resonator_geometry = geometry;
    
    CONSTRAIN(brightness, 0.0f, 0.9995f);
    p->resonator_brightness = brightness;
    
    CONSTRAIN(damping, 0.0f, 0.9995f);
    p->resonator_damping = damping;
    
    CONSTRAIN(position, 0.0f, 0.9995f);
    p->resonator_position = position;
    
    CONSTRAIN(space, 0.0f, 0.9995f);
    p->space = space * 1.11f; // vb, use the last bit to trigger rev freeze

    
    unit->part->set_easter_egg(easter_egg);
    
    // gate input
    if(INRATE(2) == calc_FullRate) {
        float sum = 0.f;
        for(int i=0; i<inNumSamples; ++i) {
            sum += gate_in[i];
        }
        ps.gate = sum > 0.001f;
    }
    else
        ps.gate = gate_in[0] > 0.f;
    
    
    // check input rates
    if(INRATE(0) == calc_FullRate)
        blow_in = in0;
    else
        blow_in = unit->silence;
    
    if(INRATE(1) == calc_FullRate)
        strike_in = in1;
    else
        strike_in = unit->silence;
    
    
    // input and output can't be the same arrays
    
    for(size_t count = 0; count < inNumSamples; count += size) {
        
        unit->part->Process(ps, blow_in+count, strike_in+count, out+count, aux+count, size);
    }
    
    SoftLimit_block2(unit, out, outL, inNumSamples);
    SoftLimit_block2(unit, aux, outR, inNumSamples);
    
}


PluginLoad(MiElements) {
    ft = inTable;
    DefineDtorUnit(MiElements);
}




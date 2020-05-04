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


// TODO: easteregg, FM


#include "SC_PlugIn.h"

#include "elements/dsp/dsp.h"
#include "elements/dsp/part.h"


float elements::Dsp::kSampleRate = 32000.0f; //44100.0f; //48000.0f;
float elements::Dsp::kSrFactor = 32000.0f / kSampleRate;
float elements::Dsp::kIntervalCorrection = logf(kSrFactor)/logf(2.0f)*12.0f;


static InterfaceTable *ft;


struct MiElements : public Unit {
    
    elements::Part              *part;
    elements::PerformanceState  ps;
    elements::Patch             *p;
    
    float               blow_in[elements::kMaxBlockSize];
    float               strike_in[elements::kMaxBlockSize];
    
    uint16_t            *reverb_buffer;
    void                *info_out;
    float               *out, *aux;     // output buffers
    double              sr;
    long                sigvs;
    
    short               blockCount;
    
};


static void MiElements_Ctor(MiElements *unit);
static void MiElements_Dtor(MiElements *unit);
static void MiElements_next(MiElements *unit, int inNumSamples);


static void MiElements_Ctor(MiElements *unit) {
    
    
    elements::Dsp::setSr(SAMPLERATE);
    
    
    // allocate memory
    unit->reverb_buffer = (uint16_t*)RTAlloc(unit->mWorld, 32768*sizeof(uint16_t)); // 65536
    
    if(unit->reverb_buffer == NULL) {
        Print("mem alloc failed!");
        unit = NULL;
        ClearUnitOutputs(unit, BUFLENGTH); // ??
        return;
    }
    
    unit->out = (float *)RTAlloc(unit->mWorld, 4096*sizeof(float));
    unit->aux = (float *)RTAlloc(unit->mWorld, 4096*sizeof(float));
    
    // Init and seed the random parameters and generators with the serial number.
    unit->part = new elements::Part;
    memset(unit->part, 0, sizeof(*unit->part));
    unit->part->Init(unit->reverb_buffer);
    //unit->part->Seed((uint32_t*)(0x1fff7a10), 3);
    uint32_t mySeed = 0x1fff7a10;
    unit->part->Seed(&mySeed, 3);
    
    unit->part->set_easter_egg(false);
    unit->p = unit->part->mutable_patch();
    
    unit->blockCount = 0;
    
    
    /*
    // check input rates
    if(INRATE(0) == calc_FullRate)
        unit->performance_state.internal_exciter = false;
    else
        unit->performance_state.internal_exciter = true;
    
    if(INRATE(1) == calc_ScalarRate)
        unit->performance_state.internal_strum = true;
    else
        unit->performance_state.internal_strum = false;
    
    if(INRATE(2) == calc_ScalarRate)
        unit->performance_state.internal_note = true;
    else
        unit->performance_state.internal_note = false;
*/
    
    
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
    if(unit->out)
        RTFree(unit->mWorld, unit->out);
    if(unit->aux)
        RTFree(unit->mWorld, unit->aux);
}


#pragma mark ----- dsp loop -----

void MiElements_next( MiElements *unit, int inNumSamples)
{
    float   *in0 = IN(0);
    float   *in1 = IN(1);
    
    float   gate_in = IN0(2);
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
    CONSTRAIN(voct_in, 20.f, 120.f);
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

    
    // gate input
    ps.gate = gate_in > 0.f;
    
    
    int kend = inNumSamples / size;
    
    for(size_t k = 0; k < kend; ++k) {
        int offset = k * size;
        
        if(INRATE(0) == calc_FullRate)
            memcpy(blow_in, in0+offset, size*sizeof(float));
        else
            memset(blow_in, 0, size*sizeof(float));
        
        if(INRATE(1) == calc_FullRate)
            memcpy(strike_in, in1+offset, size*sizeof(float));
        else
            memset(strike_in, 0, size*sizeof(float));
        
        
        unit->part->Process(ps, blow_in, strike_in, out, aux, size);
        
        for (size_t i = 0; i < size; ++i) {
            int index = i + offset;
            outL[index] = stmlib::SoftLimit(out[i]*0.5);
            outR[index] = stmlib::SoftLimit(aux[i]*0.5);
        }
        
    }
    
    
}


PluginLoad(MiElements) {
    ft = inTable;
    DefineDtorUnit(MiElements);
}




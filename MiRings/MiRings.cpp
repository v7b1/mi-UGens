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
 
 MiRings - a clone of the eurorack module 'Rings' by
 https://mutable-instruments.net/
 
 Original code by Émilie Gillet
 
 https://vboehm.net
 
 */


#include "SC_PlugIn.h"

#include "rings/dsp/part.h"
#include "rings/dsp/strummer.h"
#include "rings/dsp/string_synth_part.h"
#include "rings/dsp/dsp.h"


float rings::Dsp::sr = 48000.0;
float rings::Dsp::a3 = 440.0 / 48000.0;
int kBlockSize = 64;
int kMaxBlockSize = rings::kMaxBlockSize;


static InterfaceTable *ft;


struct MiRings : public Unit {
    
    rings::Part             part;
    rings::StringSynthPart  string_synth;
    rings::Strummer         strummer;
    rings::PerformanceState performance_state;
    rings::Patch            patch;
    
    uint16_t                *reverb_buffer;
    
    bool                    prev_trig;
    int                     prev_poly;
    //int                     bcount;
    
};


static void MiRings_Ctor(MiRings *unit);
static void MiRings_Dtor(MiRings *unit);
static void MiRings_next(MiRings *unit, int inNumSamples);


static void MiRings_Ctor(MiRings *unit) {
    
    rings::Dsp::setSr(SAMPLERATE);

    // TODO: fix changing block sizes - code works for 64 samples
    //Print("sc block size: %d\n", BUFLENGTH);
    if(BUFLENGTH > kMaxBlockSize) {
        Print("MiRings ERROR: sc block size too big!\n");
        return;
    }
    /*else if (BUFLENGTH < kBlockSize) {
        Print("MiRings ERROR: sc block size too small!\n");
        return;
    }*/
    
    kBlockSize = BUFLENGTH;
        
    
    // allocate memory + init with zeros
    unit->reverb_buffer = (uint16_t*)RTAlloc(unit->mWorld, 32768*sizeof(uint16_t));
    memset(unit->reverb_buffer, 0, 32768*sizeof(uint16_t));
    
    // zero out...
    memset(&unit->strummer, 0, sizeof(unit->strummer));
    memset(&unit->part, 0, sizeof(unit->part));
    memset(&unit->string_synth, 0, sizeof(unit->string_synth));

    unit->strummer.Init(0.01, rings::Dsp::getSr() / kBlockSize);
    unit->part.Init(unit->reverb_buffer);
    unit->string_synth.Init(unit->reverb_buffer);
    
    unit->part.set_polyphony(1);
    unit->part.set_model(rings::RESONATOR_MODEL_MODAL);

    unit->string_synth.set_polyphony(1);
    unit->string_synth.set_fx(rings::FX_FORMANT);
    unit->prev_poly = 0;
    
    unit->performance_state.fm = 0.f;       // TODO: fm not used, maybe later...
    unit->prev_trig = false;
    
    //unit->bcount = 0;
    
    
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

    // TODO: check out ClearUnitOutputs
//    ClearUnitOutputs(unit, 1);
    
    SETCALC(MiRings_next);
    //MiRings_next(unit, 64);       // do we reallly need this?
    
}


static void MiRings_Dtor(MiRings *unit) {
    
    if(unit->reverb_buffer) {
        RTFree(unit->mWorld, unit->reverb_buffer);
    }
}


#pragma mark ----- dsp loop -----

void MiRings_next( MiRings *unit, int inNumSamples)
{
    float   *in = IN(0);
    float   *trig_in = IN(1);
    
    float   voct_in = IN0(2);
    float   struct_in = IN0(3);
    float   bright_in = IN0(4);
    float   damp_in = IN0(5);
    float   pos_in = IN0(6);
    short   model = IN0(7);
    short   polyphony = IN0(8);
    bool    intern_exciter = (IN0(9) > 0.f);
    bool    easter_egg = (IN0(10) > 0.f);
    bool    bypass = (IN0(11) > 0.f);
    
    float *out1 = OUT(0);
    float *out2 = OUT(1);
    
    rings::Patch *patch = &unit->patch;
    float   input[BUFLENGTH];
    size_t  size = inNumSamples; //kBlockSize; 
    

    // check input rates for excitation input
    if(INRATE(0) == calc_FullRate) {
        // we have to copy input vector (why?)
        std::copy(&in[0], &in[size], &input[0]);
        // intern_exciter should be off, but user can override
        unit->performance_state.internal_exciter = intern_exciter;
    }
    else {
        // if there's no audio input, set input to zero...
        memset(input, 0, size*sizeof(float));
        // ... and use internal exciter!
        unit->performance_state.internal_exciter = true;
    }
    
    // set resonator model
    CONSTRAIN(model, 0, 5);
    unit->part.set_model(static_cast<rings::ResonatorModel>(model));
    unit->string_synth.set_fx(static_cast<rings::FxType>(model));
    
    // set polyphony
    if(polyphony != unit->prev_poly) {
        CONSTRAIN(polyphony, 1, 4);
        unit->part.set_polyphony(polyphony);
        unit->string_synth.set_polyphony(polyphony);
        unit->prev_poly = polyphony;
    }
    
    // set pitch
    CONSTRAIN(voct_in, 0.f, 114.f);
    unit->performance_state.tonic = 12.f;
    unit->performance_state.note = voct_in;
    
    
    // set params
    CONSTRAIN(struct_in, 0.0f, 1.0f);    //0.9995f
    patch->structure = struct_in;

    float chord = struct_in * (rings::kNumChords - 1);
    unit->performance_state.chord = roundf(chord);

    CONSTRAIN(bright_in, 0.0f, 1.0f);
    patch->brightness = bright_in;

    CONSTRAIN(damp_in, 0.0f, 1.0f);
    patch->damping = damp_in;
    
    CONSTRAIN(pos_in, 0.0f, 1.0f);
    patch->position = pos_in;

    
    // check trigger input
    if(!unit->performance_state.internal_strum) {
        
        bool trig = false;
        bool prev_trig = unit->prev_trig;
        float sum = 0.f;
        
        if(INRATE(1) == calc_FullRate) {    // trigger input is audio rate
            // TODO: use vDSP for the summation
            for(int i=0; i<inNumSamples; ++i)
                sum += trig_in[i];
            trig = (sum > 0.f);
        }
        else {          // trigger input is control or scalar rate
            trig = (trig_in[0] > 0.f);
        }
        
        if(trig) {
            if(!prev_trig)
                unit->performance_state.strum = true;
            else
                unit->performance_state.strum = false;
        }
        unit->prev_trig = trig;
        
    }

    unit->part.set_bypass(bypass);
    
    if(easter_egg) {
        unit->strummer.Process(NULL, size, &unit->performance_state);
        unit->string_synth.Process(unit->performance_state, unit->patch, input, out1, out2, size);
    } else {
        unit->strummer.Process(input, size, &unit->performance_state);
        unit->part.Process(unit->performance_state, unit->patch, input, out1, out2, size);
    }
    
}


PluginLoad(MiRings) {
    ft = inTable;
    DefineDtorUnit(MiRings);
}




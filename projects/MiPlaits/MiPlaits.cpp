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
 
 MiPlaits - a clone of the eurorack module 'Plaits' by
 https://mutable-instruments.net/
 
 Original code by Émilie Gillet
 
 https://vboehm.net
 
 */

#include "SC_PlugIn.h"

#include "plaits/dsp/dsp.h"
#include "plaits/dsp/voice.h"


float kSampleRate = 48000.0f;
float a0 = (440.0f / 8.0f) / kSampleRate;
const size_t   kBlockSize = plaits::kBlockSize;

static InterfaceTable *ft;


struct MiPlaits : public Unit {
    
    plaits::Voice       *voice_;
    plaits::Modulations modulations;
    plaits::Patch       patch;
    float               transposition_;
    float               octave_;
    short               trigger_connected;
    short               trigger_toggle;
    
    char                *shared_buffer;
    void                *info_out;
    bool                prev_trig;
    float               sr;
    int                 sigvs;
};


static void MiPlaits_Ctor(MiPlaits *unit);
static void MiPlaits_Dtor(MiPlaits *unit);
static void MiPlaits_next(MiPlaits *unit, int inNumSamples);



static void MiPlaits_Ctor(MiPlaits *unit) {
    
    if (BUFLENGTH < kBlockSize) {
        Print("MiPlaits ERROR: block size can't be smaller than %d samples!\n", kBlockSize);
        unit = NULL;
        return;
    }
    
    kSampleRate = SAMPLERATE;
    a0 = (440.0f / 8.0f) / kSampleRate;

    // init some params
    unit->transposition_ = 0.;
    unit->octave_ = 0.5;
    unit->patch.note = 48.0;
    unit->patch.harmonics = 0.1;
        
    
    // allocate memory
    unit->shared_buffer = (char*)RTAlloc(unit->mWorld, 32768);
    // init with zeros
    memset(unit->shared_buffer, 0, 32768);

    if(unit->shared_buffer == NULL) {
        Print("MiPlaits ERROR: mem alloc failed!");
        unit = NULL;
    }
    stmlib::BufferAllocator allocator(unit->shared_buffer, 32768);

    unit->voice_ = new plaits::Voice;
    unit->voice_->Init(&allocator);
    
    
    memset(&unit->patch, 0, sizeof(unit->patch));
    memset(&unit->modulations, 0, sizeof(unit->modulations));
    
    unit->prev_trig = false;
    
    unit->modulations.timbre_patched = (INRATE(3) != calc_ScalarRate);
    unit->modulations.morph_patched = (INRATE(4) != calc_ScalarRate);
    unit->modulations.trigger_patched = (INRATE(5) != calc_ScalarRate);
    unit->modulations.level_patched = (INRATE(6) != calc_ScalarRate);
    
    // TODO: we don't have an fm input yet.
    unit->modulations.frequency_patched = false;

    SETCALC(MiPlaits_next);
    //MiPlaits_next(unit, 64);       // do we reallly need this?
    
}


static void MiPlaits_Dtor(MiPlaits *unit) {
    
    delete unit->voice_;
    if(unit->shared_buffer) {
        RTFree(unit->mWorld, unit->shared_buffer);
    }
}


#pragma mark ----- dsp loop -----

void MiPlaits_next( MiPlaits *unit, int inNumSamples)
{
    float voct = IN0(0);
    float engine_in = IN0(1);
    
    float harm_in = IN0(2);
    float timbre_in = IN0(3);
    float morph_in = IN0(4);
    float *trig_in = IN(5);
    float *level_in = IN(6);
    float fm_mod = IN0(7);
    float timb_mod = IN0(8);
    float morph_mod = IN0(9);
    float decay_in = IN0(10);
    float lpgColor_in = IN0(11);
    
    float *out = OUT(0);
    float *aux = OUT(1);

    
    // TODO: check setting pitch
    float pitch = fabs(voct);
    CONSTRAIN(pitch, 0.f, 127.f);
    unit->patch.note = pitch;
    
    int engine = int(engine_in);
    CONSTRAIN(engine, 0, 15);      // 16 engines
    unit->patch.engine = engine;

    CONSTRAIN(harm_in, 0.0f, 1.0f);
    unit->patch.harmonics = harm_in;

    CONSTRAIN(timbre_in, 0.0f, 1.0f);
    unit->patch.timbre = timbre_in;

    CONSTRAIN(morph_in, 0.0f, 1.0f);
    unit->patch.morph = morph_in;
    
    CONSTRAIN(fm_mod, -1.0f, 1.0f);
    unit->patch.frequency_modulation_amount = fm_mod;
    
    CONSTRAIN(timb_mod, -1.0f, 1.0f);
    unit->patch.timbre_modulation_amount = timb_mod;
    
    CONSTRAIN(morph_mod, -1.0f, 1.0f);
    unit->patch.morph_modulation_amount = morph_mod;
    
    CONSTRAIN(decay_in, 0.0f, 1.0f);
    unit->patch.decay = decay_in;
    
    CONSTRAIN(lpgColor_in, 0.0f, 1.0f);
    unit->patch.lpg_colour = lpgColor_in;
    
    
    if (INRATE(5) != calc_ScalarRate) {
        
        unit->modulations.trigger_patched = true;
        float sum = 0.f;
        
        if (INRATE(5) == calc_FullRate) {
                        // trigger input is audio rate
            // TODO: add vDSP vector summation 
            for(int i=0; i<inNumSamples; ++i)
                sum += trig_in[i];
        }
        else {          // trigger input is control rate
            sum = trig_in[0];
        }
        
        unit->modulations.trigger = sum;
    }
    else {
        unit->modulations.trigger_patched = false;
    }
    
    if (INRATE(6) != calc_ScalarRate) {
        unit->modulations.level_patched = true;
        unit->modulations.level = level_in[0];
    }
    else
        unit->modulations.level_patched = false;
    
    
    for(int count = 0; count < inNumSamples; count += kBlockSize) {
        
        unit->voice_->Render(unit->patch, unit->modulations, out+count, aux+count, kBlockSize);

    }
    
}



PluginLoad(MiPlaits) {
    ft = inTable;
    DefineDtorUnit(MiPlaits);
}




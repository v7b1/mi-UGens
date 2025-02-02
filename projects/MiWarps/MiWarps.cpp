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
 
 MiWarps - a clone of the eurorack module 'Warps' by
 https://mutable-instruments.net/
 
 Original code by Émilie Gillet
 
 https://vboehm.net
 
 */


#include "SC_PlugIn.h"

#include "warps/dsp/modulator.h"



const size_t kBlockSize = 60; //    96;

static InterfaceTable *ft;


struct MiWarps : public Unit {
    
    warps::Modulator    *modulator;
    short               patched[2];
    
    warps::FloatFrame   *input;
    warps::FloatFrame   *output;
    float               *silence;
    
    uint16_t            count;
    
};


static void MiWarps_Ctor(MiWarps *unit);
static void MiWarps_Dtor(MiWarps *unit);
static void MiWarps_next(MiWarps *unit, int inNumSamples);


static void MiWarps_Ctor(MiWarps *unit) {
/*
    if (BUFLENGTH < kBlockSize) {
        Print("MiWarps ERROR: sc block size too small!\n");
        unit = NULL;
        return;
    }
 */
    
    unit->modulator = new warps::Modulator;
    memset(unit->modulator, 0, sizeof(*unit->modulator));
    unit->modulator->Init(SAMPLERATE);
    

    unit->input = (warps::FloatFrame*)RTAlloc(unit->mWorld, warps::kMaxBlockSize*sizeof(warps::FloatFrame));
    unit->output = (warps::FloatFrame*)RTAlloc(unit->mWorld, warps::kMaxBlockSize*sizeof(warps::FloatFrame));
    memset(unit->output, 0, warps::kMaxBlockSize*sizeof(warps::FloatFrame));
    unit->silence = (float*)RTAlloc(unit->mWorld, warps::kMaxBlockSize*sizeof(float));
    memset(unit->silence, 0, warps::kMaxBlockSize*sizeof(float));
    
    unit->count = 0;
    
    SETCALC(MiWarps_next);
    MiWarps_next(unit, 64);

}


static void MiWarps_Dtor(MiWarps *unit) {
    
    if(unit->input)
        RTFree(unit->mWorld, unit->input);
    if(unit->output)
        RTFree(unit->mWorld, unit->output);
    if(unit->silence)
        RTFree(unit->mWorld, unit->silence);
    
    if(unit->modulator)
        delete unit->modulator;

}



void MiWarps_next( MiWarps *unit, int inNumSamples)
{
    float   *carrier = IN(0);
    float   *modulator = IN(1);
    
    float   level1 = IN0(2);
    float   level2 = IN0(3);
    float   algorithm = IN0(4) * 0.125f;
    float   timbre = IN0(5);
    short   osc_shape = IN0(6);
    float   freq = IN0(7);
    float   pre_gain = IN0(8);
    bool    easter_egg = (IN0(9) > 0.f);
    
    float   *out = OUT(0);
    float   *aux = OUT(1);
    
    uint16_t count = unit->count;
    
    warps::FloatFrame  *input = unit->input;
    warps::FloatFrame  *output = unit->output;
    
    warps::Parameters *p = unit->modulator->mutable_parameters();
    
    CONSTRAIN(level1, 0.f, 1.f);
    p->channel_drive[0] = level1*level1;
    
    CONSTRAIN(level2, 0.f, 1.f);
    p->channel_drive[1] = level2*level2;
    
    CONSTRAIN(algorithm, 0.f, 1.f);
    p->modulation_algorithm = algorithm;
    
    CONSTRAIN(timbre, 0.f, 1.f);
    p->modulation_parameter = timbre;
    
    CONSTRAIN(osc_shape, 0, 3);
    p->carrier_shape = osc_shape;
    
    CONSTRAIN(freq, 0.0f, 15000.0f);
    p->note = freq;
    
    CONSTRAIN(pre_gain, 1.0f, 10.0f);
    p->limiter_pre_gain = pre_gain;

    // easter_egg mode?
    if(easter_egg) {
        p->frequency_shift_pot = algorithm;
        p->phase_shift = algorithm;
        unit->modulator->set_easter_egg(true);
    }
    else
        unit->modulator->set_easter_egg(false);
    
    
    //unit->modulator->set_bypass(true);
    
    if( INRATE(0) != calc_FullRate)
        carrier = unit->silence;
    if( INRATE(1) != calc_FullRate)
        modulator = unit->silence;
    
    for (int i=0; i<inNumSamples; ++i) {

        input[count].l = carrier[i];
        input[count].r = modulator[i];

        count++;
        if(count >= kBlockSize) {
            unit->modulator->Processf(input, output, kBlockSize);
            count = 0;
        }

        out[i] = output[count].l;
        aux[i] = output[count].r;
    }

    unit->count = count;

}


PluginLoad(MiWarps) {
    ft = inTable;
    DefineDtorUnit(MiWarps);
}




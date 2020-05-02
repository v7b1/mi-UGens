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
 
 MiBraids - a clone of the eurorack module 'Braids' by
 https://mutable-instruments.net/
 
 Original code by Émilie Gillet
 
 https://vboehm.net
 
 */


// TODO: look into 'auto_trigger' and internal env


#include "SC_PlugIn.h"

#include "stmlib/utils/dsp.h"

#include "braids/envelope.h"
#include "braids/macro_oscillator.h"
#include "braids/quantizer.h"
#include "braids/signature_waveshaper.h"
#include "braids/quantizer_scales.h"
#include "braids/vco_jitter_source.h"

#include "Accelerate/Accelerate.h"
#include "samplerate.h"


#define     MI_SAMPLERATE       96000.f
#define     BLOCK_SIZE          32
#define     SAMP_SCALE          (float)(1.0 / 32756.0)


//const uint16_t decimation_factors[] = { 1, 2, 3, 4, 6, 12, 24 };
const uint16_t bit_reduction_masks[] = {
    0xffff,
    0xfff0,
    0xff00,
    0xf800,
    0xf000,
    0xe000,
    0xc000 };


static InterfaceTable *ft;


typedef struct
{
    braids::MacroOscillator *osc;
    
    float       samps[BLOCK_SIZE] ;
    int16_t     buffer[BLOCK_SIZE];
    uint8_t     sync_buffer[BLOCK_SIZE];
    
} PROCESS_CB_DATA ;


struct MiBraids : public Unit {
    
    //braids::VcoJitterSource jitter_source;
    braids::Quantizer   *quantizer;
    braids::SignatureWaveshaper *ws;
    
    bool            last_trig;
    
    // resampler
    SRC_STATE       *src_state;
    PROCESS_CB_DATA pd;
    float           *samples;
    float           ratio;
};



static long src_input_callback(void *cb_data, float **audio);

static void MiBraids_Ctor(MiBraids *unit);
static void MiBraids_Dtor(MiBraids *unit);
static void MiBraids_next(MiBraids *unit, int inNumSamples);
static void MiBraids_next_reduc(MiBraids *unit, int inNumSamples);
static void MiBraids_next_resamp(MiBraids *unit, int inNumSamples);



static void MiBraids_Ctor(MiBraids *unit) {

    unit->ratio = SAMPLERATE / MI_SAMPLERATE;
    //Print("sr ratio: %f\n", unit->ratio);
    
    if (BUFLENGTH < BLOCK_SIZE) {
        Print("MiBraids error: block size can't be smaller than 32 samples!\n");
        ClearUnitOutputs(unit, BUFLENGTH); // ??
        unit = NULL;
        return;
    }
        
    
    unit->pd.osc = new braids::MacroOscillator;
    memset(unit->pd.osc, 0, sizeof(*unit->pd.osc));
    
    unit->pd.osc->Init(SAMPLERATE);
    unit->pd.osc->set_pitch((48 << 7));
    unit->pd.osc->set_shape(braids::MACRO_OSC_SHAPE_VOWEL_FOF);

    
    unit->ws = new braids::SignatureWaveshaper;
    unit->ws->Init(123774);
    
    unit->quantizer = new braids::Quantizer;
    unit->quantizer->Init();
    unit->quantizer->Configure(braids::scales[0]);
    
    //unit->jitter_source.Init();
    
    memset(unit->pd.buffer, 0, sizeof(int16_t)*BLOCK_SIZE);
    memset(unit->pd.sync_buffer, 0, sizeof(unit->pd.sync_buffer));
    memset(unit->pd.samps, 0, sizeof(float)*BLOCK_SIZE);
    
    unit->last_trig = false;
    
    // setup SRC ----------------
    int error;
    int converter = SRC_SINC_FASTEST;       //SRC_SINC_MEDIUM_QUALITY;
    
    
    // Initialize the sample rate converter
    
    if((unit->src_state = src_callback_new(src_input_callback, converter, 1, &error, &unit->pd)) == NULL)
    {
        Print("\n\n MiBraids error: src_new() failed : %s.\n\n", src_strerror (error)) ;
        unit = NULL;
        return;
    }
    
    unit->samples = (float *)RTAlloc(unit->mWorld, 1024 * sizeof(float));
    
    
    // check resample flag
    int resamp = (int)IN0(5);
    CONSTRAIN(resamp, 0, 2);
    switch(resamp) {
        case 0:
            SETCALC(MiBraids_next);
            Print("resamp: OFF\n");
            break;
        case 1:
            unit->pd.osc->Init(MI_SAMPLERATE);
            SETCALC(MiBraids_next_resamp);
            Print("internal sr: 96kHz - resamp: ON\n");
            break;
        case 2:
            SETCALC(MiBraids_next_reduc);
            Print("resamp: OFF, reduction: ON\n");
            break;
    }
    
    //MiBraids_next(unit, 64);       // do we reallly need this?

    
}


static void MiBraids_Dtor(MiBraids *unit) {
    delete unit->pd.osc;
    delete unit->ws;
    delete unit->quantizer;
    if(unit->samples) {
        RTFree(unit->mWorld, unit->samples);
    }
    if(unit->src_state)
        src_delete(unit->src_state);
}


#pragma mark ----- dsp loop -----

void MiBraids_next( MiBraids *unit, int inNumSamples)
{
    float   voct_in = IN0(0);
    float   timbre_in = IN0(1);
    float   color_in = IN0(2);
    float   model_in = IN0(3);
    float   *trig_in = IN(4);
    
    float   *out = OUT(0);
    
    int16_t *buffer = unit->pd.buffer;
    uint8_t *sync_buffer = unit->pd.sync_buffer;
    size_t  size = BLOCK_SIZE;
    
    braids::MacroOscillator *osc = unit->pd.osc;
    //braids::Quantizer *quantizer = unit->quantizer; // TODO: do we want the quantizer?
    //braids::VcoJitterSource *jitter_source = &unit->jitter_source;

    
    // TODO: check setting pitch
    CONSTRAIN(voct_in, 0.f, 127.f);
    int pit = (int)voct_in;
    float frac = voct_in - pit;
    osc->set_pitch((pit << 7) + (int)(frac * 128.f));
    
    // set parameters
    CONSTRAIN(timbre_in, 0.f, 1.f);
    int16_t timbre = timbre_in * 32767.f;
    CONSTRAIN(color_in, 0.f, 1.f);
    int16_t color = color_in * 32767.f;
    osc->set_parameters(timbre, color);
    
    // set shape/model
    uint8_t shape = (int)(model_in);
    if (shape >= braids::MACRO_OSC_SHAPE_LAST)
        shape -= braids::MACRO_OSC_SHAPE_LAST;
    osc->set_shape(static_cast<braids::MacroOscillatorShape>(shape));
    
    // detect trigger
    if (INRATE(4) != calc_ScalarRate) {
        
        float sum = 0.f;
        
        if (INRATE(4) == calc_FullRate) {  // trigger input is audio rate
            // TODO: add vDSP vector summation
            for(int i=0; i<inNumSamples; ++i)
                sum += trig_in[i];
            
        }
        else {          // trigger input is control rate
            sum = trig_in[0];
        }
        
        bool trigger = (sum != 0.0);
        bool trigger_flag = (trigger && (!unit->last_trig));
        unit->last_trig = trigger;
        
        if(trigger_flag)
            osc->Strike();
        
    }
    
    
    for(int count = 0; count < inNumSamples; count += size) {
        // render
        osc->Render(sync_buffer, buffer, size);
        
        for (auto i = 0; i < size; ++i) {
            out[count + i] = buffer[i] * SAMP_SCALE;
        }
    }
    
}


void MiBraids_next_resamp( MiBraids *unit, int inNumSamples)
{
    float voct_in = IN0(0);
    float timbre_in = IN0(1);
    float color_in = IN0(2);
    float model_in = IN0(3);
    float *trig_in = IN(4);
    
    float *out = OUT(0);
    
    
    braids::MacroOscillator *osc = unit->pd.osc;
    //braids::Quantizer *quantizer = unit->quantizer;
    //braids::VcoJitterSource *jitter_source = &unit->jitter_source;
    SRC_STATE   *src_state = unit->src_state;
    
    float       *samples, *output;
    float       ratio = unit->ratio;
    
    samples = unit->samples;
    
    CONSTRAIN(voct_in, 0.f, 127.f);
    int pit = (int)voct_in;
    float frac = voct_in - pit;
    osc->set_pitch((pit << 7) + (int)(frac * 128.f));
    
    // set parameters
    CONSTRAIN(timbre_in, 0.f, 1.f);
    int16_t timbre = timbre_in * 32767.f;
    CONSTRAIN(color_in, 0.f, 1.f);
    int16_t color = color_in * 32767.f;
    osc->set_parameters(timbre, color);
    
    // set shape/model
    uint8_t shape = (int)(model_in);
    if (shape >= braids::MACRO_OSC_SHAPE_LAST)
        shape -= braids::MACRO_OSC_SHAPE_LAST;
    osc->set_shape(static_cast<braids::MacroOscillatorShape>(shape));
    
    // detect trigger
    if (INRATE(4) != calc_ScalarRate) {
        
        float sum = 0.f;
        
        if (INRATE(4) == calc_FullRate) {
            // trigger input is audio rate
            // TODO: add vDSP vector summation
            for(int i=0; i<inNumSamples; ++i)
                sum += trig_in[i];
            
        }
        else {          // trigger input is control rate
            sum = trig_in[0];
        }
        
        bool trigger = (sum != 0.0);
        bool trigger_flag = (trigger && (!unit->last_trig));
        unit->last_trig = trigger;
        
        if(trigger_flag)
            osc->Strike();
        
    }
    
    
    for(int count = 0; count < inNumSamples; count += BLOCK_SIZE) {
        output = out + count;
        
        // render
        src_callback_read(src_state, ratio, BLOCK_SIZE, output);
    }
    
}



void MiBraids_next_reduc( MiBraids *unit, int inNumSamples)
{
    float   voct_in = IN0(0);
    float   timbre_in = IN0(1);
    float   color_in = IN0(2);
    float   model_in = IN0(3);
    float   *trig_in = IN(4);
    
    uint16_t decim = IN0(6);
    size_t   reduc = IN0(7);
    float    sig_in = IN0(8);
    
    float   *out = OUT(0);
    
    int16_t *buffer = unit->pd.buffer;
    uint8_t *sync_buffer = unit->pd.sync_buffer;
    size_t  size = BLOCK_SIZE;
    
    braids::MacroOscillator *osc = unit->pd.osc;

    
    // TODO: check setting pitch
    CONSTRAIN(voct_in, 0.f, 127.f);
    int pit = (int)voct_in;
    float frac = voct_in - pit;
    osc->set_pitch((pit << 7) + (int)(frac * 128.f));
    
    // set parameters
    CONSTRAIN(timbre_in, 0.f, 1.f);
    int16_t timbre = timbre_in * 32767.f;
    CONSTRAIN(color_in, 0.f, 1.f);
    int16_t color = color_in * 32767.f;
    osc->set_parameters(timbre, color);
    
    // set shape/model
    uint8_t shape = (int)(model_in);
    if (shape >= braids::MACRO_OSC_SHAPE_LAST)
        shape -= braids::MACRO_OSC_SHAPE_LAST;
    osc->set_shape(static_cast<braids::MacroOscillatorShape>(shape));
    
    // detect trigger
    if (INRATE(4) != calc_ScalarRate) {
        
        float sum = 0.f;
        
        if (INRATE(4) == calc_FullRate) {  // trigger input is audio rate
            // TODO: add vDSP vector summation
            for(int i=0; i<inNumSamples; ++i)
                sum += trig_in[i];
            
        }
        else {          // trigger input is control rate
            sum = trig_in[0];
        }
        
        bool trigger = (sum != 0.0);
        bool trigger_flag = (trigger && (!unit->last_trig));
        unit->last_trig = trigger;
        
        if(trigger_flag)
            osc->Strike();
        
    }
    
    
    braids::SignatureWaveshaper *ws = unit->ws;

    
    CONSTRAIN(decim, 1, BLOCK_SIZE);
    CONSTRAIN(reduc, 0, 6);
    CONSTRAIN(sig_in, 0.f, 1.f);
    
    size_t      decimation_factor = decim;
    uint16_t    bit_mask = bit_reduction_masks[reduc];
    uint16_t    signature = (int)(sig_in * 65535.f);
    int16_t     sample = 0;
    
    
    for(int count = 0; count < inNumSamples; count += size) {
        // render
        osc->Render(sync_buffer, buffer, BLOCK_SIZE);
        
        for (auto i = 0; i < size; ++i) {
            
            if((i % decimation_factor) == 0) {
                sample = buffer[i] & bit_mask;
            }
            int16_t warped = ws->Transform(sample);
            buffer[i] = stmlib::Mix(sample, warped, signature);
            
            
            out[count + i] = buffer[i] * SAMP_SCALE;
        }
    }
    
}




#pragma mark ---------- callback function -----------

static long
src_input_callback(void *cb_data, float **audio)
{
    PROCESS_CB_DATA *data = (PROCESS_CB_DATA *) cb_data;
    const int input_frames = BLOCK_SIZE;
    
    int16_t     *buffer = data->buffer;
    uint8_t     *sync_buffer = data->sync_buffer;
    float       *samps = data->samps;

    data->osc->Render(sync_buffer, buffer, input_frames);
    
    for (size_t i = 0; i < input_frames; ++i) {
        samps[i] = (buffer[i] * SAMP_SCALE);
    }
    
    *audio = &(samps [0]);
    
    return input_frames;
}


PluginLoad(MiBraids) {
    ft = inTable;
    DefineDtorUnit(MiBraids);
}




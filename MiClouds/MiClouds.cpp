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
 
 MiClouds - based on the 'Clouds' eurorack module by
 https://mutable-instruments.net/
 
 Original code by Émilie Gillet
 
 https://vboehm.net
 
 */



#include "SC_PlugIn.h"

#include "clouds/dsp/granular_processor.h"
#include "clouds/resources.h"
#include "clouds/dsp/audio_buffer.h"
#include "clouds/dsp/mu_law.h"
#include "clouds/dsp/sample_rate_converter.h"

static InterfaceTable *ft;


const uint16 kAudioBlockSize = 32;        // sig vs can't be smaller than this!
const uint16 kNumArgs = 14;


enum ModParams {
    PARAM_PITCH,
    PARAM_POSITION,
    PARAM_SIZE,
    PARAM_DENSITY,
    PARAM_TEXTURE,
    PARAM_DRYWET,
    PARAM_CHANNEL_LAST
};



struct MiClouds : public Unit {
    
    clouds::GranularProcessor   *processor;
    
    // buffers
    uint8_t     *large_buffer;
    uint8_t     *small_buffer;
    
    // parameters
    float       in_gain;
    bool        freeze;
    bool        trigger, previous_trig;
    bool        gate;
    
    float       pot_value_[PARAM_CHANNEL_LAST];
    float       smoothed_value_[PARAM_CHANNEL_LAST];
    float       coef;      // smoothing coefficient for parameter changes
    
    clouds::FloatFrame  input[kAudioBlockSize];
    clouds::FloatFrame  output[kAudioBlockSize];
    
    float       sr;
    long        sigvs;
    
    bool        gate_connected;
    bool        trig_connected;
    uint32      pcount;
    
    
    clouds::SampleRateConverter<-clouds::kDownsamplingFactor, 45, clouds::src_filter_1x_2_45> src_down_;
    clouds::SampleRateConverter<+clouds::kDownsamplingFactor, 45, clouds::src_filter_1x_2_45> src_up_;
    
};



static void MiClouds_Ctor(MiClouds *unit);
static void MiClouds_Dtor(MiClouds *unit);
static void MiClouds_next(MiClouds *unit, int inNumSamples);


static void MiClouds_Ctor(MiClouds *unit) {
    
    if(BUFLENGTH < kAudioBlockSize) {
        Print("MiClouds ERROR: Block Size can't be smaller than %d samples\n", kAudioBlockSize);
        unit = NULL;
        return;
    }
    
    int largeBufSize = 118784;
    int smallBufSize = 65536-128;
    
    // alloc mem
    unit->large_buffer = (uint8_t*)RTAlloc(unit->mWorld, largeBufSize * sizeof(uint8_t));
    unit->small_buffer = (uint8_t*)RTAlloc(unit->mWorld, smallBufSize * sizeof(uint8_t));

    
    if(unit->large_buffer == NULL || unit->small_buffer == NULL) {
        Print( "mem alloc failed!" );
        unit = NULL;
        return;
    }
    
    unit->sr = SAMPLERATE;
    
    unit->processor = new clouds::GranularProcessor;
    memset(unit->processor, 0, sizeof(*unit->processor));
    //Print("sizeof processor: %d\n", sizeof(*unit->processor));
    unit->processor->Init(unit->large_buffer, largeBufSize, unit->small_buffer, smallBufSize);
    unit->processor->set_sample_rate(unit->sr);
    unit->processor->set_num_channels(2);       // always use stereo setup
    unit->processor->set_low_fidelity(false);
    unit->processor->set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
    
    // init values
    unit->pot_value_[PARAM_PITCH] = unit->smoothed_value_[PARAM_PITCH] = 0.f;
    unit->pot_value_[PARAM_POSITION] = unit->smoothed_value_[PARAM_POSITION] = 0.f;
    unit->pot_value_[PARAM_SIZE] = unit->smoothed_value_[PARAM_SIZE] = 0.5f;
    unit->pot_value_[PARAM_DENSITY] = unit->smoothed_value_[PARAM_DENSITY] = 0.1f;
    unit->pot_value_[PARAM_TEXTURE] = unit->smoothed_value_[PARAM_TEXTURE] = 0.5f;
    
    unit->pot_value_[PARAM_DRYWET] = unit->smoothed_value_[PARAM_DRYWET] = 1.f;
    unit->processor->mutable_parameters()->stereo_spread = 0.5f;
    unit->processor->mutable_parameters()->reverb = 0.f;
    unit->processor->mutable_parameters()->feedback = 0.f;
    unit->processor->mutable_parameters()->freeze = false;
    
    unit->in_gain = 1.0f;
    unit->coef = 0.1f;
    unit->previous_trig = false;
    
    unit->src_down_.Init();
    unit->src_up_.Init();
    
    unit->pcount = 0;
    
    uint16 numAudioInputs = unit->mNumInputs - kNumArgs;
    Print("MiClouds > numAudioIns: %d\n", numAudioInputs);
    
    SETCALC(MiClouds_next);
    ClearUnitOutputs(unit, 1);
    //MiClouds_next(unit, 1);
    
}


static void MiClouds_Dtor(MiClouds *unit) {
    
    if(unit->large_buffer) {
        RTFree(unit->mWorld, unit->large_buffer);
    }
    if(unit->small_buffer) {
        RTFree(unit->mWorld, unit->small_buffer);
    }
    if(unit->processor) delete unit->processor;
}



#pragma mark ----- dsp loop -----

void MiClouds_next( MiClouds *unit, int inNumSamples )
{
    float   pitch = IN0(0);
/*
    float   pos_in = IN0(1);
    float   size_in = IN0(2);
    float   dens_in = IN0(3);
    float   tex_in = IN0(4);
    float   dry_wet_in = IN0(5);
 */
    float   in_gain = IN0(6);
    float   spread = IN0(7);
    float   reverb = IN0(8);
    float   fb = IN0(9);
    bool    freeze = IN0(10) > 0.f;
    short   mode = IN0(11);
    bool    lofi = IN0(12) > 0.f;
    float   *trig_in = IN(13);

    
    float   *outL = OUT(0);
    float   *outR = OUT(1);
    
    int vs = inNumSamples;

    // find out number of audio inputs
    uint16 numAudioInputs = unit->mNumInputs - kNumArgs;
    
    
    if(numAudioInputs == 1)
        Copy(inNumSamples, IN(kNumArgs+1), IN(kNumArgs));
    
    clouds::FloatFrame  *input = unit->input;
    clouds::FloatFrame  *output = unit->output;
    
    float       *smoothed_value = unit->smoothed_value_;
    float       coef = unit->coef;
    
    
    clouds::GranularProcessor   *gp = unit->processor;
    clouds::Parameters   *p = gp->mutable_parameters();

    
    
    CONSTRAIN(pitch, -48.0f, 48.0f);
    smoothed_value[PARAM_PITCH] += coef * (pitch - smoothed_value[PARAM_PITCH]);
    p->pitch = smoothed_value[PARAM_PITCH];
    
    
    for(auto i=1; i<PARAM_CHANNEL_LAST; ++i) {
        float value = IN0(i);
        CONSTRAIN(value, 0.0f, 1.0f);
        smoothed_value[i] += coef * (value - smoothed_value[i]);
//        smoothed_value[i] = value;
    }
    
    p->position = smoothed_value[PARAM_POSITION];
    p->size = smoothed_value[PARAM_SIZE];
    p->density = smoothed_value[PARAM_DENSITY];
    p->texture = smoothed_value[PARAM_TEXTURE];
    p->dry_wet = smoothed_value[PARAM_DRYWET];
    
    
    /*
    CONSTRAIN(pos_in, 0.f, 1.f);
    CONSTRAIN(size_in, 0.f, 1.f);
    CONSTRAIN(dens_in, 0.f, 1.f);
    CONSTRAIN(tex_in, 0.f, 1.f);
    CONSTRAIN(dry_wet_in, 0.f, 1.f);
    
    p->position = pos_in;
    p->size = size_in;
    p->density = dens_in;
    p->texture = tex_in;
    p->dry_wet = dry_wet_in;
    */
    
    CONSTRAIN(in_gain, 0.125f, 8.f);
    CONSTRAIN(spread, 0.f, 1.f);
    CONSTRAIN(reverb, 0.f, 1.f);
    CONSTRAIN(fb, 0.f, 1.f);
    CONSTRAIN(mode, 0, 3);
    
    p->stereo_spread = spread;
    p->reverb = reverb;
    p->feedback = fb;
    gp->set_low_fidelity(lofi);
    gp->set_freeze(freeze);
    gp->set_playback_mode(static_cast<clouds::PlaybackMode>(mode));
    
    
    
    uint16 trig_rate = INRATE(13);
    
    for(int count = 0; count < vs; count += kAudioBlockSize) {
        
        for(auto i=0; i<kAudioBlockSize; ++i) {
            input[i].l = IN(kNumArgs)[i + count] * in_gain;
            input[i].r = IN(kNumArgs+1)[i + count] * in_gain;
        }
        
        bool trigger = false;
        switch(trig_rate) {
            case 1 :
                trigger = ( trig_in[0] > 0.f );
                break;
            case 2 :
                float sum = 0.f;
                for(auto i=0; i<kAudioBlockSize; ++i) {
                    sum += ( trig_in[i+count] );
                }
                trigger = ( sum > 0.f );
                break;
        }
        
        p->trigger = (trigger && !unit->previous_trig);
        unit->previous_trig = trigger;
        
        
        gp->Process(input, output, kAudioBlockSize);
        gp->Prepare();      // muss immer hier sein?
        
        if(p->trigger)
            p->trigger = false;
        
        for(auto i=0; i<kAudioBlockSize; ++i) {
            outL[i + count] = output[i].l;
            outR[i + count] = output[i].r;
        }
    }
}




PluginLoad(MiClouds) {
    ft = inTable;
    DefineDtorUnit(MiClouds);
}

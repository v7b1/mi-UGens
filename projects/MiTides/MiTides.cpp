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
 
 MiTides - based on the 'Tides(2)' eurorack module by
 https://mutable-instruments.net/
 
 Original code by Émilie Gillet
 
 https://vboehm.net
 
 */



#include "SC_PlugIn.h"

#include "stmlib/dsp/dsp.h"

#include "tides2/poly_slope_generator.h"
#include "tides2/ramp_extractor.h"
//#include "tides2/resources.h"

static InterfaceTable *ft;


const size_t kAudioBlockSize = 8;        // sig vs can't be smaller than this!
const size_t kNumOutputs = 4;

static tides::Ratio kRatios[19] = {
    { 0.0625f, 16 },
    { 0.125f, 8 },
    { 0.1666666f, 6 },
    { 0.25f, 4 },
    { 0.3333333f, 3 },
    { 0.5f, 2 },
    { 0.6666666f, 3 },
    { 0.75f, 4 },
    { 0.8f, 5 },
    { 1, 1 },
    { 1.25f, 4 },
    { 1.3333333f, 3 },
    { 1.5f, 2 },
    { 2.0f, 1 },
    { 3.0f, 1 },
    { 4.0f, 1 },
    { 6.0f, 1 },
    { 8.0f, 1 },
    { 16.0f, 1 },
};



struct MiTides : public Unit {
    
    tides::PolySlopeGenerator poly_slope_generator;
    tides::RampExtractor ramp_extractor;
    
    tides::PolySlopeGenerator::OutputSample out[kAudioBlockSize];
    stmlib::GateFlags   no_gate[kAudioBlockSize];
    stmlib::GateFlags   gate_input[kAudioBlockSize];
    stmlib::GateFlags   clock_input[kAudioBlockSize];
    stmlib::GateFlags   previous_flags_[2 + 1];       // two inlets, TODO: why + 1
    tides::Ratio        r_;
    
    float   ramp[kAudioBlockSize];
    
    tides::OutputMode   output_mode;
    tides::OutputMode   previous_output_mode;
    tides::RampMode     ramp_mode;
    tides::Range        range;
    
    float       frequency, freq_lp;
    float       shape, shape_lp;
    float       slope, slope_lp;
    float       smoothness, smooth_lp;
    float       shift, shift_lp;
    
    bool        must_reset_ramp_extractor;
    
    float       sr;
    float       r_sr;


};



static void MiTides_Ctor(MiTides *unit);
static void MiTides_Dtor(MiTides *unit);
static void MiTides_next(MiTides *unit, int inNumSamples);


static void MiTides_Ctor(MiTides *unit) {
    
    if(BUFLENGTH < kAudioBlockSize) {
        Print("MiTides ERROR: Block Size can't be smaller than %d samples\n", kAudioBlockSize);
        unit = NULL;
        return;
    }
    
    
    unit->sr = SAMPLERATE;
    unit->r_sr =  1.f / unit->sr;
    
    unit->poly_slope_generator.Init();
    unit->ramp_extractor.Init(unit->sr, 40.0f * unit->r_sr);
    
    std::fill(&unit->no_gate[0], &unit->no_gate[kAudioBlockSize], stmlib::GATE_FLAG_LOW);
    std::fill(&unit->clock_input[0], &unit->clock_input[kAudioBlockSize], stmlib::GATE_FLAG_LOW);
    std::fill(&unit->ramp[0], &unit->ramp[kAudioBlockSize], 0.f);
    
    unit->output_mode = tides::OUTPUT_MODE_FREQUENCY;
    unit->previous_output_mode = tides::OUTPUT_MODE_FREQUENCY;
    unit->ramp_mode = tides::RAMP_MODE_LOOPING;
    unit->range = tides::RANGE_AUDIO;
    
    unit->previous_flags_[0] = stmlib::GATE_FLAG_LOW;
    unit->previous_flags_[1] = stmlib::GATE_FLAG_LOW;
    
    unit->must_reset_ramp_extractor = false;
    
    unit->frequency = 1.f;
    unit->shape = 0.5f;
    unit->slope = 0.5f;
    unit->smoothness = 0.5f;
    unit->shift = 0.3f;
    
    
    unit->freq_lp = unit->shape_lp = unit->slope_lp = unit->smooth_lp = unit->shift_lp = 0.f;
    
    unit->r_.ratio = 1.0f;
    unit->r_.q = 1;
    
    
    SETCALC(MiTides_next);
    ClearUnitOutputs(unit, 1);
    //MiTides_next(unit, 1);
    
}


static void MiTides_Dtor(MiTides *unit) {
    
    // ...
}



#pragma mark ----- dsp loop -----

void MiTides_next( MiTides *unit, int inNumSamples )
{
    // TODO: make these audio rate inputs
    float   freq_in = IN0(0);

    float   shape_in = IN0(1);
    float   slope_in = IN0(2);
    float   smooth_in = IN0(3);
    float   shift_in = IN0(4);
    

    float   *trig_in = IN(5);
    float   *clock_in = IN(6);

    int     outp_mode = IN0(7)+0.5f;
    int     rmp_mode = IN0(8)+0.5f;
    int     ratio = IN0(9)+0.5f;
    int     rate = IN0(10);     // choose from CONTROL or AUDIO rate/range
    
    
    int vs = inNumSamples;

    tides::RampExtractor *ramp_extractor = &unit->ramp_extractor;
    tides::PolySlopeGenerator::OutputSample *out = unit->out;
    float   *ramp = unit->ramp;
    
    stmlib::GateFlags *clock_input = unit->clock_input;
    stmlib::GateFlags *gate_flags = unit->no_gate;
    stmlib::GateFlags *previous_flags = unit->previous_flags_;
    
    bool    must_reset_ramp_extractor = unit->must_reset_ramp_extractor;
    bool    use_clock = false;
    bool    use_trigger = false;
    
    CONSTRAIN(outp_mode, 0, 3);
    CONSTRAIN(rmp_mode, 0, 2);
    CONSTRAIN(ratio, 0, 18);
    tides::OutputMode   output_mode = (tides::OutputMode)outp_mode;
    tides::RampMode     ramp_mode = (tides::RampMode)rmp_mode;
    tides::Range        range = (tides::Range)rate; //unit->range;
    tides::Ratio        r_ = kRatios[ratio];
    
    float   frequency, shape, slope, shift, smoothness;
    
//    float   freq_lp = unit->freq_lp;
    float   shape_lp = unit->shape_lp;
    float   slope_lp = unit->slope_lp;
    float   shift_lp = unit->shift_lp;
    float   smooth_lp = unit->smooth_lp;
    
    float   r_sr = unit->r_sr;

    if( INRATE(5) == calc_FullRate )
        use_trigger = true;
    
    if( INRATE(6) == calc_FullRate )
        use_clock = true;
    
    
    for(int count = 0; count < vs; count += kAudioBlockSize) {

        // check for gate/trigger input
        if(use_trigger) {

            gate_flags = unit->gate_input;
            for(int i=0; i<kAudioBlockSize; ++i) {
                bool trig = trig_in[i + count] > 0.01;
                previous_flags[0] = stmlib::ExtractGateFlags(previous_flags[0], trig);
                gate_flags[i] = previous_flags[0];
            }
        }

        if (use_clock) {

            if (must_reset_ramp_extractor) {
                ramp_extractor->Reset();
            }

            for(int i=0; i<kAudioBlockSize; ++i) {
                bool trig = clock_in[i + count] > 0.01;
                previous_flags[1] = stmlib::ExtractGateFlags(previous_flags[1], trig);
                clock_input[i] = previous_flags[1];
            }

            frequency = ramp_extractor->Process(range,
                                                range == tides::RANGE_AUDIO && ramp_mode == tides::RAMP_MODE_AR,
                                                r_,
                                                clock_input,
                                                ramp,
                                                kAudioBlockSize);

            must_reset_ramp_extractor = false;

        }
        else {
            frequency = (freq_in) * r_sr;
            CONSTRAIN(frequency, 0.f, 0.4f);
            // no filtering for now
            //            ONE_POLE(freq_lp, frequency, 0.3f);
            //            frequency = freq_lp;
            must_reset_ramp_extractor = true;
        }


        // parameter inputs
        shape = shape_in;
        CONSTRAIN(shape, 0.f, 1.f);
        ONE_POLE(shape_lp, shape, 0.1f);
        slope = slope_in;
        CONSTRAIN(slope, 0.f, 1.f);
        ONE_POLE(slope_lp, slope, 0.1f);
        smoothness = smooth_in;
        CONSTRAIN(smoothness, 0.f, 1.f);
        ONE_POLE(smooth_lp, smoothness, 0.1f);
        shift = shift_in;
        CONSTRAIN(shift, 0.f, 1.f);
        ONE_POLE(shift_lp, shift, 0.1f);


        unit->poly_slope_generator.Render(ramp_mode,
                                          output_mode,
                                          range,
                                          frequency, slope_lp, shape_lp, smooth_lp, shift_lp,
                                          gate_flags,
                                          !use_trigger && use_clock ? ramp : NULL,
                                          out, kAudioBlockSize);
        

        for(int i=0; i<kAudioBlockSize; ++i) {
            for(int j=0; j<kNumOutputs; ++j) {
                OUT(j)[i + count] = out[i].channel[j] * 0.1f;
            }
        }
        
    }
    
//    unit->freq_lp = freq_lp;
    unit->shape_lp = shape_lp;
    unit->shift_lp = shift_lp;
    unit->slope_lp = slope_lp;
    unit->smooth_lp = smooth_lp;
    
    unit->must_reset_ramp_extractor = must_reset_ramp_extractor;

}




PluginLoad(MiTides) {
    ft = inTable;
    DefineDtorUnit(MiTides);
}

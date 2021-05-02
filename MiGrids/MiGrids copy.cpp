/*
 *  MiGrids.cpp
 *  Plugins
 *
 *  Created by vboehm on 29.04.21
 *
 *
 */

// Copyright 2012 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//


// BPM VALUES ONLY WORK CORRECTLY AT SR 48 kHz !!!!!!!!

#include "SC_PlugIn.h"

#include "avrlib/op.h"
#include "grids/clock.h"
#include "grids/pattern_generator.h"

#include <cstdio>

#define COUNTMAX 6

// the original module has a control rate of 8000 Hz
// if we run at 48 kHz, take ever 6th sample as a clock tick


static InterfaceTable *ft;

using namespace grids;

struct MiGrids : public Unit {
    
    int8_t      swing_amount;       // untested
    uint32_t    tap_duration;
    uint8_t     count;
    float       sr;
    
};

uint8_t ticks_granularity[] = { 6, 3, 1 };

#define CONSTRAIN(var, min, max) \
if (var < (min)) { \
var = (min); \
} else if (var > (max)) { \
var = (max); \
}



static void MiGrids_Ctor(MiGrids *unit);
static void MiGrids_next(MiGrids *unit, int inNumSamples);


static void MiGrids_Ctor(MiGrids *unit) {
    
    
    grids::clock.Init();
    pattern_generator.Init();
    
    unit->swing_amount = 0;
    unit->count = 0;
    unit->sr = SAMPLERATE;
    

    
    SETCALC(MiGrids_next);        // tells sc synth the name of the calculation function
    MiGrids_next(unit, 1);
}


#pragma mark ----- dsp loop -----

void MiGrids_next( MiGrids *unit, int inNumSamples )
{
    // TODO: change first input to receive external clock?
    // and add a dedicated bpm input?
    uint16_t    bpm = IN0(0);
    // use overflow to limit data range to uint8
    uint8_t     map_x = IN0(1) * 255.0f;
    uint8_t     map_y = IN0(2) * 255.0f;
    uint8_t     randomness = IN0(3) * 255.0f;
    uint8_t     bd_density = IN0(4) * 255.0f;
    uint8_t     sd_density = IN0(5) * 255.0f;
    uint8_t     hh_density = IN0(6) * 255.0f;
    uint8_t     mode    = ( IN0(7) != 0.f );
    uint8_t     swing  = ( IN0(8) != 0.f );
    uint8_t     config  = ( IN0(9) != 0.f );
    
    float *out1 = OUT(0);
    float *out2 = OUT(1);
    float *out3 = OUT(2);
    float *out4 = OUT(3);
    float *out5 = OUT(4);
    float *out6 = OUT(5);

    
//    bpm = avrlib::U8U8MulShift8(bpm, 220) + 20;
//    if(bpm < 20) bpm = 20;
//    if(bpm >= 511) bpm = 511;
    CONSTRAIN(bpm, 20, 511);
    
    if (bpm != grids::clock.bpm() && !grids::clock.locked()) {
        grids::clock.Update(bpm, pattern_generator.clock_resolution());
        std::printf("new bpm: %d\n", grids::clock.bpm());
    }
    PatternGeneratorSettings* settings = pattern_generator.mutable_settings();
    settings->options.drums.x = map_x;
    settings->options.drums.y = map_y;
    settings->options.drums.randomness = randomness;
    settings->density[0] = bd_density;
    settings->density[1] = sd_density;
    settings->density[2] = hh_density;
    
    pattern_generator.set_output_mode(mode);
    pattern_generator.set_swing(swing);
    pattern_generator.set_output_clock(config);

//    if(INRATE(1) == calc_FullRate) {
//        for (size_t i = 0; i < inNumSamples; ++i) {
//            ;
//        }
//    }
    
    uint8_t count = unit->count;
    
    for (size_t i = 0; i < inNumSamples; ++i) {
        
        // the original module has a SR of 8000 Hz
        
        if(count >= COUNTMAX) {
            count = 0;
            static uint8_t previous_inputs;
    //        uint8_t inputs_value = ~inputs.Read();
            uint8_t num_ticks = 0;
            uint8_t increment = ticks_granularity[pattern_generator.clock_resolution()];
            grids::clock.Tick();
            grids::clock.Wrap(unit->swing_amount);  // TODO: initialize swing_amount
            if (grids::clock.raising_edge()) {
                num_ticks = increment;
            }
            if (grids::clock.past_falling_edge()) {
                pattern_generator.ClockFallingEdge();
            }
            
            if (num_ticks) {
                unit->swing_amount = pattern_generator.swing_amount();
                pattern_generator.TickClock(num_ticks);
            }
            
            pattern_generator.IncrementPulseCounter();
        
        }
        count++;
        
        uint8_t state = pattern_generator.state();
        
        // decode 'state' into output triggers/gates
        out1[i] = (state & 1);
        out2[i] = (state & 2) >> 1;
        out3[i] = (state & 4) >> 2;
        out4[i] = (state & 8) >> 3;
        out5[i] = (state & 16) >> 4;
        out6[i] = (state & 32) >> 5;
        
        
    }
    
    unit->count = count;
    
}


PluginLoad(MiGrids) {
    ft = inTable;
    DefineSimpleUnit(MiGrids);
}

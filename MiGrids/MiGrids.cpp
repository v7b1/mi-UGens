/*
 mi-UGens - SuperCollider UGen Library
 Copyright (c) 2021 Volker Böhm. All rights reserved.
 
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
 
 MiGrids - a clone of the eurorack module 'Grids' by
 https://mutable-instruments.net/
 
 Original code by Émilie Gillet
 
 https://vboehm.net
 
 */


#include "SC_PlugIn.h"

#include "avrlib/op.h"
#include "grids/clock.h"
#include "grids/pattern_generator.h"

#include <cstdio>

#define COUNTMAX 4

// the original module runs at a control rate of 8000 Hz

static InterfaceTable *ft;

using namespace grids;

struct MiGrids : public Unit {
    
    grids::Clock clock;
    grids::PatternGenerator pattern_generator;
    int8_t      swing_amount;
    uint32_t    tap_duration;
    uint8_t     count;
    float       sr;
    float       c;     // factor for bpm calculation
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
    
    unit->sr = SAMPLERATE;
    unit->c = ((1L<<32) * 8) / (120 * unit->sr / COUNTMAX);
    
    unit->clock.Init(unit->c);
    unit->pattern_generator.Init();
    
    uint8_t reso = IN0(10);
    CONSTRAIN(reso, 0, 2);
    unit->pattern_generator.set_clock_resolution(reso);
//    unit->clock.Update(120, unit->pattern_generator.clock_resolution());
    unit->pattern_generator.Reset();
    
    unit->swing_amount = 0;
    unit->count = 0;
    
    SETCALC(MiGrids_next);        // tells sc synth the name of the calculation function
    MiGrids_next(unit, 1);
}


#pragma mark ----- dsp loop -----

void MiGrids_next( MiGrids *unit, int inNumSamples )
{
    // TODO: change first input to receive external clock?
    // and add a dedicated bpm input?
    float       bpm = IN0(0);
    // use overflow to limit data range to uint8
    uint8_t     map_x = IN0(1) * 255.0f;
    uint8_t     map_y = IN0(2) * 255.0f;
    uint8_t     randomness = IN0(3) * 255.0f;
    uint8_t     bd_density = IN0(4) * 255.0f;
    uint8_t     sd_density = IN0(5) * 255.0f;
    uint8_t     hh_density = IN0(6) * 255.0f;
    uint8_t     mode    = ( IN0(7) == 0.f );
    uint8_t     swing  = ( IN0(8) != 0.f );
    uint8_t     config  = ( IN0(9) != 0.f );
    
//    float *out1 = OUT(0);
//    float *out2 = OUT(1);
//    float *out3 = OUT(2);
//    float *out4 = OUT(3);
//    float *out5 = OUT(4);
//    float *out6 = OUT(5);
    
    grids::Clock *clock = &unit->clock;
    grids::PatternGenerator *pattern_generator = &unit->pattern_generator;
    
    CONSTRAIN(bpm, 20.0f, 511.0f);
    
    if (bpm != clock->bpm() && !clock->locked()) {
        clock->Update_f(bpm, unit->c);
//        std::printf("new bpm: %d\n", clock->bpm());
    }
    PatternGeneratorSettings* settings = pattern_generator->mutable_settings();
    settings->options.drums.x = map_x;
    settings->options.drums.y = map_y;
    settings->options.drums.randomness = randomness;
    settings->density[0] = bd_density;
    settings->density[1] = sd_density;
    settings->density[2] = hh_density;
    
    pattern_generator->set_output_mode(mode);
    pattern_generator->set_swing(swing);
    pattern_generator->set_output_clock(config);

//    if(INRATE(1) == calc_FullRate) {
//        for (size_t i = 0; i < inNumSamples; ++i) {
//            ;
//        }
//    }
    
    uint8_t count = unit->count;
    
    for (size_t i = 0; i < inNumSamples; ++i) {
        
        if(count >= COUNTMAX) {
            count = 0;
//          static uint8_t previous_inputs;
//          uint8_t inputs_value = ~inputs.Read();
            uint8_t num_ticks = 0;
            uint8_t increment = ticks_granularity[pattern_generator->clock_resolution()];
            clock->Tick();
            clock->Wrap(unit->swing_amount);
            if (clock->raising_edge()) {
                num_ticks = increment;
            }
            if (clock->past_falling_edge()) {
                pattern_generator->ClockFallingEdge();
            }
            
            if (num_ticks) {
                unit->swing_amount = pattern_generator->swing_amount();
                pattern_generator->TickClock(num_ticks);
            }
            
            pattern_generator->IncrementPulseCounter();
        
        }
        count++;
        
        uint8_t state = pattern_generator->state();
        
        // decode 'state' into output triggers/gates
        
        // OUTPUT MODE  OUTPUT CLOCK  BIT7  BIT6  BIT5  BIT4  BIT3  BIT2  BIT1  BIT0
        // DRUMS        FALSE          RND   CLK  HHAC  SDAC  BDAC    HH    SD    BD
        // DRUMS        TRUE           RND   CLK   CLK   BAR   ACC    HH    SD    BD
        // EUCLIDEAN    FALSE          RND   CLK  RST3  RST2  RST1  EUC3  EUC2  EUC1
        // EUCLIDEAN    TRUE           RND   CLK   CLK  STEP   RST  EUC3  EUC2  EUC1
        
//        out1[i] = (state & 1);
//        out2[i] = (state & 2) >> 1;
//        out3[i] = (state & 4) >> 2;
//        out4[i] = (state & 8) >> 3;
//        out5[i] = (state & 16) >> 4;
//        out6[i] = (state & 32) >> 5;
        
        for(int k=0; k<6; k++) {
            OUT(k)[i] = (state >> k) & 1;
        }
        
        
    }
    
    unit->count = count;
    
}


PluginLoad(MiGrids) {
    ft = inTable;
    DefineSimpleUnit(MiGrids);
}

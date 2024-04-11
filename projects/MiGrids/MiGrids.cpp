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
    float       sr;
    float       c;     // factor for bpm calculation
    bool        previous_clock, previous_reset;
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
    
    uint8_t reso = IN0(14);
    CONSTRAIN(reso, 0, 2);
    unit->pattern_generator.set_clock_resolution(reso);
    unit->pattern_generator.Reset();
    
    unit->swing_amount = 0;
    unit->previous_clock = unit->previous_reset = false;
    
    SETCALC(MiGrids_next);        // tells sc synth the name of the calculation function
    MiGrids_next(unit, 1);
}


#pragma mark ----- dsp loop -----

void MiGrids_next( MiGrids *unit, int inNumSamples )
{
    // TODO: change first input to receive external clock?
    // and add a dedicated bpm input?
    bool        on_off = ( IN0(0) != 0.f );
    float       bpm = IN0(1);
    // use overflow to limit data range to uint8
    uint8_t     map_x = IN0(2) * 255.0f;
    uint8_t     map_y = IN0(3) * 255.0f;
    uint8_t     randomness = IN0(4) * 255.0f;
    uint8_t     bd_density = IN0(5) * 255.0f;
    uint8_t     sd_density = IN0(6) * 255.0f;
    uint8_t     hh_density = IN0(7) * 255.0f;
    float       *clock_trig  = IN(8);
    float       *reset_trig  = IN(9);
    bool        use_ext_clock  = ( IN0(10) != 0.f );
    uint8_t     mode    = ( IN0(11) == 0.f );
    uint8_t     swing  = ( IN0(12) != 0.f );
    uint8_t     config  = ( IN0(13) != 0.f );

    
    if (!on_off) {
        for(int k=0; k<8; ++k) {
            memset(OUT(k), 0, inNumSamples * sizeof(float));
        }
        return;
    }
    
    grids::Clock *clock = &unit->clock;
    grids::PatternGenerator *pattern_generator = &unit->pattern_generator;
    
    CONSTRAIN(bpm, 20.0f, 511.0f);
    
    if (bpm != clock->bpm() && !clock->locked()) {
        clock->Update_f(bpm, unit->c, unit->pattern_generator.clock_resolution());
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
    
    uint8_t increment = ticks_granularity[pattern_generator->clock_resolution()];
    bool    previous_clock = unit->previous_clock;
    bool    previous_reset = unit->previous_reset;
    
    
    for (size_t i = 0; i < inNumSamples; i+=COUNTMAX) {
        
        uint8_t num_ticks = 0;
        
        // external clock input
        if(use_ext_clock) {
            
            float clock_sum = 0.f;
            if (INRATE(8) == calc_FullRate) {
                for(int k=i; k<(i+COUNTMAX); ++k)
                    clock_sum += clock_trig[k];
                
                bool clock_in = clock_sum > 0.1f;
                if (clock_in && !previous_clock) {
                    num_ticks = increment;
                }
                else if (!clock_in && previous_clock) {
                    pattern_generator->ClockFallingEdge();
                    clock_sum = 0.f;
                }
                previous_clock = clock_in;
            }
            
        }
        else {
            clock->Tick();
            clock->Wrap(unit->swing_amount);
            if (clock->raising_edge()) {
                num_ticks = increment;
            }
            if (clock->past_falling_edge()) {
                pattern_generator->ClockFallingEdge();
            }
        }
        
        // handle reset input
        if (INRATE(9) == calc_FullRate) {
            float reset_sum = 0.f;
            for(int k=i; k<(i+COUNTMAX); ++k)
                reset_sum += reset_trig[k];
            bool reset_in = (reset_sum > 0.1f);
            if (reset_in && !previous_reset)
                pattern_generator->Reset();

            previous_reset = reset_in;
        }
        
        if (num_ticks) {
            unit->swing_amount = pattern_generator->swing_amount();
            pattern_generator->TickClock(num_ticks);
        }
        
        pattern_generator->IncrementPulseCounter();
        
        
        uint8_t state = pattern_generator->state();
        
        // decode 'state' into output triggers/gates
        
        for(int k=0; k<8; k++) {
//            OUT(k)[i] = (state >> k) & 1;
            float val = (state >> k) & 1;
            std::fill_n(OUT(k)+i, COUNTMAX, val);
        }
        
        
    }
    
    unit->previous_clock = previous_clock;
    unit->previous_reset = previous_reset;
    
}


PluginLoad(MiGrids) {
    ft = inTable;
    DefineSimpleUnit(MiGrids);
}

//
// Copyright 2020 Volker Böhm.
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


// a clone of mutable instruments' 'Tides(2)' module for maxmsp
// by volker böhm, july 2020, https://vboehm.net


// Original code by Émilie Gillet, https://mutable-instruments.net/



#include "c74_msp.h"

#include "stmlib/dsp/dsp.h"
#include "stmlib/dsp/hysteresis_quantizer.h"
#include "stmlib/dsp/units.h"

#include "tides2/poly_slope_generator.h"
#include "tides2/ramp_extractor.h"
#include "tides2/resources.h"




const size_t kAudioBlockSize = 8;       // sig vs can't be smaller than this!
const size_t kNumOutputs = 4;

using namespace c74::max;


static t_class* this_class = nullptr;


struct t_myObj {
	t_pxobject	obj;
    
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
    bool        trig_connected;
    bool        clock_connected;
    bool        use_trigger;
    bool        use_clock;
    
    float       sr;
    float       r_sr;
    long        sigvs;
    
};



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


void* myObj_new(t_symbol *s, long argc, t_atom *argv)
{
	t_myObj* self = (t_myObj*)object_alloc(this_class);
	
    if(self)
    {
        dsp_setup((t_pxobject*)self, 7);
        outlet_new(self, "signal");
        outlet_new(self, "signal");
        outlet_new(self, "signal");
        outlet_new(self, "signal");
        
        self->sigvs = sys_getblksize();
        
        if(self->sigvs < kAudioBlockSize) {
            object_error((t_object*)self,
                         "sigvs can't be smaller than %d samples\n", kAudioBlockSize);
            object_free(self);
            self = NULL;
            return self;
        }

        self->sr = sys_getsr();
        self->r_sr =  1.f / self->sr;
        

        self->poly_slope_generator.Init();
        self->ramp_extractor.Init(self->sr, 40.0f * self->r_sr);
        
        std::fill(&self->no_gate[0], &self->no_gate[kAudioBlockSize], stmlib::GATE_FLAG_LOW);
        std::fill(&self->clock_input[0], &self->clock_input[kAudioBlockSize], stmlib::GATE_FLAG_LOW);
        std::fill(&self->ramp[0], &self->ramp[kAudioBlockSize], 0.f);
        
        self->output_mode = tides::OUTPUT_MODE_GATES;
        self->previous_output_mode = tides::OUTPUT_MODE_GATES;
        self->ramp_mode = tides::RAMP_MODE_LOOPING;
        self->range = tides::RANGE_CONTROL;
        
        self->previous_flags_[0] = stmlib::GATE_FLAG_LOW;
        self->previous_flags_[1] = stmlib::GATE_FLAG_LOW;
        
        self->use_trigger = false;
        self->use_clock = false;
        self->must_reset_ramp_extractor = false;
        
        self->frequency = 1.f;
        self->shape = 0.5f;
        self->slope = 0.5f;
        self->smoothness = 0.5f;
        self->shift = 0.3f;
        
        
        self->freq_lp = self->shape_lp = self->slope_lp = self->smooth_lp = self->shift_lp = 0.f;
        
        self->r_.ratio = 1.0f;
        self->r_.q = 1;
        
        // process attributes
        attr_args_process(self, argc, argv);
        
    }
    else {
        object_free(self);
        self = NULL;
    }
    
	return self;
}



void myObj_int(t_myObj* self, long m) {
    
    long innum = proxy_getinlet((t_object *)self);
    
    switch (innum) {
        case 5:
            self->use_trigger = (m != 0);
            break;
        case 6:
            self->use_clock = (m != 0);
            break;
    }
    
}

void myObj_float(t_myObj* self, double m) {
    
    long innum = proxy_getinlet((t_object *)self);
    
    switch (innum) {
        case 0:
            self->frequency = m;
            break;
        case 1:
            self->shape = m;
            break;
        case 2:
            self->slope = m;
            break;
        case 3:
            self->smoothness = m;
            break;
        case 4:
            self->shift = m;
            break;
    }
    
}


#pragma mark -------- general pots -----------

void myObj_freq(t_myObj* self, double m) {
    self->frequency = m;
}

void myObj_shape(t_myObj* self, double m) {
    self->shape = m;
}

void myObj_slope(t_myObj* self, double m) {
    self->slope = m;
}

void myObj_smooth(t_myObj* self, double m) {
    self->smoothness = m;
}

void myObj_shift(t_myObj* self, double m) {
    self->shift = m;
}


void myObj_ratio(t_myObj* self, long m)
{
    CONSTRAIN(m, 0, 18);
    self->r_ = kRatios[m];
    object_post((t_object*)self, "ratio[%d]: ratio: %f -- q: %d", m, self->r_.ratio, self->r_.q);
}



t_max_err output_mode_setter(t_myObj *self, void *attr, long ac, t_atom *av)
{
    if (ac && av) {
        t_atom_long m = atom_getlong(av);
        self->output_mode = tides::OutputMode(m);
        if(self->output_mode != self->previous_output_mode) {
            self->poly_slope_generator.Reset();
            self->previous_output_mode = self->output_mode;
        }
    }
    
    return MAX_ERR_NONE;
}



t_max_err ramp_mode_setter(t_myObj *self, void *attr, long ac, t_atom *av)
{
    if (ac && av) {
        t_atom_long m = atom_getlong(av);
        self->ramp_mode = tides::RampMode(m);
    }
    
    return MAX_ERR_NONE;
}


t_max_err range_setter(t_myObj *self, void *attr, long ac, t_atom *av)
{
    if (ac && av) {
        t_atom_long m = atom_getlong(av);
        if( m != 0 ) self->range = tides::RANGE_AUDIO;
        else self->range = tides::RANGE_CONTROL;
        self->must_reset_ramp_extractor = true;  // TODO: do we need this?
    }
    
    return MAX_ERR_NONE;
}


#pragma mark -------- DSP Loop ----------

void myObj_perform64(t_myObj* self, t_object* dsp64, double** ins, long numins, double** outs, long numouts, long sampleframes, long flags, void* userparam)
{
    double  *freq_in = ins[0];
    double  *shape_in = ins[1];
    double  *slope_in = ins[2];
    double  *smooth_in = ins[3];
    double  *shift_in = ins[4];
    
    double  *trig_in = ins[5];      // trigger signal input
    double  *clock_in = ins[6];     // clock signal input
    
    
    long vs = sampleframes;
    
    if (self->obj.z_disabled)
        return;
    
    tides::RampExtractor *ramp_extractor = &self->ramp_extractor;
    tides::PolySlopeGenerator::OutputSample *out = self->out;
    float   *ramp = self->ramp;
    
    stmlib::GateFlags *clock_input = self->clock_input;
    stmlib::GateFlags *gate_flags = self->no_gate;
    stmlib::GateFlags *previous_flags = self->previous_flags_;
    
    bool    must_reset_ramp_extractor = self->must_reset_ramp_extractor;
    bool    use_clock = self->use_clock;
    bool    use_trigger = self->use_trigger;
    bool    clock_connected = self->clock_connected;
    bool    trig_connected = self->trig_connected;
    
    tides::OutputMode   output_mode = self->output_mode;
    tides::RampMode     ramp_mode = self->ramp_mode;
    tides::Range        range = self->range;
    
    float   frequency, shape, slope, shift, smoothness;
    float   freq_knob = self->frequency;
    float   shape_knob = self->shape;
    float   slope_knob = self->slope;
    float   shift_knob = self->shift;
    float   smoothness_knob = self->smoothness;
    
    float   freq_lp = self->freq_lp;
    float   shape_lp = self->shape_lp;
    float   slope_lp = self->slope_lp;
    float   shift_lp = self->shift_lp;
    float   smooth_lp = self->smooth_lp;
    
    float   r_sr = self->r_sr;
    
    
    for(int count = 0; count < vs; count += kAudioBlockSize) {
        
        // check for gate/trigger input
        if(use_trigger && trig_connected) {
            
            gate_flags = self->gate_input;
            for(int i=0; i<kAudioBlockSize; ++i) {
                bool trig = trig_in[i + count] > 0.01;
                previous_flags[0] = stmlib::ExtractGateFlags(previous_flags[0], trig);
                gate_flags[i] = previous_flags[0];
            }
        }
        
        if (use_clock && clock_connected) {
            
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
                                                self->r_,
                                                clock_input,
                                                ramp,
                                                kAudioBlockSize);
            
            must_reset_ramp_extractor = false;
            
        }
        else {
            frequency = (freq_in[count] + freq_knob) * r_sr;
            CONSTRAIN(frequency, 0.f, 0.4f);
            // no filtering for now
//            ONE_POLE(freq_lp, frequency, 0.3f);
//            frequency = freq_lp;
            must_reset_ramp_extractor = true;
        }
        
        
        // parameter inputs
        shape = shape_knob + (float)shape_in[count];
        CONSTRAIN(shape, 0.f, 1.f);
        ONE_POLE(shape_lp, shape, 0.1f);
        slope = slope_knob + (float)slope_in[count];
        CONSTRAIN(slope, 0.f, 1.f);
        ONE_POLE(slope_lp, slope, 0.1f);
        smoothness = smoothness_knob + (float)smooth_in[count];
        CONSTRAIN(smoothness, 0.f, 1.f);
        ONE_POLE(smooth_lp, smoothness, 0.1f);
        shift = shift_knob + shift_in[count];
        CONSTRAIN(shift, 0.f, 1.f);
        ONE_POLE(shift_lp, shift, 0.1f);
        
        
        self->poly_slope_generator.Render(ramp_mode,
                                          output_mode,
                                          range,
                                          frequency, slope_lp, shape_lp, smooth_lp, shift_lp,
                                          gate_flags,
                                          !use_trigger && use_clock ? ramp : NULL,
                                          out, kAudioBlockSize);


        for(int i=0; i<kAudioBlockSize; ++i) {
            for(int j=0; j<kNumOutputs; ++j) {
                outs[j][i + count] = out[i].channel[j] * 0.1f;
            }
        }
        
    }
    
    self->freq_lp = freq_lp;
    self->shape_lp = shape_lp;
    self->shift_lp = shift_lp;
    self->slope_lp = slope_lp;
    self->smooth_lp = smooth_lp;
    
    self->must_reset_ramp_extractor = must_reset_ramp_extractor;

}



void myObj_dsp64(t_myObj* self, t_object* dsp64, short* count, double samplerate, long maxvectorsize, long flags)
{
    // is a signal connected to the trigger/clock input?
    self->trig_connected = count[5];
    self->clock_connected = count[6];
    
    
    if(maxvectorsize < kAudioBlockSize) {
        object_error((t_object*)self, "sigvs can't be smaller than %d samples, sorry!", kAudioBlockSize);
        return;
    }
    
    if(samplerate != self->sr) {
        self->sr = samplerate;
        self->r_sr = 1.f / self->sr;
        
        self->ramp_extractor.Init(self->sr, 40.0f * self->r_sr);
    }
    
        object_method_direct(void, (t_object*, t_object*, t_perfroutine64, long, void*),
                         dsp64, gensym("dsp_add64"), (t_object*)self, (t_perfroutine64)myObj_perform64, 0, NULL);
    
}




//void myObj_free(t_myObj* self)
//{
//    dsp_free((t_pxobject*)self);
//
//}


void myObj_assist(t_myObj* self, void* unused, t_assist_function io, long index, char* string_dest)
{
	if (io == ASSIST_INLET) {
		switch (index) {
			case 0:
                strncpy(string_dest,"(signal) FREQ", ASSIST_STRING_MAXSIZE); break;
            case 1:
                strncpy(string_dest,"(signal) SHAPE", ASSIST_STRING_MAXSIZE); break;
            case 2:
                strncpy(string_dest,"(signal) SLOPE", ASSIST_STRING_MAXSIZE); break;
            case 3:
                strncpy(string_dest,"(signal) SMOOTHNESS", ASSIST_STRING_MAXSIZE); break;
            case 4:
                strncpy(string_dest,"(signal) SHIFT/LEVEL", ASSIST_STRING_MAXSIZE); break;
            case 5:
                strncpy(string_dest,"(signal) TRIG IN", ASSIST_STRING_MAXSIZE); break;
            case 6:
                strncpy(string_dest,"(signal) CLOCK IN", ASSIST_STRING_MAXSIZE); break;
		}
	}
	else if (io == ASSIST_OUTLET) {
		switch (index) {
            case 0:
                strncpy(string_dest,"(signal) OUT 1", ASSIST_STRING_MAXSIZE);
                break;
            case 1:
                strncpy(string_dest,"(signal) OUT 2", ASSIST_STRING_MAXSIZE);
                break;
            case 2:
                strncpy(string_dest,"(signal) OUT 3", ASSIST_STRING_MAXSIZE);
                break;
            case 3:
                strncpy(string_dest,"(signal) OUT 4", ASSIST_STRING_MAXSIZE);
                break;
		}
	}
}


void ext_main(void* r) {
	this_class = class_new("vb.mi.tds~", (method)myObj_new, (method)dsp_free, sizeof(t_myObj), 0, A_GIMME, 0);

	class_addmethod(this_class, (method)myObj_assist,	"assist",	A_CANT,		0);
	class_addmethod(this_class, (method)myObj_dsp64,	"dsp64",	A_CANT,		0);

    class_addmethod(this_class, (method)myObj_int,      "int",      A_LONG, 0);
    class_addmethod(this_class, (method)myObj_float,    "float",    A_FLOAT, 0);
    class_addmethod(this_class, (method)myObj_freq,     "freq",     A_FLOAT, 0);
    class_addmethod(this_class, (method)myObj_shape,    "shape",    A_FLOAT, 0);
    class_addmethod(this_class, (method)myObj_shift,    "shift",    A_FLOAT, 0);
    class_addmethod(this_class, (method)myObj_slope,    "slope",    A_FLOAT, 0);
    class_addmethod(this_class, (method)myObj_smooth,   "smooth",   A_FLOAT, 0);
    class_addmethod(this_class, (method)myObj_ratio,    "ratio",    A_LONG, 0);

    
	class_dspinit(this_class);
	class_register(CLASS_BOX, this_class);
    
    // ATTRIBUTES ..............
    // output mode
    CLASS_ATTR_CHAR(this_class, "output_mode", 0, t_myObj, output_mode);
    CLASS_ATTR_ENUMINDEX(this_class, "output_mode", 0, "GATE AMPLITUDE PHASE FREQUENCY");
    CLASS_ATTR_LABEL(this_class, "output_mode", 0, "output mode");
    CLASS_ATTR_FILTER_CLIP(this_class, "output_mode", 0, 3);
    CLASS_ATTR_ACCESSORS(this_class, "output_mode", NULL, (method)output_mode_setter);
    CLASS_ATTR_SAVE(this_class, "output_mode", 0);
    
    // ramp mode
    CLASS_ATTR_CHAR(this_class, "ramp_mode", 0, t_myObj, ramp_mode);
    CLASS_ATTR_ENUMINDEX(this_class, "ramp_mode", 0, "AD LOOPING AR");
    CLASS_ATTR_LABEL(this_class, "ramp_mode", 0, "ramp mode");
    CLASS_ATTR_FILTER_CLIP(this_class, "ramp_mode", 0, 2);
    CLASS_ATTR_ACCESSORS(this_class, "ramp_mode", NULL, (method)ramp_mode_setter);
    CLASS_ATTR_SAVE(this_class, "ramp_mode", 0);
    
    // range
    CLASS_ATTR_CHAR(this_class, "range", 0, t_myObj, range);
    CLASS_ATTR_ENUMINDEX(this_class, "range", 0, "CONTROL AUDIO");
    CLASS_ATTR_LABEL(this_class, "range", 0, "range selector");
    CLASS_ATTR_FILTER_CLIP(this_class, "range", 0, 1);
    CLASS_ATTR_ACCESSORS(this_class, "range", NULL, (method)range_setter);
    CLASS_ATTR_SAVE(this_class, "range", 0);
    
    
    object_post(NULL, "vb.mi.tds~ by volker boehm -- https://vboehm.net");
    object_post(NULL, "based on mutable instruments' 'tides' module");
}

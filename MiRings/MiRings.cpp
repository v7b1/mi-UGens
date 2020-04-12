
#include "SC_PlugIn.h"

#include "rings/dsp/part.h"
#include "rings/dsp/strummer.h"
#include "rings/dsp/string_synth_part.h"
#include "rings/dsp/dsp.h"
//#include "TriggerInput.h"


//float kSampleRate = 48000.0;
int kBlockSize = rings::kMaxBlockSize;
//float a0 = (440.0f / 8.0f) / kSampleRate;

static InterfaceTable *ft;


struct MiRings : public Unit {
    
    rings::Part             part;
    rings::StringSynthPart  string_synth;
    rings::Strummer         strummer;
    
    float                   in_level;
    
    uint16_t                *reverb_buffer;
    
    rings::PerformanceState performance_state;
    rings::Patch            patch;
    
    double                  sr;
    int                     sigvs;
    short                   strum_connected;
    short                   fm_patched;
    bool                    prev_trig;
    int                     prev_poly;
    int                     pcount;
    
};


static void MiRings_Ctor(MiRings *unit);
static void MiRings_Dtor(MiRings *unit);
static void MiRings_next(MiRings *unit, int inNumSamples);
void myObj_info(MiRings *unit);



static void MiRings_Ctor(MiRings *unit) {
    
    Print("MiRings SR: %f\n", SAMPLERATE);
    //kSampleRate = SAMPLERATE;
    //a0 = (440.0f / 8.0f) / kSampleRate;

    // init some params
    unit->in_level = 0.0f;
    
    // allocate memory
    unit->reverb_buffer = (uint16_t*)RTAlloc(unit->mWorld, 65536*sizeof(uint16_t));

    // init with zeros
    memset(unit->reverb_buffer, 0, 65536*sizeof(uint16_t));

    unit->strummer.Init(0.01, rings::kSampleRate / kBlockSize);
    unit->part.Init(unit->reverb_buffer);
    unit->string_synth.Init(unit->reverb_buffer);
    
    unit->part.set_polyphony(1);
    unit->part.set_model(rings::RESONATOR_MODEL_MODAL);

    unit->string_synth.set_polyphony(1);
    unit->string_synth.set_fx(rings::FX_FORMANT);
    unit->prev_poly = 1;
    
    unit->performance_state.internal_exciter = false;
    unit->performance_state.internal_strum = true;
    unit->performance_state.internal_note = true;
    unit->prev_trig = false;
    
    unit->pcount = 0;
    
    SETCALC(MiRings_next);        // tells sc synth the name of the calculation function
    //MiRings_next(unit, 64);       // do we reallly need this?

}


static void MiRings_Dtor(MiRings *unit) {
    
    SCWorld_Allocator alloc(ft, unit->mWorld);

    if(unit->reverb_buffer) {
        RTFree(unit->mWorld, unit->reverb_buffer);
    }
    Print("done freeing memory!\n");
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
    int     model = IN0(7);
    int     polyphony = IN0(8);
    bool    useTrigger = IN0(9);
    bool    intern_exciter = IN0(10);
    bool    intern_note = IN0(11);
    bool    bypass = IN0(12);
    
    
    float *out1 = OUT(0);
    float *out2 = OUT(1);
    
    float input[64];

    size_t     size = kBlockSize;   //rings::kBlockSize;
    uint8_t    count = 0;
    rings::Patch *patch = &unit->patch;
    
    unit->performance_state.internal_note = intern_note;
    unit->performance_state.internal_exciter = intern_exciter;
    
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
    float pitch = abs(voct_in);
    CONSTRAIN(pitch, 0.f, 127.f);
    unit->performance_state.tonic = pitch;
    unit->performance_state.note = 0.0f;


    CONSTRAIN(struct_in, 0.0f, 0.9995f);
    patch->structure = struct_in;
    float chord = struct_in;
    chord *= static_cast<float>(rings::kNumChords - 1);       // 11 - 1
    int32_t chord_ = static_cast<int32_t>(chord);
    CONSTRAIN(chord_, 0, rings::kNumChords - 1);
    unit->performance_state.chord = chord_;

    CONSTRAIN(bright_in, 0.0f, 1.0f);
    patch->brightness = bright_in;

    CONSTRAIN(damp_in, 0.0f, 1.0f);
    patch->damping = damp_in;
    
    CONSTRAIN(pos_in, 0.0f, 1.0f);
    patch->position = pos_in;
    
    if(INRATE(0) == calc_FullRate) {
        // we have to copy input vector (why?)
        std::copy(&in[0], &in[size], &input[0]);
    }
    else {
        // set input to zero
        memset(input, 0, size*sizeof(float));
    }
    
    if(useTrigger) {
        unit->performance_state.internal_strum = false;
        
        bool trig = false;
        bool prev_trig = unit->prev_trig;
        
        if(INRATE(1) == calc_FullRate) {    // trigger input is audio rate
            for(int i=0; i<inNumSamples; ++i) {
                if(trig_in[i] > 0.f && !prev_trig) {
                    trig = true;
                    //printf("trigger\n");
                    break;
                }
            }
            unit->prev_trig = trig;
            unit->performance_state.strum = trig;
        }
        else {          // trigger input is control rate
            bool trig = (trig_in[0] > 0.f);
            if(trig) {
                if(!prev_trig) {
                    unit->performance_state.strum = true;
                    //printf("trigger+\n");
                } else
                    unit->performance_state.strum = false;
            }
            unit->prev_trig = trig;
        }
        
    } else
        unit->performance_state.internal_strum = true;
    
/*
    unit->pcount++;
    if(unit->pcount >= 2000) {
        unit->pcount = 0;
        myObj_info(unit);
    }
  */

    unit->part.set_bypass(bypass);
    
    unit->strummer.Process(in, size, &unit->performance_state);
    //if(unit->performance_state.strum)
      //  printf("STRUM!\n");
    unit->part.Process(unit->performance_state, unit->patch, input, out1, out2, size);
    
    /*
    for(count = 0; count < inNumSamples; count += size) {
        
        
        float trig = 0.f;
        for(int i=0; i<inNumSamples; ++i) {
            if(in[i] > 0.001f) {
                trig = 1.0f;
                unit->performance_state.strum = true;
                printf("trigger!\n");
                break;
            }
        }*/
        
        /*
        unit->strummer->Process(in+count, size, &unit->performance_state);
        unit->part.Process(unit->performance_state, unit->patch, in+count, out1+count, out2+count, size);
        
    }*/
    /*
    for(int i=0; i<size; i++) {
        out1[i] = in[i];
        out2[i] = in[i];
    }*/
    
}


void myObj_info(MiRings *unit)
{
    
    rings::Patch p = unit->patch;
    rings::PerformanceState *ps = &unit->performance_state;
    rings::Part *pa = &unit->part;
    //rings::State *st = self->settings->mutable_state();

    printf( "Patch ----------------------->\n");
    printf("structure: %f\n", p.structure);
    printf("brightness: %f\n", p.brightness);
    printf("damping: %f\n", p.damping);
    printf("position: %f\n", p.position);
    
    printf("Performance_state ----------->\n");
    printf("strum: %d\n", ps->strum);
    printf("internal_exciter: %d\n", ps->internal_exciter);
    printf("internal_strum: %d\n", ps->internal_strum);
    printf("internal_note: %d\n", ps->internal_note);
    printf("tonic: %f\n", ps->tonic);
    printf("note: %f\n", ps->note);
    printf("fm: %f\n", ps->fm);
    printf("chord: %d\n", ps->chord);
    
    printf("polyphony: %d\n", pa->polyphony());
    printf("model: %d\n", pa->model());
    
    printf( "-----\n");
    
}


PluginLoad(MiRings) {
    ft = inTable;
    DefineDtorUnit(MiRings);
}




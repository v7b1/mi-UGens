

#include "SC_PlugIn.h"

#include "plaits/dsp/dsp.h"
#include "plaits/dsp/voice.h"


float kSampleRate = 48000.0;
float a0 = (440.0f / 8.0f) / kSampleRate;

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
    plaits::Voice::Frame   output[16];
    float               sr;
    int                 sigvs;
};


static void MiPlaits_Ctor(MiPlaits *unit);
static void MiPlaits_Dtor(MiPlaits *unit);
static void MiPlaits_next(MiPlaits *unit, int inNumSamples);



static void MiPlaits_Ctor(MiPlaits *unit) {
    //printf("MiPlaits constructor!\n");
    
    //printf("sr: %f\n", SAMPLERATE);
    kSampleRate = SAMPLERATE;
    a0 = (440.0f / 8.0f) / kSampleRate;

    // init some params
    unit->transposition_ = 0.;
    unit->octave_ = 0.5;
    unit->patch.note = 48.0;
    unit->patch.harmonics = 0.1;
        
    
    // allocate memory
    unit->shared_buffer = (char*)RTAlloc(unit->mWorld, 65536);
    // init with zeros?
    memset(unit->shared_buffer, 0, 65536);

    if(unit->shared_buffer == NULL) {
        printf("mem alloc failed!");
        unit = NULL;
    }
    stmlib::BufferAllocator allocator(unit->shared_buffer, 65536);

    unit->voice_ = new plaits::Voice;
    unit->voice_->Init(&allocator);
    
    memset(&unit->patch, 0, sizeof(unit->patch));
    memset(&unit->modulations, 0, sizeof(unit->modulations));
    

    SETCALC(MiPlaits_next);        // tells sc synth the name of the calculation function
    //MiPlaits_next(unit, 64);       // do we reallly need this?
    
}


static void MiPlaits_Dtor(MiPlaits *unit) {
    
    SCWorld_Allocator alloc(ft, unit->mWorld);

    unit->voice_->plaits::Voice::~Voice();
    delete unit->voice_;
    if(unit->shared_buffer) {
        RTFree(unit->mWorld, unit->shared_buffer);
    }
}


#pragma mark ----- dsp loop -----

void MiPlaits_next( MiPlaits *unit, int inNumSamples)
{
    /*
    float *voct = IN(0);
    float *engine_in = IN(1);
    float *harm_in = IN(2);
    float *timbre_in = IN(3);
    float *morph_in = IN(4);
    */
    float voct = IN0(0);
    float engine_in = IN0(1);
    
    float harm_in = IN0(2);
    float timbre_in = IN0(3);
    float morph_in = IN0(4);
    float *trig_in = IN(5);
    float useTrigger = IN0(6);
    float decay_in = IN0(7);
    float lpgColor_in = IN0(8);
    
    float *out = OUT(0);
    float *aux = OUT(1);

    size_t     size = plaits::kBlockSize;
    plaits::Voice::Frame   *output = unit->output;
    uint8_t    count = 0;
    
    
    float pitch = abs(voct);
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
    
    CONSTRAIN(decay_in, 0.0f, 1.0f);
    unit->patch.decay = decay_in;
    
    CONSTRAIN(lpgColor_in, 0.0f, 1.0f);
    unit->patch.lpg_colour = lpgColor_in;
    
    if(useTrigger != 0) {
        unit->modulations.trigger_patched = true;
        float trig = 0.f;
        for(int i=0; i<inNumSamples; ++i) {
            if(trig_in[i] > 0.001f) {
                trig = 1.0f;
                break;
            }
        }
        unit->modulations.trigger = trig;
    }
    else
        unit->modulations.timbre_patched = false;
    
    
    for(count = 0; count < inNumSamples; count += size) {
        
        unit->voice_->Render(unit->patch, unit->modulations, output, size);

        for(auto i=0; i<size; ++i) {
            out[count+i] = output[i].out / 32768.f;
            aux[count+i] = output[i].aux / 32768.f;
        }
    }
    
}



PluginLoad(MiPlaits) {
    ft = inTable;
    DefineDtorUnit(MiPlaits);
}




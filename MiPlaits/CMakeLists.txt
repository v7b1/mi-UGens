cmake_minimum_required (VERSION 2.8)
project (MiPlaits)

# paths to our sources
set(MUTABLE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../eurorack")
set(STMLIB_PATH ${MUTABLE_PATH}/stmlib)
set(MI_PATH ${MUTABLE_PATH}/plaits)

include_directories(${SC_PATH}/include/plugin_interface)
include_directories(${SC_PATH}/include/common)
include_directories(${SC_PATH}/external_libraries/libsndfile/)

include_directories(${MUTABLE_PATH})



set(STMLIB_SOURCES 
	${STMLIB_PATH}/stmlib.h
	${STMLIB_PATH}/utils/random.cc
	${STMLIB_PATH}/utils/random.h
	${STMLIB_PATH}/utils/dsp.h
	${STMLIB_PATH}/dsp/atan.cc
	${STMLIB_PATH}/dsp/atan.h
	${STMLIB_PATH}/dsp/units.cc
	${STMLIB_PATH}/dsp/units.h

)

set(MI_SOURCES 
	${MI_PATH}/resources.cc
	${MI_PATH}/resources.h
	${MI_PATH}/dsp/voice.cc
	${MI_PATH}/dsp/voice.h
	${MI_PATH}/dsp/speech/lpc_speech_synth.cc
	${MI_PATH}/dsp/speech/lpc_speech_synth.h
	${MI_PATH}/dsp/speech/lpc_speech_synth_controller.cc
	${MI_PATH}/dsp/speech/lpc_speech_synth_controller.h
	${MI_PATH}/dsp/speech/lpc_speech_synth_phonemes.cc
	${MI_PATH}/dsp/speech/lpc_speech_synth_words.cc
	${MI_PATH}/dsp/speech/lpc_speech_synth_words.h
	${MI_PATH}/dsp/speech/naive_speech_synth.cc
	${MI_PATH}/dsp/speech/naive_speech_synth.h
	${MI_PATH}/dsp/speech/sam_speech_synth.cc
	${MI_PATH}/dsp/speech/sam_speech_synth.h
	${MI_PATH}/dsp/drums/analog_bass_drum.h
	${MI_PATH}/dsp/drums/analog_snare_drum.h
	${MI_PATH}/dsp/drums/hi_hat.h
	${MI_PATH}/dsp/drums/synthetic_bass_drum.h
	${MI_PATH}/dsp/drums/synthetic_snare_drum.h
	${MI_PATH}/dsp/dsp.h
	${MI_PATH}/dsp/engine/additive_engine.cc
	${MI_PATH}/dsp/engine/additive_engine.h
	${MI_PATH}/dsp/engine/bass_drum_engine.cc
	${MI_PATH}/dsp/engine/bass_drum_engine.h
	${MI_PATH}/dsp/engine/chord_engine.cc
	${MI_PATH}/dsp/engine/chord_engine.h
	${MI_PATH}/dsp/engine/engine.h
	${MI_PATH}/dsp/engine/fm_engine.cc
	${MI_PATH}/dsp/engine/fm_engine.h
	${MI_PATH}/dsp/engine/grain_engine.cc
	${MI_PATH}/dsp/engine/grain_engine.h
	${MI_PATH}/dsp/engine/hi_hat_engine.cc
	${MI_PATH}/dsp/engine/hi_hat_engine.h
	${MI_PATH}/dsp/engine/modal_engine.cc
	${MI_PATH}/dsp/engine/modal_engine.h
	${MI_PATH}/dsp/engine/noise_engine.cc
	${MI_PATH}/dsp/engine/noise_engine.h
	${MI_PATH}/dsp/engine/particle_engine.cc
	${MI_PATH}/dsp/engine/particle_engine.h
	${MI_PATH}/dsp/engine/snare_drum_engine.cc
	${MI_PATH}/dsp/engine/snare_drum_engine.h
	${MI_PATH}/dsp/engine/speech_engine.cc
	${MI_PATH}/dsp/engine/speech_engine.h
	${MI_PATH}/dsp/engine/string_engine.cc
	${MI_PATH}/dsp/engine/string_engine.h
	${MI_PATH}/dsp/engine/swarm_engine.cc
	${MI_PATH}/dsp/engine/swarm_engine.h
	${MI_PATH}/dsp/engine/virtual_analog_engine.cc
	${MI_PATH}/dsp/engine/virtual_analog_engine.h
	${MI_PATH}/dsp/engine/waveshaping_engine.cc
	${MI_PATH}/dsp/engine/waveshaping_engine.h
	${MI_PATH}/dsp/engine/wavetable_engine.cc
	${MI_PATH}/dsp/engine/wavetable_engine.h
	${MI_PATH}/dsp/envelope.h
	${MI_PATH}/dsp/fx/diffuser.h
	${MI_PATH}/dsp/fx/fx_engine.h
	${MI_PATH}/dsp/fx/low_pass_gate.h
	${MI_PATH}/dsp/fx/overdrive.h
	${MI_PATH}/dsp/fx/sample_rate_reducer.h
	${MI_PATH}/dsp/noise/clocked_noise.h
	${MI_PATH}/dsp/noise/dust.h
	${MI_PATH}/dsp/noise/fractal_random_generator.h
	${MI_PATH}/dsp/noise/particle.h
	${MI_PATH}/dsp/noise/smooth_random_generator.h
	${MI_PATH}/dsp/oscillator/formant_oscillator.h
	${MI_PATH}/dsp/oscillator/grainlet_oscillator.h
	${MI_PATH}/dsp/oscillator/harmonic_oscillator.h
	${MI_PATH}/dsp/oscillator/oscillator.h
	${MI_PATH}/dsp/oscillator/sine_oscillator.h
	${MI_PATH}/dsp/oscillator/string_synth_oscillator.h
	${MI_PATH}/dsp/oscillator/variable_saw_oscillator.h
	${MI_PATH}/dsp/oscillator/variable_shape_oscillator.h
	${MI_PATH}/dsp/oscillator/vosim_oscillator.h
	${MI_PATH}/dsp/oscillator/wavetable_oscillator.h
	${MI_PATH}/dsp/oscillator/z_oscillator.h
	${MI_PATH}/dsp/physical_modelling/delay_line.h
	${MI_PATH}/dsp/physical_modelling/modal_voice.cc
	${MI_PATH}/dsp/physical_modelling/modal_voice.h
	${MI_PATH}/dsp/physical_modelling/resonator.cc
	${MI_PATH}/dsp/physical_modelling/resonator.h
	${MI_PATH}/dsp/physical_modelling/string.cc
	${MI_PATH}/dsp/physical_modelling/string.h
	${MI_PATH}/dsp/physical_modelling/string_voice.cc
	${MI_PATH}/dsp/physical_modelling/string_voice.h
)



set(BUILD_SOURCES 
	MiPlaits.cpp
)



set(CMAKE_SHARED_MODULE_PREFIX "")
if(APPLE OR WIN32)
set(CMAKE_SHARED_MODULE_SUFFIX ".scx")
endif()

add_library(MiPlaits MODULE
        ${STMLIB_SOURCES}
        ${MI_SOURCES}
        ${BUILD_SOURCES}
)

if(APPLE)
# create groups in our project
source_group(TREE ${MUTABLE_PATH} FILES ${STMLIB_SOURCES} ${MI_SOURCES})

target_link_libraries(${PROJECT_NAME} PUBLIC "-framework Accelerate")
endif()

// Caleb Crittenden, 2026
// based on mutable instruments' Beads, designed by Émilie Gillet.
// All DSP is her original code (MIT license): https://github.com/pichenettes/eurorack
// Wrapper modeled on Volker Böhm's MiClouds.

MiBeads : MultiOutUGen {

	*ar {
		arg in, pitch=0, pitch_rnd=0, time=0.5, time_rnd=0, size=0.5, size_rnd=0,
		shape=0.5, shape_rnd=0, density=0.7, fb=0, drywet=0.5, rvb=0,
		freeze=0, seed=0, seed_btn=2, quality=0, in_gain=1, synth=0, mul=1.0, add=0.0;

		var inputArray = (in ?? { Silent.ar }).asArray;
		if(inputArray.size > 2) {
			"MiBeads: only the first two input channels are used".warn;
			inputArray = inputArray.keep(2);
		};

		^this.multiNewList(['audio', pitch, pitch_rnd, time, time_rnd, size, size_rnd,
			shape, shape_rnd, density, fb, drywet, rvb,
			freeze, seed, seed_btn, quality, in_gain, synth] ++ inputArray).madd(mul, add);
	}

	checkInputs {
		(18..inputs.size - 1).do { |i|
			if(inputs.at(i).rate != 'audio') {
				^("input is not audio rate:" + inputs.at(i) + inputs.at(i).rate);
			};
		};
		^this.checkValidInputs;
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}
}

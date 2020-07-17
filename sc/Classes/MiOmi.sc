// volker böhm, 2020 - https://vboehm.net

MiOmi : MultiOutUGen {

	*ar {
		arg audio_in=0, gate=0, pit=48, contour=0.2, detune=0.25, level1=0.5, level2=0.5,
		ratio1=0.5, ratio2=0.5, fm1=0, fm2=0, fb=0, xfb=0,
		filter_mode=0, cutoff=0.5, reson=0, strength=0.5, env=0.5, rotate=0.2, space=0.5,
		mul=1.0, add=0;

		^this.multiNew('audio', audio_in, gate, pit, contour, detune,
			level1, level2, ratio1, ratio2, fm1, fm2, fb, xfb, filter_mode,
			cutoff, reson, strength, env, rotate, space).madd(mul, add);
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

}
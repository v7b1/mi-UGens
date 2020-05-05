// volker böhm, 2020 - https://vboehm.net

MiElements : MultiOutUGen {

	*ar {
		arg blow_in=0, strike_in=0, gate=0, pit=48, strength=0.5, contour=0.2, bow_level=0,
		blow_level=0, strike_level=0, flow=0.5, mallet=0.5, bow_timb=0.5, blow_timb=0.5,
		strike_timb=0.5, geom=0.25, bright=0.5, damp=0.7, pos=0.2, space=0.3, model=0,
		easteregg=0, mul=1.0, add=0;

		^this.multiNew('audio', blow_in, strike_in, gate, pit, strength, contour, bow_level,
			blow_level, strike_level, flow, mallet, bow_timb, blow_timb, strike_timb, geom,
			bright, damp, pos, space, model, easteregg).madd(mul, add);
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

}
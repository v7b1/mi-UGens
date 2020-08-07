// volker böhm, 2020 - https://vboehm.net

MiTides : MultiOutUGen {

	*ar {
		arg freq=1,  shape=0.5, slope=0.5, smooth=0.5, shift=0.2, trig=0, clock=0, output_mode=3,
		ramp_mode=1, ratio=9, rate=1, mul=1.0, add=0.0;
		^this.multiNew('audio', freq, shape, slope, smooth, shift, trig, clock, output_mode,
			ramp_mode, ratio, rate ).madd(mul, add);
	}

/*	checkInputs {
		if ( inputs.at(5).rate != 'audio', {
			^("input is not audio rate:" + inputs.at(5) + inputs.at(5).rate);
		});
		^this.checkValidInputs;
	}*/

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(4, rate);
	}
}
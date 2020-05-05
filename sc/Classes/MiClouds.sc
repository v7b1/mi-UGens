// volker böhm, 2020 - https://vboehm.net

MiClouds : MultiOutUGen {

	*ar {
		arg inputArray, pit=0, pos=0.5, size=0.25, dens=0.4, tex=0.5, drywet=0.5, in_gain=1,
		spread=0.5, rvb=0, fb=0, freeze=0, mode=0, lofi=0, trig=0, mul=1.0, add=0.0;
		^this.multiNewList(['audio', pit, pos, size, dens, tex, drywet, in_gain, spread, rvb, fb,
			freeze, mode, lofi, trig] ++ inputArray.asArray).madd(mul);
	}

	checkInputs {
		if ( inputs.at(14).rate != 'audio', {
			^("input is not audio rate:" + inputs.at(14) + inputs.at(14).rate);
		});
		^this.checkValidInputs;
	}

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}
}
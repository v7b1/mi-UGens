// volker böhm, 2020 - https://vboehm.net

MiWarps : MultiOutUGen {

	*ar {
		arg carrier=0, modulator=0, lev1=0.5, lev2=0.5, algo=0, timb=0, osc=1, freq=110, easteregg=0, mul=1.0, add=0;

		^this.multiNew('audio', carrier, modulator, lev1, lev2, algo, timb, osc, freq, easteregg).madd(mul, add);
	}
	/*
	checkInputs {
		if ( inputs.at(0).rate == 'control', {
			^("audio input can't be control rate!");
		});
		^this.checkValidInputs
	}*/

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

}
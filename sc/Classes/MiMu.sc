// volker böhm, 2020 - https://vboehm.net

MiMu : UGen {

	*ar {
		arg in, gain=1.0, bypass=0, mul=1.0, add=0;
		^this.multiNew('audio', in, gain, bypass).madd(mul, add);
	}
	checkInputs { ^this.checkSameRateAsFirstInput }

}
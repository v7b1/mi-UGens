// volker böhm, 2020 - https://vboehm.net

MiRipples : UGen {

	*ar {
		arg in, cf=0.3, reson=0.2, drive=1.0, mul=1.0, add=0;
		^this.multiNew('audio', in, cf, reson, drive).madd(mul, add);
	}
	checkInputs { ^this.checkSameRateAsFirstInput }

}
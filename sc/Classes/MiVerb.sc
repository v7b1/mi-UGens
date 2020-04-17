// volker böhm, 2020 - https://vboehm.net

MiVerb : MultiOutUGen {

	*ar {
		arg in, time=0.7, amount=0.5, diff=0.7, lp=0.5, freeze=0, mul=1.0, add=0.0;
		^this.multiNew('audio', in, time, amount, diff, lp, freeze).madd(mul);
	}
	//checkInputs { ^this.checkSameRateAsFirstInput }

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}
}
// vboehm.net, 2020

MiRings : MultiOutUGen {

	*ar {
		arg in=0, trig=0, pit=60.0, struct=0.25, bright=0.7, damp=0.7, pos=0.25, model=0, poly=1, use_trig=0, intern_exciter=0, intern_note=1, bypass=0, mul=1.0, add=0;
		^this.multiNew('audio', in, trig, pit, struct, bright, damp, pos, model, poly, use_trig, intern_exciter, intern_note, bypass).madd(mul, add);
	}
	//checkInputs { ^this.checkSameRateAsFirstInput }

	init { arg ... theInputs;
		inputs = theInputs;
		^this.initOutputs(2, rate);
	}

}
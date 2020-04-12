// vboehm.net, 2019

MiPlaits : MultiOutUGen {

	*ar {
		arg pitch=60.0, engine=0, harm=0.1, timbre=0.5, morph=0.5, trigger=0.0, usetrigger=0, decay=0.5,
		lpg_colour=0.5, mul=0.5;
		^this.multiNew('audio', pitch, engine, harm, timbre, morph, trigger, usetrigger, decay).madd(mul);
	}
	//checkInputs { ^this.checkSameRateAsFirstInput }

	init { arg ... theInputs;
		inputs = theInputs;  // das heisst, argNumChannels wird nicht an UGen uebergeben!
		^this.initOutputs(2, rate);
	}
}